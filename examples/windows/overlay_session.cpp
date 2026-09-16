#include "overlay_session.hpp"
#include "attached_viewport.hpp"
#include "overlay_probe.hpp"
#include "probe_content.hpp"
#include "scene_overlay_content.hpp"
#include "graph_content.hpp"
#include "spec_view_content.hpp"
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
// @spec Spec view
// @spec Overlay placement
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
    void applyTheme();
    HWND findProbeTarget();
    void startProbe();
    void attachTarget();
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
    std::unique_ptr<SpecViewContent> specContent_;
    std::unique_ptr<GraphContent> graphContent_;
    std::unique_ptr<tela::WindowsOverlay> overlay_;
    std::unique_ptr<tela::WindowsPipe> pipe_;
    // The read-only content's own size, used when the viewport sits outside the host.
    tela::Rect contentBounds_{};
    HWND target_{};
    int clicks_{};
    ProbeContent probeContent_;
    std::uint64_t connection_{}, nextId_{};
    bool rebuild_{true};
    tela::PresentationDiagnostics presentation_;
    Clock::time_point started_{Clock::now()};
};
OverlaySession::OverlaySession(const Options& config) : config_(config),renderer_(config.font),bridge_(runtime_) {
    applyTheme();
    if(config_.probe) { startProbe(); return; }
    if(!config_.graph.empty()) {
        graphContent_=std::make_unique<GraphContent>(config_.graph);
        contentBounds_=graphContent_->natural_bounds();
    } else if(!config_.specView.empty()) {
        specContent_=std::make_unique<SpecViewContent>(config_.specView);
        contentBounds_=specContent_->natural_bounds();
    } else if(!config_.sceneOverlay.empty()) {
        sceneContent_=std::make_unique<SceneOverlayContent>(config_.sceneOverlay);
        contentBounds_=sceneContent_->natural_bounds();
    } else if(std::filesystem::exists(config_.file)) data_.load(config_.file);
    // A read-only view can sit on the separate probe target window instead of waiting for Unity.
    if(config_.attachProbeTarget) { attachTarget(); return; }
    pipe_=std::make_unique<tela::WindowsPipe>(config_.pipe);
}
void OverlaySession::applyTheme() {
    if(!config_.fontSize) return;
    auto theme=runtime_.theme();
    const float size=static_cast<float>(config_.fontSize);
    // The default theme pairs 16 px text with a 24 px line; keep that ratio at any size.
    theme.line_height=size*(theme.font_size>0?theme.line_height/theme.font_size:1.5f);
    theme.font_size=size;
    runtime_.theme(theme);
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
    // Read-only content compares its own declaration inputs, including viewport changes.
    if(graphContent_) { graphContent_->refresh(runtime_); return; }
    if(specContent_) { specContent_->refresh(runtime_); return; }
    if(sceneContent_) { sceneContent_->refresh(runtime_); return; }
    if(!rebuild_) return;
    runtime_.document(tela::transition_document(data_,bridge_.anchors(),runtime_.viewport(),
        [this](const auto& t){editor_.open(t,[this](auto value){save(std::move(value));});},
        [this](const auto& a){add(a);}));
    rebuild_=false;
}
HWND OverlaySession::findProbeTarget() {
    HWND window=FindWindowW(L"Tela.ProbeTarget",nullptr);
    if(!window) throw std::runtime_error("Start tela-probe-target through Excubitor first");
    return window;
}
void OverlaySession::startProbe() {
    target_=findProbeTarget();
    synchronizeProbe(runtime_,target_); runtime_.document(probeDocument(clicks_,runtime_));
    overlay_=std::make_unique<tela::WindowsOverlay>(runtime_,renderer_,reinterpret_cast<std::uintptr_t>(target_));
}
void OverlaySession::attachTarget() {
    target_=findProbeTarget();
    synchronizeAttached(runtime_,target_,config_.placement,contentBounds_.width,contentBounds_.height);
    rebuildDocument();
    overlay_=std::make_unique<tela::WindowsOverlay>(runtime_,renderer_,reinterpret_cast<std::uintptr_t>(target_));
}
bool OverlaySession::pumpMessages() {
    HANDLE wake=pipe_?reinterpret_cast<HANDLE>(pipe_->wake_handle()):nullptr;
    // Pipe delivery and native messages wake immediately; only a tracked window polls geometry.
    const bool polling=config_.probe||config_.attachProbeTarget;
    MsgWaitForMultipleObjects(wake?1:0,wake?&wake:nullptr,FALSE,polling?100:1000,QS_ALLINPUT);
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
    if(config_.probe) return probeContent_.refresh_from_target(runtime_,reinterpret_cast<std::uintptr_t>(target_),clicks_);
    if(!target_) return true;
    // Attached content tracks the same standalone window but keeps its own declaration.
    synchronizeAttached(runtime_,target_,config_.placement,contentBounds_.width,contentBounds_.height);
    return IsWindow(target_)!=FALSE;
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
