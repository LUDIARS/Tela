// @spec SPEC-TL-DESKTOP-OVERLAY
#include "desktop_monitors.hpp"
#include "layered_composition.hpp"
#include "native_pointer.hpp"
#include <tela/windows_desktop_overlay.hpp>
#include <optional>
#include <stdexcept>
#include <utility>

namespace tela {
namespace {
constexpr wchar_t class_name[] = L"Tela.DesktopOverlay.v1";
constexpr char host_id[] = "desktop";

// A drag in progress: where the pointer went down and how far the windows have been moved.
struct Drag { POINT start{}; int dx{}, dy{}; };

// The pointer in desktop physical pixels. Window procedures run in the window's own
// Per-Monitor V2 context, so the cursor needs no conversion; the grip window's client
// coordinates would, and the grip itself moves during a drag.
POINT cursor_point() {
    POINT position{};
    GetCursorPos(&position);
    return position;
}

// The physical button the user treats as primary, which swapped buttons move to the right.
bool primary_button_down() {
    const int key = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}
}

struct WindowsDesktopOverlay::Impl {
    Runtime& runtime;
    PictorSurface& renderer;
    DesktopPlacement placement;
    std::string grip;
    std::function<void(const DesktopPlacement&)> moved;
    windows::NativePointer pointer;
    std::exception_ptr error;
    std::optional<Drag> drag;
    // A finished drag's top-left, applied by the next synchronize().
    std::optional<POINT> dropped;
    std::string hover;
    HWND hover_window{};
    bool visible{};
    // Declared last so its windows are destroyed while everything their messages touch is alive.
    windows::LayeredComposition composition{[this](bool passthrough) { return create(passthrough); }};

    Impl(Runtime& r, PictorSurface& p, DesktopOverlayOptions options)
        : runtime(r), renderer(p), placement(options.placement), grip(std::move(options.grip)),
          moved(std::move(options.moved)) {
        windows::PerMonitorScope dpi;
        if(!dpi.active()) throw std::runtime_error("Tela desktop overlay requires Per-Monitor V2 DPI awareness");
        // A size or margin no layout can satisfy fails here, before any window exists.
        place_on_desktop(placement, windows::desktop_monitors());
        WNDCLASSEXW description{sizeof(description)};
        description.lpfnWndProc = procedure;
        description.hInstance = GetModuleHandleW(nullptr);
        description.lpszClassName = class_name;
        description.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if(!RegisterClassExW(&description) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register Tela desktop overlay window class");
        composition.open();
    }
    ~Impl() {
        windows::PerMonitorScope dpi;
        hide();
        composition.destroy();
        // The surface is gone, so presentation and input ownership end as for a lost host.
        runtime.disconnect();
    }
    HWND create(bool passthrough) {
        // No owner and a tool window: nothing in the taskbar or Alt+Tab, and no host to follow.
        const auto window = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | (passthrough ? WS_EX_TRANSPARENT : 0),
            class_name, L"Tela", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), this);
        if(!window) throw std::runtime_error("Cannot create Tela desktop overlay window");
        return window;
    }
    void hide() {
        runtime.cancel();
        // Releasing capture ends a drag; its drop is kept for the next synchronize().
        composition.hide();
        hover.clear(); hover_window = nullptr;
        visible = false;
    }

    static LRESULT CALLBACK procedure(HWND w, UINT m, WPARAM wp, LPARAM lp) noexcept {
        auto self = reinterpret_cast<Impl*>(GetWindowLongPtrW(w, GWLP_USERDATA));
        if(m == WM_NCCREATE) {
            self = static_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if(!self) return DefWindowProcW(w, m, wp, lp);
        try { return self->message(w, m, wp, lp); }
        catch(...) { self->error = std::current_exception(); self->runtime.cancel(); return 0; }
    }
    LRESULT message(HWND w, UINT m, WPARAM wp, LPARAM lp) {
        switch(m) {
        case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
        // synchronize() reads every monitor's DPI and resizes, so the suggested rect is not used.
        case WM_DPICHANGED: return 0;
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
            if(drag) finish_drag(); else runtime.cancel();
            break;
        case WM_MOUSELEAVE:
            if(w == hover_window) { hover.clear(); hover_window = nullptr; }
            return 0;
        case WM_LBUTTONDOWN:
            if(!grip.empty() && composition.region(w) == grip) { begin_drag(w); return 0; }
            break;
        case WM_MOUSEMOVE:
            track_hover(w);
            if(drag) { drag_to(); return 0; }
            break;
        case WM_LBUTTONUP:
            if(drag) { if(GetCapture() == w) ReleaseCapture(); finish_drag(); return 0; }
            break;
        default: break;
        }
        if(windows::NativePointer::handles(m)) {
            // Between a drop and the next synchronize() the windows sit at the new spot but the
            // viewport does not, so the input would land on the wrong element.
            if(!dropped) pointer.forward(runtime, w, m, lp);
            return 0;
        }
        return DefWindowProcW(w, m, wp, lp);
    }
    void begin_drag(HWND w) {
        // Moving the surface is not a click on it: whatever gesture the runtime held ends here.
        runtime.cancel();
        drag = Drag{cursor_point()};
        SetCapture(w);
    }
    void drag_to() {
        const auto at = cursor_point();
        const int dx = at.x - drag->start.x, dy = at.y - drag->start.y;
        composition.offset(dx - drag->dx, dy - drag->dy);
        drag->dx = dx; drag->dy = dy;
    }
    void finish_drag() {
        if(!drag) return;
        const auto finished = *drag;
        drag.reset();
        if(!finished.dx && !finished.dy) return;
        // A drop not yet applied is where the windows started this time.
        const auto& view = runtime.viewport();
        const auto base = dropped.value_or(POINT{view.desktop_x, view.desktop_y});
        dropped = POINT{base.x + finished.dx, base.y + finished.dy};
    }
    void track_hover(HWND w) {
        if(w == hover_window) return;
        auto id = composition.region(w);
        if(id.empty()) return;
        TRACKMOUSEEVENT leave{sizeof(leave), TME_LEAVE, w, 0};
        // Without a leave notification the hover could never end, so it is not reported at all.
        if(!TrackMouseEvent(&leave)) return;
        hover = std::move(id);
        hover_window = w;
    }

    // Reports the frame as the viewport; unchanged geometry keeps the current revision.
    void observe(const DesktopFrame& frame) {
        auto next = runtime.viewport();
        if(next.host_id == host_id && next.desktop_x == frame.x && next.desktop_y == frame.y
           && next.width == frame.width && next.height == frame.height && next.dpi_scale == frame.dpi_scale
           && next.visible && next.focused)
            return;
        ++next.revision;
        next.host_id = host_id; next.view_id = "surface";
        next.desktop_x = frame.x; next.desktop_y = frame.y;
        next.width = frame.width; next.height = frame.height;
        next.dpi_scale = frame.dpi_scale;
        // A surface that is never activated has no host focus to lose, so it takes input while shown.
        next.visible = true; next.focused = true;
        runtime.viewport(next);
    }
    void synchronize() {
        windows::PerMonitorScope dpi;
        if(error) std::rethrow_exception(error);
        // A release the grip never saw (capture taken elsewhere) still ends the drag.
        if(drag && !primary_button_down()) {
            finish_drag();
            if(!composition.region(GetCapture()).empty()) ReleaseCapture();
        }
        // The windows follow the pointer during a drag; the drop presents at the new spot.
        if(drag) return;
        const auto monitors = windows::desktop_monitors();
        if(monitors.empty()) { if(visible) hide(); return; }
        bool settled = false;
        if(dropped) {
            placement = placement_at(placement, dropped->x, dropped->y, monitors);
            dropped.reset();
            settled = true;
        }
        observe(place_on_desktop(placement, monitors));
        if(!visible || runtime.needs_frame()) {
            composition.present(runtime, renderer.render(runtime));
            runtime.frame_presented();
            visible = true;
        }
        if(settled && moved) moved(placement);
    }
    void place(const DesktopPlacement& next) {
        windows::PerMonitorScope dpi;
        place_on_desktop(next, windows::desktop_monitors());
        placement = next;
        // An explicit placement replaces a drop that has not been applied yet.
        dropped.reset();
    }
};

WindowsDesktopOverlay::WindowsDesktopOverlay(Runtime& runtime, PictorSurface& renderer, DesktopOverlayOptions options)
    : impl_(std::make_unique<Impl>(runtime, renderer, std::move(options))) {}
WindowsDesktopOverlay::~WindowsDesktopOverlay() = default;
void WindowsDesktopOverlay::synchronize() { impl_->synchronize(); }
void WindowsDesktopOverlay::hide() { windows::PerMonitorScope dpi; impl_->hide(); }
void WindowsDesktopOverlay::place(const DesktopPlacement& placement) { impl_->place(placement); }
const DesktopPlacement& WindowsDesktopOverlay::placement() const noexcept { return impl_->placement; }
std::string WindowsDesktopOverlay::hovered() const {
    const auto& self = *impl_;
    if(self.hover.empty() || self.composition.region(self.hover_window) != self.hover) return {};
    return self.hover;
}
std::uintptr_t WindowsDesktopOverlay::native_window() const noexcept {
    return reinterpret_cast<std::uintptr_t>(impl_->composition.visual());
}
}
