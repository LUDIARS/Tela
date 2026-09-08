// @spec Overlay lifecycle
#include <tela/presentation_status.hpp>
#include <stdexcept>
#include <iostream>
namespace {
void expect(tela::PresentationObservation o,tela::PresentationReason reason) {
    if(tela::presentation_reason(o)!=reason) throw std::runtime_error(tela::presentation_reason_name(reason));
}
}
int main(){try{
    using R=tela::PresentationReason;
    tela::PresentationObservation ready{true,true,true,true,false,true,42,42};
    expect(ready,R::presented);
    auto o=ready;o.dirty=false;expect(o,R::unchanged);
    o=ready;o.target_exists=false;expect(o,R::target_lost);
    o=ready;o.viewport_visible=false;expect(o,R::viewport_hidden);
    o=ready;o.nonempty=false;expect(o,R::empty_viewport);
    o=ready;o.target_visible=false;expect(o,R::target_hidden);
    o=ready;o.minimized=true;expect(o,R::target_minimized);
    o=ready;o.foreground_process=0;expect(o,R::foreground_unavailable);
    o=ready;o.target_process=0;expect(o,R::foreground_unavailable);
    o=ready;o.foreground_process=99;expect(o,R::foreign_foreground);
    // Dirty content must not bypass a hidden/foreign target gate.
    o.dirty=false;expect(o,R::foreign_foreground);
    o=ready;o.viewport_visible=false;o.target_exists=false;expect(o,R::target_lost);
    tela::PresentationDiagnostics d;d.record(R::foreign_foreground,o);d.record(R::presented,ready);
    if(d.counts[static_cast<unsigned>(R::foreign_foreground)]!=1 || d.last!=R::presented)
        throw std::runtime_error("reason history lost on restore");
    tela::PresentationDiagnostics total;total.append(d);total.append(d);
    total.append(tela::PresentationDiagnostics{});
    if(total.last!=R::presented || total.counts[static_cast<unsigned>(R::presented)]!=2)
        throw std::runtime_error("reconnection must accumulate observations without inventing new ones");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
