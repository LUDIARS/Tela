// @implements SPEC-TL-OVERLAY
// @spec Overlay lifecycle
#include <windows.h>
#include <string>
#include <iostream>

namespace {
unsigned clicks{};
LRESULT CALLBACK procedure(HWND w,UINT m,WPARAM wp,LPARAM lp){
    if(m==WM_LBUTTONDOWN){++clicks;InvalidateRect(w,nullptr,TRUE);std::cout<<"host_clicks="<<clicks<<std::endl;return 0;}
    if(m==WM_KEYDOWN&&wp==VK_ESCAPE){DestroyWindow(w);return 0;}
    if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
    if(m==WM_PAINT){PAINTSTRUCT ps{};HDC dc=BeginPaint(w,&ps);RECT r{};GetClientRect(w,&r);FillRect(dc,&r,reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
        SetBkMode(dc,TRANSPARENT);const auto text=L"Separate host process - clicks: "+std::to_wstring(clicks);TextOutW(dc,30,280,text.c_str(),static_cast<int>(text.size()));
        for(int x=0;x<r.right;x+=40){MoveToEx(dc,x,0,nullptr);LineTo(dc,x,r.bottom);}EndPaint(w,&ps);return 0;}
    return DefWindowProcW(w,m,wp,lp);
}
}
int main(){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSW c{};c.lpfnWndProc=procedure;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"Tela.ProbeTarget";c.hCursor=LoadCursor(nullptr,IDC_ARROW);
    if(!RegisterClassW(&c))return 1;
    HWND w=CreateWindowW(c.lpszClassName,L"Tela composition probe - separate host",WS_OVERLAPPEDWINDOW,100,100,850,550,nullptr,nullptr,c.hInstance,nullptr);
    if(!w)return 2;ShowWindow(w,SW_SHOW);SetForegroundWindow(w);
    MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}return 0;
}
