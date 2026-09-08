// @implements SPEC-TL-RUNTIME
// @spec Runtime
#pragma once
#include <tela/layout.hpp>
#include <tela/host_contract.hpp>
#include <tela/input_ownership.hpp>
#include <optional>
#include <unordered_map>

namespace tela {
struct ElementState { bool pressed{}; std::uint64_t activations{}; };
// Single UI-thread owner; adapters enqueue cross-thread messages before calling.
class Runtime {
public:
    void document(Document document);
    void theme(Theme theme);
    void viewport(Viewport viewport);
    void disconnect();
    void invalidate() noexcept { dirty_ = true; }
    bool needs_frame() const noexcept { return viewport_.visible && dirty_; }
    void frame_presented() noexcept { dirty_ = false; ++frames_; }
    std::uint64_t frames() const noexcept { return frames_; }
    const std::vector<PlacedElement>& elements() const noexcept { return placed_; }
    const Theme& theme() const noexcept { return theme_; }
    const Viewport& viewport() const noexcept { return viewport_; }
    const ElementState* state(const std::string& id) const;
    InputPolicy hit(float x, float y) const;
    bool pointer(const HostPointerEvent&, InputSource);
    void cancel();
    bool captured() const { return ownership_.active().has_value(); }
private:
    const PlacedElement* target(float x, float y) const;
    void layout();
    Document document_;
    Theme theme_;
    Viewport viewport_;
    std::vector<PlacedElement> placed_;
    std::unordered_map<std::string, ElementState> states_;
    GestureOwnership ownership_;
    std::uint64_t frames_{};
    bool dirty_{true};
};
}
