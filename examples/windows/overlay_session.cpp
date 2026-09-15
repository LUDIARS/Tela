#include "overlay_session.hpp"
#include "overlay_probe.hpp"
#include "probe_content.hpp"
#include "scene_overlay_content.hpp"
#include "transition_editor.hpp"
#include <tela/windows_overlay.hpp>
#include <tela/windows_pipe.hpp>
#include <tela/transitions.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
// @spec Overlay lifecycle
// @spec Unity bridge
// @spec Transition authoring
// @spec Scene overlay
namespace {
using Clock=std::chrono::steady_clock;
class OverlaySession {
public:
    explicit OverlaySession(const Options& config);
    OverlayRunReport run();
private:
    void save(tela::Transition value);
    void add(const tela::Anchor& anchor);
    void rebuildDocument();
    void startProbe();
    bool pumpMessages();
    bool expired() const;
    bool updateProbe();
    void receive();
    void receive(const tela::PipeEvent& event);
    void accept(const tela::BridgeMessage& message);
    void disconnect();
    const Options& config_;
    tela::Runtime runtime_;
    tela::PictorSurface renderer_;
    tela::Transitions data_;
    TransitionEditor editor_;
    tela::BridgeSession bridge_;
    // Declared before the overlay so input windows are released before the actions' owner.
    std::unique_ptr<SceneOverlayContent> sceneContent_;
    std::unique_ptr<tela::WindowsOverlay> overlay_;
    std::unique_ptr<tela::WindowsPipe> pipe_;
    HWND target_{};
    int clicks_{};
    ProbeContent probeContent_;
    std::uint64_t connection_{}, nextId_{};
    bool rebuild_{true};
    tela::PresentationDiagnostics presentation_;
    Clock::time_point started_{Clock::now()};
};
OverlaySession::OverlaySession(const Options& config) : config_(config),renderer_(config.font),bridge_(runtime_) {
    if(config_.probe) { startProbe(); return; }
    if(!config_.sceneOverlay.empty()) sceneContent_=std::make_unique<SceneOverlayContent>(config_.sceneOverlay);
    else if(std::filesystem::exists(config_.file)) data_.load(config_.file);
    pipe_=std::make_unique<tela::WindowsPipe>(config_.pipe);
}
void OverlaySession::save(tela::Transition value) {
    auto next=data_; next.set(std::move(value)); next.save(config_.file);
    data_=std::move(next); rebuild_=true;
}
void OverlaySession::add(const tela::Anchor& anchor) {
    std::string id;
    do { id="transition-"+std::to_string(++nextId_); }
    while(std::any_of(data_.entries().begin(),data_.entries().end(),[&](const auto& t){return t.id==id;}));
    editor_.open({id,"Screen","Destination","",anchor.object_id},[this](auto value){save(std::move(value));});
}
void OverlaySession::rebuildDocument() {
    if(config_.probe) return;
    // Scene overlay content compares its own declaration inputs, including viewport changes.
    if(sceneContent_) { sceneContent_->refresh(runtime_); return; }
    if(!rebuild_) return;
    runtime_.document(tela::transition_document(data_,bridge_.anchors(),runtime_.viewport(),
        [this](const auto& t){editor_.open(t,[this](auto value){save(std::move(value));});},
        [this](const auto& a){add(a);}));
    rebuild_=false;
}
void OverlaySession::startProbe() {
    target_=FindWindowW(L"Tela.ProbeTarget",nullptr);
    if(!target_) throw std::runtime_error("Start tela-probe-target through Excubitor first");
    synchronizeProbe(runtime_,target_); runtime_.document(probeDocument(clicks_,runtime_));
    overlay_=std::make_unique<tela::WindowsOverlay>(runtime_,renderer_,reinterpret_cast<std::uintptr_t>(target_));
}
bool OverlaySession::pumpMessages() {
    HANDLE wake=pipe_?reinterpret_cast<HANDLE>(pipe_->wake_handle()):nullptr;
    // Pipe delivery and native messages wake immediately; only the probe polls geometry.
    MsgWaitForMultipleObjects(wake?1:0,wake?&wake:nullptr,FALSE,config_.probe?100:1000,QS_ALLINPUT);
    bool running=true; MSG message{};
    while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
        if(message.message==WM_QUIT) running=false;
        TranslateMessage(&message); DispatchMessageW(&message);
    }
    return running;
}
bool OverlaySession::expired() const {
    return config_.seconds && Clock::now()-started_>=std::chrono::seconds(config_.seconds);
}
bool OverlaySession::updateProbe() {
    if(!config_.probe) return true;
    return probeContent_.refresh_from_target(runtime_,reinterpret_cast<std::uintptr_t>(target_),clicks_);
}
void OverlaySession::disconnect() {
    if(overlay_) presentation_.append(overlay_->diagnostics());
    bridge_.disconnect(); overlay_.reset(); connection_=0;
}
void OverlaySession::accept(const tela::BridgeMessage& message) {
    if(!bridge_.accept(message)) return;
    switch(message.kind) {
    case tela::BridgeKind::hello:
        overlay_=std::make_unique<tela::WindowsOverlay>(runtime_,renderer_,bridge_.host_window()); break;
    case tela::BridgeKind::anchors:
    case tela::BridgeKind::selection:
    case tela::BridgeKind::viewport: rebuild_=true; break;
    default: break;
    }
}
void OverlaySession::receive(const tela::PipeEvent& event) {
    if(!event.message) {
        disconnect();
        if(!event.error.empty()) std::cerr<<"Tela IPC: "<<event.error<<'\n';
        return;
    }
    if(event.connection!=connection_) { disconnect(); connection_=event.connection; }
    try { accept(*event.message); }
    catch(const std::exception& e) { disconnect(); std::cerr<<"Tela rejected IPC: "<<e.what()<<'\n'; }
}
void OverlaySession::receive() {
    if(!pipe_) return;
    for(const auto& event:pipe_->drain()) receive(event);
}
OverlayRunReport OverlaySession::run() {
    while(pumpMessages()) {
        if(expired()) break;
        if(!updateProbe()) {
            if(overlay_) overlay_->synchronize(); // record target loss before teardown
            break;
        }
        receive(); rebuildDocument();
        if(overlay_) overlay_->synchronize();
    }
    if(overlay_) presentation_.append(overlay_->diagnostics());
    return {runtime_.frames(),presentation_};
}
}
OverlayRunReport runOverlay(const Options& options) { return OverlaySession(options).run(); }
