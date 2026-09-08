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
void OverlayDiagnostics::write(const std::string& path, std::uint64_t frames) const {
    if(path.empty()) return;
    DWORD handles{}; GetProcessHandleCount(GetCurrentProcess(),&handles);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started_).count();
    const auto cpuMs=(cpuTime()-cpu_)/10000.0;
    std::ofstream report(path); if(!report) throw std::runtime_error("Cannot open diagnostic report");
    report<<"{\"backend\":\"pictor-cpu-layered\",\"seconds\":"<<elapsed<<",\"cpu_ms\":"<<cpuMs<<",\"frames\":"<<frames<<",\"handles_before\":"<<handles_<<",\"handles_after\":"<<handles<<",\"gdi_before\":"<<gdi_<<",\"gdi_after\":"<<GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<<"}\n";
    if(!report) throw std::runtime_error("Cannot write diagnostic report");
}
