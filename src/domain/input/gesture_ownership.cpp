// @spec SPEC-TL-INPUT
#include <tela/input_ownership.hpp>

namespace tela {
std::string GestureOwnership::cancel(){
    if(!gesture_)return {};auto id=gesture_->id;gesture_.reset();return id;
}
InputDecision GestureOwnership::apply(const HostPointerEvent& event,InputSource source,
    std::uint64_t revision,bool available,const InputTarget& target){
    InputDecision result;
    auto& sequence=source==InputSource::native?native_sequence_:host_sequence_;
    if(event.sequence<=sequence||event.viewport_revision!=revision)return result;
    sequence=event.sequence;
    if(event.phase==PointerPhase::cancel){
        if(gesture_&&gesture_->source==source&&gesture_->token==event.gesture_id)result.released=cancel();
        return result;
    }
    if(!available)return result;
    if(event.phase==PointerPhase::down){
        if(gesture_||target.id.empty()||event.button!=PointerButton::primary)return result;
        if((source==InputSource::native&&target.policy!=InputPolicy::exclusive)||
           (source==InputSource::host_observation&&target.policy!=InputPolicy::shared))return result;
        gesture_=Gesture{target.id,source,event.gesture_id,event.button};result.pressed=target.id;
    }else if(gesture_&&gesture_->source==source&&gesture_->token==event.gesture_id&&
        event.phase==PointerPhase::up&&event.button==gesture_->button){
        if(target.id==gesture_->id&&target.actionable)result.activated=target.id;
        result.released=cancel();
    }
    result.consumed=source==InputSource::native&&(!result.pressed.empty()||!result.released.empty()||
        (gesture_&&gesture_->source==source));
    return result;
}
}
