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
// Editing keys a host forwards to the focused field. Typed characters go
// through Runtime::text instead (including an input method's committed text).
enum class EditKey { backspace, enter, escape, tab, back_tab };
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
    // Field focus. Empty when no field has it.
    const std::string& focused() const noexcept { return focused_; }
    void focus(const std::string& id);
    void blur();
    // Appends committed text to the focused field. Control characters are
    // dropped. False when no field is focused (the host may use the input).
    bool text(const std::string& utf8);
    // Backspace removes the last character, Enter keeps the text and leaves
    // the field, Escape restores the text from when the field was focused,
    // Tab / Shift+Tab move to the next / previous field. False when no field
    // is focused (Escape may then close the host window).
    bool key(EditKey key);
private:
    const PlacedElement* target(float x, float y) const;
    const PlacedElement* placed(const std::string& id) const;
    void edit_focused(const std::string& value);
    void layout();
    Document document_;
    Theme theme_;
    Viewport viewport_;
    std::vector<PlacedElement> placed_;
    std::unordered_map<std::string, ElementState> states_;
    GestureOwnership ownership_;
    std::uint64_t frames_{};
    bool dirty_{true};
    std::string focused_;
    std::string focus_original_;  // restored by Escape
};
}
