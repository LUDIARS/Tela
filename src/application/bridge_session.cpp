// @spec SPEC-TL-BRIDGE
#include <tela/bridge.hpp>

namespace tela {
bool BridgeSession::accept(const BridgeMessage& m) {
    if(m.kind==BridgeKind::hello) {
        if(generation_)return false; // only a new transport connection can start a generation
        generation_=m.generation;sequence_=m.sequence;host_=m.host;view_=m.view;window_=static_cast<std::uintptr_t>(m.host_window);
        return true;
    }
    if(!generation_ || m.generation!=generation_ || m.host!=host_ || m.view!=view_ || m.sequence<=sequence_)return false;
    sequence_=m.sequence;
    if(m.kind==BridgeKind::viewport) { runtime_.viewport(m.viewport);return true; }
    if(m.revision!=runtime_.viewport().revision)return false;
    switch(m.kind) {
    case BridgeKind::pointer: runtime_.pointer(m.pointer,InputSource::host_observation);break;
    case BridgeKind::selection: selection_=m.selection;break;
    case BridgeKind::anchors: anchors_=m.anchors;break;
    default:break;
    }
    return true;
}
void BridgeSession::disconnect() {
    runtime_.disconnect();generation_=sequence_=window_=0;host_.clear();view_.clear();anchors_.clear();selection_.clear();
}
}
