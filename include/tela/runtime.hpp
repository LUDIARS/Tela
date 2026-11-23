// @implements SPEC-TL-RUNTIME
// @spec Runtime
#pragma once
#include <tela/layout.hpp>
#include <tela/host_contract.hpp>
#include <tela/input_ownership.hpp>
#include <tela/field_geometry.hpp>
#include <optional>
#include <unordered_map>

namespace tela {
// Display state kept by stable ID across redeclarations. Values stay with the application.
struct ElementState {
    bool pressed{};
    std::uint64_t activations{};
    std::size_t caret{}, anchor{}; // UTF-8 byte offsets into the declared value
    float scroll{};                // scroll containers, logical pixels
};
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
    // Keyboard input goes to the focused control. Key and text events share one sequence.
    bool key(const HostKeyEvent&);
    bool text(const HostTextEvent&);
    // Focus moves by clicking a control, Tab / Shift+Tab, or these calls.
    bool focus(const std::string& id);
    void blur();
    const std::string& focused() const noexcept { return focused_; }
    // Width of text in the host's font, used to place the caret from a click.
    void text_measure(TextMeasure measure);
    const TextMeasure& text_measure() const noexcept { return measure_; }
private:
    const PlacedElement* target(float x, float y) const;
    const PlacedElement* placed(const std::string& id) const;
    const Element* declared(const std::string& id) const;
    static bool focusable(const Element&) noexcept;
    void layout();
    void track(const std::string& id, Point point, bool start, std::uint32_t modifiers);
    bool wheel(const HostPointerEvent&, Point point);
    bool step_focus(int direction);
    void reveal(const std::string& id);
    bool edit_key(const Element&, const HostKeyEvent&);
    bool slider_key(const Element&, Key);
    void change(const Element&, text_edit::Edit);
    void set_fraction(const Element&, float fraction);
    Document document_;
    Theme theme_;
    Viewport viewport_;
    std::vector<PlacedElement> placed_;
    std::unordered_map<std::string, ElementState> states_;
    GestureOwnership ownership_;
    std::string focused_;
    TextMeasure measure_{approximate_text_width};
    std::uint64_t frames_{}, key_sequence_{};
    bool dirty_{true};
};
}
