// @spec SPEC-TL-OVERLAY
#include "layered_bitmap.hpp"
#include <tela/windows_overlay.hpp>
#include <tela/input_regions.hpp>
#include <windowsx.h>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace tela {
struct WindowsOverlay::Impl {
    Runtime& runtime;
    PictorSurface& renderer;
    HWND target{}, visual{};
    std::unordered_map<std::string,HWND> regions;
    std::uint64_t sequence{}, gesture{};
    std::exception_ptr error;
    bool visible{};
    PresentationDiagnostics diagnostics;
    Impl(Runtime& r, PictorSurface& p, HWND t) : runtime(r), renderer(p), target(t) {
        if (!IsWindow(target)) throw std::invalid_argument("Tela target HWND does not exist");
        WNDCLASSEXW c{sizeof(c)}; c.lpfnWndProc = procedure; c.hInstance = GetModuleHandleW(nullptr);
        c.lpszClassName = L"Tela.LayeredOverlay.v1"; c.hCursor = LoadCursor(nullptr,IDC_ARROW);
        if (!RegisterClassExW(&c) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register Tela window class");
        visual = create(true);
    }
    ~Impl() { hide(); for (auto [id,w] : regions) DestroyWindow(w); if (visual) DestroyWindow(visual); }
    HWND create(bool passthrough) {
        auto w = CreateWindowExW(WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|(passthrough?WS_EX_TRANSPARENT:0),
            L"Tela.LayeredOverlay.v1",L"Tela",WS_POPUP,0,0,1,1,target,nullptr,GetModuleHandleW(nullptr),this);
        if (!w) throw std::runtime_error("Cannot create Tela overlay window");
        return w;
    }
    void hide() {
        runtime.cancel();
        if (GetCapture()==visual) ReleaseCapture();
        for (auto [id,w] : regions) { if (GetCapture()==w) ReleaseCapture(); ShowWindow(w,SW_HIDE); }
        if (visual) ShowWindow(visual,SW_HIDE);
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
        if (m==WM_LBUTTONDOWN || m==WM_LBUTTONUP || m==WM_MOUSEMOVE) {
            POINT pos{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)}; ClientToScreen(w,&pos);
            if (m==WM_LBUTTONDOWN) ++gesture;
            HostPointerEvent e;
            e.sequence=++sequence; e.viewport_revision=runtime.viewport().revision; e.gesture_id=gesture;
            e.phase=m==WM_LBUTTONDOWN?PointerPhase::down:m==WM_LBUTTONUP?PointerPhase::up:PointerPhase::move;
            e.button=m==WM_MOUSEMOVE?PointerButton::none:PointerButton::primary;
            e.desktop_x=pos.x; e.desktop_y=pos.y;
            runtime.pointer(e,InputSource::native);
            if (m==WM_LBUTTONDOWN && runtime.captured()) SetCapture(w);
            if (m==WM_LBUTTONUP && GetCapture()==w) ReleaseCapture();
            return 0;
        }
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
        const auto& v=runtime.viewport();
        const auto surface=renderer.render(runtime);
        auto visualSurface=surface;
        std::unordered_map<std::string,bool> keep;
        for (const auto& e:exclusive_regions(runtime.elements())) {
            const auto rect=intersect(e.bounds,{0,0,v.width/v.dpi_scale,v.height/v.dpi_scale});
            const int x=std::clamp(static_cast<int>(std::floor(rect.x*v.dpi_scale)),0,v.width);
            const int y=std::clamp(static_cast<int>(std::floor(rect.y*v.dpi_scale)),0,v.height);
            const int width=std::min(v.width-x,static_cast<int>(std::ceil(rect.width*v.dpi_scale)));
            const int height=std::min(v.height-y,static_cast<int>(std::ceil(rect.height*v.dpi_scale)));
            if (width<=0 || height<=0) continue;
            auto it=regions.find(e.id);
            if (it==regions.end()) it=regions.emplace(e.id,create(false)).first;
            keep[e.id]=true;
            // The region window carries the final composited pixels for its
            // fragments; SetWindowRgn below clips it to exactly those fragments,
            // and the same rects are cleared from visualSurface so Windows never
            // blends a pixel twice.
            windows::present(it->second,surface,v.desktop_x+x,v.desktop_y+y,x,y,width,height,true);
            HRGN mask=CreateRectRgn(0,0,0,0);
            if(!mask)throw std::runtime_error("Cannot allocate hit region");
            for(auto fragment:e.fragments){
                const int left=std::clamp(static_cast<int>(std::ceil(fragment.x*v.dpi_scale)),x,x+width);
                const int top=std::clamp(static_cast<int>(std::ceil(fragment.y*v.dpi_scale)),y,y+height);
                const int right=std::clamp(static_cast<int>(std::ceil((fragment.x+fragment.width)*v.dpi_scale)),left,x+width);
                const int bottom=std::clamp(static_cast<int>(std::ceil((fragment.y+fragment.height)*v.dpi_scale)),top,y+height);
                HRGN part=CreateRectRgn(left-x,top-y,right-x,bottom-y);
                if(!part){DeleteObject(mask);throw std::runtime_error("Cannot allocate hit fragment");}
                const auto merged=CombineRgn(mask,mask,part,RGN_OR);DeleteObject(part);
                if(merged==ERROR){DeleteObject(mask);throw std::runtime_error("Cannot combine hit fragments");}
                for(int row=top;row<bottom;++row)
                    std::fill_n(visualSurface.pixels.begin()+(static_cast<size_t>(row)*v.width+left)*4,static_cast<size_t>(right-left)*4,0);
            }
            // Windows owns mask after a successful SetWindowRgn.
            if(!SetWindowRgn(it->second,mask,FALSE)){DeleteObject(mask);throw std::runtime_error("Cannot apply hit mask");}
            SetWindowPos(it->second,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
        }
        for (auto it=regions.begin();it!=regions.end();) {
            if (!keep.contains(it->first)) {
                if (GetCapture()==it->second) { runtime.cancel(); ReleaseCapture(); }
                DestroyWindow(it->second); it=regions.erase(it);
            } else ++it;
        }
        windows::present(visual,visualSurface,v.desktop_x,v.desktop_y);
        SetWindowPos(visual,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
        runtime.frame_presented(); visible=true;
        diagnostics.record(PresentationReason::presented,*observation);
    }
};
WindowsOverlay::WindowsOverlay(Runtime& r,PictorSurface& p,std::uintptr_t target)
    : impl_(std::make_unique<Impl>(r,p,reinterpret_cast<HWND>(target))) {}
WindowsOverlay::~WindowsOverlay()=default;
void WindowsOverlay::synchronize() { impl_->synchronize(); }
void WindowsOverlay::hide() { impl_->hide(); }
std::uintptr_t WindowsOverlay::native_window() const noexcept { return reinterpret_cast<std::uintptr_t>(impl_->visual); }
const PresentationDiagnostics& WindowsOverlay::diagnostics() const noexcept { return impl_->diagnostics; }
}
