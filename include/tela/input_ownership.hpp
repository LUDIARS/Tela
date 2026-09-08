// @implements SPEC-TL-INPUT
// @spec Input ownership
#pragma once
#include <tela/document.hpp>
#include <tela/host_contract.hpp>
#include <optional>

namespace tela {
enum class InputSource { native, host_observation };
struct InputTarget { std::string id; InputPolicy policy{InputPolicy::passthrough}; bool actionable{}; };
struct Gesture { std::string id; InputSource source; std::uint64_t token; PointerButton button; };
struct InputDecision { std::string pressed, released, activated; bool consumed{}; };
// Domain aggregate: one physical gesture has one owner until release/cancel.
// It does not know windows, callbacks, draw commands or mutable widget state.
class GestureOwnership {
public:
    InputDecision apply(const HostPointerEvent&,InputSource,std::uint64_t revision,bool available,const InputTarget&);
    std::optional<Gesture> active() const { return gesture_; }
    std::string cancel();
    void reset() { gesture_.reset();host_sequence_=native_sequence_=0; }
private:
    std::optional<Gesture> gesture_;
    std::uint64_t host_sequence_{},native_sequence_{};
};
}
