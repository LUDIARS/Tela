// @implements SPEC-TL-TRANSITIONS
// @spec Transition authoring
#include "transition_editor.hpp"
#include <windows.h>
#include <stdexcept>

namespace {
std::wstring wide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(!n&&!s.empty())throw std::invalid_argument("Invalid UTF-8");std::wstring w(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;}
std::string text(HWND h){int n=GetWindowTextLengthW(h);std::wstring w(n+1,L'\0');GetWindowTextW(h,w.data(),n+1);int count=WideCharToMultiByte(CP_UTF8,0,w.data(),n,nullptr,0,nullptr,nullptr);std::string s(count,'\0');WideCharToMultiByte(CP_UTF8,0,w.data(),n,s.data(),count,nullptr,nullptr);return s;}
}
struct TransitionEditor::Impl {
    HWND window{}, source{}, destination{}, condition{};
    tela::Transition draft;
    std::function<void(tela::Transition)> save;
    ~Impl(){if(window)DestroyWindow(window);}
    static LRESULT CALLBACK procedure(HWND w,UINT m,WPARAM wp,LPARAM lp) noexcept {
        auto self=reinterpret_cast<Impl*>(GetWindowLongPtrW(w,GWLP_USERDATA));
        if(m==WM_NCCREATE){self=static_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
        if(!self)return DefWindowProcW(w,m,wp,lp);
        if(m==WM_CLOSE){ShowWindow(w,SW_HIDE);return 0;}
        if(m==WM_COMMAND && LOWORD(wp)==1){
            try{auto value=self->draft;value.source=text(self->source);value.destination=text(self->destination);value.condition=text(self->condition);self->save(std::move(value));ShowWindow(w,SW_HIDE);}
            catch(const std::exception& e){MessageBoxA(w,e.what(),"Tela save failed",MB_OK|MB_ICONERROR);}return 0;
        }
        return DefWindowProcW(w,m,wp,lp);
    }
    void open(tela::Transition value,std::function<void(tela::Transition)> callback){
        draft=std::move(value);save=std::move(callback);
        if(!window){
            WNDCLASSW c{};c.lpfnWndProc=procedure;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"Tela.TransitionEditor";c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);c.hCursor=LoadCursor(nullptr,IDC_ARROW);
            if(!RegisterClassW(&c)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Cannot register transition editor");
            window=CreateWindowW(c.lpszClassName,L"Tela - Edit transition",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, CW_USEDEFAULT,CW_USEDEFAULT,500,255,nullptr,nullptr,c.hInstance,this);
            if(!window)throw std::runtime_error("Cannot create transition editor");
            HWND* fields[]{&source,&destination,&condition};const wchar_t* labels[]{L"Source screen",L"Destination",L"Condition"};
            for(int i=0;i<3;++i){CreateWindowW(L"STATIC",labels[i],WS_CHILD|WS_VISIBLE,15,20+i*45,110,25,window,nullptr,c.hInstance,nullptr);*fields[i]=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,130,18+i*45,330,28,window,nullptr,c.hInstance,nullptr);SendMessageW(*fields[i],EM_SETLIMITTEXT,1024,0);}
            CreateWindowW(L"BUTTON",L"Save",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,350,160,110,30,window,reinterpret_cast<HMENU>(1),c.hInstance,nullptr);
        }
        SetWindowTextW(source,wide(draft.source).c_str());SetWindowTextW(destination,wide(draft.destination).c_str());SetWindowTextW(condition,wide(draft.condition).c_str());
        ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(source);
    }
};
TransitionEditor::TransitionEditor():impl_(std::make_unique<Impl>()){}
TransitionEditor::~TransitionEditor()=default;
void TransitionEditor::open(tela::Transition t,std::function<void(tela::Transition)> save){impl_->open(std::move(t),std::move(save));}
