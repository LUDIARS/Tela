// @spec SPEC-TL-OVERLAY
#include "layered_composition.hpp"
#include "native_pointer.hpp"
#include <tela/windows_overlay.hpp>
#include <optional>
#include <stdexcept>

namespace tela {
struct WindowsOverlay::Impl {
    Runtime& runtime;
    PictorSurface& renderer;
    HWND target{};
    windows::NativePointer pointer;
    std::exception_ptr error;
    bool visible{};
    PresentationDiagnostics diagnostics;
    // Declared last so its windows are destroyed while everything their messages touch is alive.
    windows::LayeredComposition composition{[this](bool passthrough) { return create(passthrough); }};
    Impl(Runtime& r, PictorSurface& p, HWND t) : runtime(r), renderer(p), target(t) {
        if (!IsWindow(target)) throw std::invalid_argument("Tela target HWND does not exist");
        WNDCLASSEXW c{sizeof(c)}; c.lpfnWndProc = procedure; c.hInstance = GetModuleHandleW(nullptr);
        c.lpszClassName = L"Tela.LayeredOverlay.v1"; c.hCursor = LoadCursor(nullptr,IDC_ARROW);
        if (!RegisterClassExW(&c) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register Tela window class");
        composition.open();
    }
    ~Impl() { hide(); composition.destroy(); }
    HWND create(bool passthrough) {
        auto w = CreateWindowExW(WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|(passthrough?WS_EX_TRANSPARENT:0),
            L"Tela.LayeredOverlay.v1",L"Tela",WS_POPUP,0,0,1,1,target,nullptr,GetModuleHandleW(nullptr),this);
        if (!w) throw std::runtime_error("Cannot create Tela overlay window");
        return w;
    }
    void hide() {
        runtime.cancel();
        composition.hide();
        visible = false;
    }
    static LRESULT CALLBACK procedure(HWND w, UINT m, WPARAM wp, LPARAM lp) noexcept {
        auto self = reinterpret_cast<Impl*>(GetWindowLongPtrW(w,GWLP_USERDATA));
        if (m==WM_NCCREATE) {
            self = static_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
            SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(w,m,wp,lp);
        try { return self->message(w,m,wp,lp); }
        catch (...) { self->error = std::current_exception(); self->runtime.cancel(); return 0; }
    }
    LRESULT message(HWND w, UINT m, WPARAM wp, LPARAM lp) {
        if (m==WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        if (m==WM_CAPTURECHANGED || m==WM_CANCELMODE) runtime.cancel();
        if (windows::NativePointer::handles(m)) { pointer.forward(runtime,w,m,lp); return 0; }
        return DefWindowProcW(w,m,wp,lp);
    }
    // Returns the observation to present under, or nothing when suppressed.
    std::optional<PresentationObservation> preparePresentation() {
        if (error) std::rethrow_exception(error);
        const auto& v=runtime.viewport();
        DWORD target_pid{}, foreground_pid{};
        GetWindowThreadProcessId(target,&target_pid);
        GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
        // A newly shown overlay has no frame yet, so treat becoming visible as dirty.
        const PresentationObservation observation{v.visible,v.width>0&&v.height>0,
            IsWindow(target)!=FALSE,IsWindowVisible(target)!=FALSE,IsIconic(target)!=FALSE,
            !visible||runtime.needs_frame(),
            static_cast<std::uint32_t>(target_pid),static_cast<std::uint32_t>(foreground_pid)};
        const auto reason=presentation_reason(observation);
        if(reason!=PresentationReason::presented) {
            diagnostics.record(reason,observation);
            // hide() also cancels; an already-hidden overlay holds no capture, so
            // cancelling again would repeatedly discard gestures owned elsewhere.
            if(reason!=PresentationReason::unchanged && visible) hide();
            return std::nullopt;
        }
        if (!visible) runtime.invalidate();
        return observation;
    }
    void synchronize() {
        const auto observation=preparePresentation();
        if(!observation) return;
        composition.present(runtime,renderer.render(runtime));
        runtime.frame_presented(); visible=true;
        diagnostics.record(PresentationReason::presented,*observation);
    }
};
WindowsOverlay::WindowsOverlay(Runtime& r,PictorSurface& p,std::uintptr_t target)
    : impl_(std::make_unique<Impl>(r,p,reinterpret_cast<HWND>(target))) {}
WindowsOverlay::~WindowsOverlay()=default;
void WindowsOverlay::synchronize() { impl_->synchronize(); }
void WindowsOverlay::hide() { impl_->hide(); }
std::uintptr_t WindowsOverlay::native_window() const noexcept { return reinterpret_cast<std::uintptr_t>(impl_->composition.visual()); }
const PresentationDiagnostics& WindowsOverlay::diagnostics() const noexcept { return impl_->diagnostics; }
}
