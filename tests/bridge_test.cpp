// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
#include <tela/bridge.hpp>
#include <stdexcept>
#include <iostream>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(){try{
    tela::BridgeMessage hello;hello.kind=tela::BridgeKind::hello;hello.generation=77;hello.sequence=1;hello.host="Unity";hello.view="Scene";hello.host_window=123;
    auto bytes=tela::encode_bridge(hello);auto decoded=tela::decode_bridge(bytes);require(decoded.host_window==123&&decoded.generation==77,"hello round trip");
    for(size_t i=0;i<bytes.size();++i){try{tela::decode_bridge(std::span(bytes).first(i));throw std::runtime_error("truncated message accepted");}catch(const std::invalid_argument&){}}
    bytes.push_back(0);try{tela::decode_bridge(bytes);return 2;}catch(const std::invalid_argument&){}
    tela::Runtime r;tela::BridgeSession session(r);require(session.accept(decoded),"hello accepted");require(!session.accept(decoded),"hello cannot reset live generation");
    auto view=hello;view.kind=tela::BridgeKind::viewport;view.sequence=2;view.revision=5;view.viewport={"Unity","Scene",5,-1200,-200,800,600,1.5,true,true};
    auto round=tela::decode_bridge(tela::encode_bridge(view));require(round.viewport.desktop_x==-1200&&round.viewport.dpi_scale==1.5f,"physical negative origin and DPI");require(session.accept(round),"viewport accepted");
    auto selection=hello;selection.kind=tela::BridgeKind::selection;selection.sequence=3;selection.revision=5;selection.selection={"GlobalObjectId-V1-2"};require(session.accept(selection),"selection accepted");require(!session.accept(selection),"duplicate dropped");
    selection.sequence=4;selection.revision=4;require(!session.accept(selection),"stale revision dropped");
    selection.sequence=5;selection.generation=78;require(!session.accept(selection),"foreign generation dropped");
    session.disconnect();require(!r.viewport().visible&&session.selection().empty(),"disconnect clears view");require(!session.accept(selection),"no input before hello");
    hello.generation=78;require(session.accept(hello),"new connection can establish new generation");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
