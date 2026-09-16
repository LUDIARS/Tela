// @spec SPEC-TL-VIEW-HOST
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <tela/windows_view.hpp>
#include <windows.h>
#include <windowsx.h>
#include <cstring>
#include <stdexcept>

namespace tela {
namespace {
constexpr wchar_t class_name[] = L"Tela.View.v1";

std::wstring widen(const std::string& value) {
    if(value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), size);
    return wide;
}
}

struct WindowsView::Impl {
    Runtime& runtime;
    PictorSurface& renderer;
    HWND window{};
    std::exception_ptr error;
    bool closed{}, presented{}, borderless{};
    WINDOWPLACEMENT restore{sizeof(WINDOWPLACEMENT)};
    LONG_PTR restore_style{};
    std::uint64_t sequence{}, gesture{};

    Impl(Runtime& r, PictorSurface& p, const std::string& title, int width, int height, bool start_fullscreen)
        : runtime(r), renderer(p) {
        WNDCLASSEXW description{sizeof(description)};
        description.lpfnWndProc = procedure;
        description.hInstance = GetModuleHandleW(nullptr);
        description.lpszClassName = class_name;
        description.hCursor = LoadCursor(nullptr, IDC_ARROW);
        description.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWTEXT + 1);
        if(!RegisterClassExW(&description) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register Tela view window class");
        RECT wanted{0, 0, width, height};
        AdjustWindowRect(&wanted, WS_OVERLAPPEDWINDOW, FALSE);
        window = CreateWindowExW(0, class_name, widen(title).c_str(), WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, wanted.right - wanted.left, wanted.bottom - wanted.top,
            nullptr, nullptr, GetModuleHandleW(nullptr), this);
        if(!window) throw std::runtime_error("Cannot create Tela view window");
        // Excubitor hides the launcher console through STARTUPINFO, and the first ShowWindow
        // consumes that preference; the view must then show itself independently.
        ShowWindow(window, SW_SHOWDEFAULT);
        ShowWindow(window, SW_SHOWNORMAL);
        SetForegroundWindow(window);
        if(start_fullscreen) set_fullscreen(true);
    }
    ~Impl() { if(window) DestroyWindow(window); }

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
        case WM_DESTROY:
            // Closing the view releases input the same way a lost host does.
            runtime.cancel(); runtime.disconnect(); closed = true; window = nullptr; return 0;
        case WM_ERASEBKGND: return 1; // the surface covers the whole client area
        case WM_SIZE: runtime.invalidate(); return 0;
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE: runtime.cancel(); return 0;
        case WM_KEYDOWN:
            if(wp == VK_ESCAPE) { DestroyWindow(w); return 0; }
            if(wp == VK_F11) { set_fullscreen(!borderless); return 0; }
            return 0;
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_MOUSEMOVE: return pointer(w, m, lp);
        default: return DefWindowProcW(w, m, wp, lp);
        }
    }
    LRESULT pointer(HWND w, UINT m, LPARAM lp) {
        POINT position{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ClientToScreen(w, &position);
        if(m == WM_LBUTTONDOWN) ++gesture;
        HostPointerEvent event;
        event.sequence = ++sequence;
        event.viewport_revision = runtime.viewport().revision;
        event.gesture_id = gesture;
        event.phase = m == WM_LBUTTONDOWN ? PointerPhase::down : m == WM_LBUTTONUP ? PointerPhase::up : PointerPhase::move;
        event.button = m == WM_MOUSEMOVE ? PointerButton::none : PointerButton::primary;
        event.desktop_x = position.x;
        event.desktop_y = position.y;
        runtime.pointer(event, InputSource::native);
        if(m == WM_LBUTTONDOWN && runtime.captured()) SetCapture(w);
        if(m == WM_LBUTTONUP && GetCapture() == w) ReleaseCapture();
        return 0;
    }
    void set_fullscreen(bool wanted) {
        if(!window || wanted == borderless) return;
        if(wanted) {
            restore_style = GetWindowLongPtrW(window, GWL_STYLE);
            GetWindowPlacement(window, &restore);
            MONITORINFO monitor{sizeof(monitor)};
            if(!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor)) return;
            SetWindowLongPtrW(window, GWL_STYLE, (restore_style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        } else {
            SetWindowLongPtrW(window, GWL_STYLE, restore_style);
            SetWindowPlacement(window, &restore);
            SetWindowPos(window, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        borderless = wanted;
        runtime.invalidate();
    }
    // Reports the client area as the viewport. Returns false when nothing can be drawn.
    bool observe() {
        if(error) std::rethrow_exception(error);
        if(!window || closed) return false;
        RECT client{};
        POINT origin{};
        if(!GetClientRect(window, &client) || !ClientToScreen(window, &origin)) return false;
        auto next = runtime.viewport();
        const float dpi = GetDpiForWindow(window) / 96.f;
        const bool visible = IsWindowVisible(window) && !IsIconic(window);
        const bool focused = GetAncestor(GetForegroundWindow(), GA_ROOT) == window;
        if(next.host_id == "view" && next.desktop_x == origin.x && next.desktop_y == origin.y
           && next.width == client.right && next.height == client.bottom && next.dpi_scale == dpi
           && next.visible == visible && next.focused == focused)
            return client.right > 0 && client.bottom > 0 && visible;
        ++next.revision;
        next.host_id = "view"; next.view_id = "client";
        next.desktop_x = origin.x; next.desktop_y = origin.y;
        next.width = client.right; next.height = client.bottom;
        next.dpi_scale = dpi; next.visible = visible; next.focused = focused;
        runtime.viewport(next);
        return client.right > 0 && client.bottom > 0 && visible;
    }
    void present() {
        const auto surface = renderer.render(runtime);
        if(surface.width <= 0 || surface.height <= 0) return;
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = surface.width;
        info.bmiHeader.biHeight = -surface.height; // top-down
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        HDC target = GetDC(window);
        if(!target) return;
        // The surface is premultiplied BGRA; an opaque window shows it directly.
        SetDIBitsToDevice(target, 0, 0, static_cast<DWORD>(surface.width), static_cast<DWORD>(surface.height),
            0, 0, 0, static_cast<UINT>(surface.height), surface.pixels.data(), &info, DIB_RGB_COLORS);
        ReleaseDC(window, target);
        runtime.frame_presented();
        presented = true;
    }
};

WindowsView::WindowsView(Runtime& runtime, PictorSurface& renderer, const std::string& title,
                         int width, int height, bool start_fullscreen)
    : impl_(std::make_unique<Impl>(runtime, renderer, title, width, height, start_fullscreen)) {}
WindowsView::~WindowsView() = default;

bool WindowsView::pump() {
    MSG message{};
    while(PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if(message.message == WM_QUIT) impl_->closed = true;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return !impl_->closed;
}
void WindowsView::synchronize() {
    if(!impl_->observe()) return;
    if(impl_->presented && !impl_->runtime.needs_frame()) return;
    impl_->present();
}
void WindowsView::fullscreen(bool wanted) { impl_->set_fullscreen(wanted); }
bool WindowsView::fullscreen() const noexcept { return impl_->borderless; }
std::uintptr_t WindowsView::native_window() const noexcept { return reinterpret_cast<std::uintptr_t>(impl_->window); }
}
