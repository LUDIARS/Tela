#include "overlay_diagnostics.hpp"
#include <windows.h>
#include <fstream>
#include <stdexcept>
// @spec Overlay lifecycle
namespace {
std::uint64_t cpuTime(){FILETIME create{},exit{},kernel{},user{};if(!GetProcessTimes(GetCurrentProcess(),&create,&exit,&kernel,&user))return 0;ULARGE_INTEGER k{},u{};k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;return k.QuadPart+u.QuadPart;}
}
OverlayDiagnostics::OverlayDiagnostics() : started_(std::chrono::steady_clock::now()), cpu_(cpuTime()), handles_{}, gdi_(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)) {
    GetProcessHandleCount(GetCurrentProcess(),&handles_);
}
void OverlayDiagnostics::write(const std::string& path, const OverlayRunReport& result) const {
    if(path.empty()) return;
    DWORD handles{}; GetProcessHandleCount(GetCurrentProcess(),&handles);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started_).count();
    const auto cpuMs=(cpuTime()-cpu_)/10000.0;
    std::ofstream report(path); if(!report) throw std::runtime_error("Cannot open diagnostic report");
    report<<"{\"report_version\":2,\"backend\":\"pictor-cpu-layered\",\"seconds\":"<<elapsed<<",\"cpu_ms\":"<<cpuMs<<",\"frames\":"<<result.frames<<",\"handles_before\":"<<handles_<<",\"handles_after\":"<<handles<<",\"gdi_before\":"<<gdi_<<",\"gdi_after\":"<<GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    const auto& p=result.presentation;
    report<<",\"presentation\":{\"last_reason\":\""<<tela::presentation_reason_name(p.last)<<"\",\"target_process\":"<<p.observation.target_process
        <<",\"foreground_process\":"<<p.observation.foreground_process<<",\"counts\":{";
    for(unsigned i=0;i<p.counts.size();++i) {
        if(i) report<<',';
        report<<'"'<<tela::presentation_reason_name(static_cast<tela::PresentationReason>(i))<<"\":"<<p.counts[i];
    }
    report<<"}}}\n";
    if(!report) throw std::runtime_error("Cannot write diagnostic report");
}
