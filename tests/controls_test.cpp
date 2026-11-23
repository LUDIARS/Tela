// @implements SPEC-TL-CONTROLS
// @spec Controls
#include <tela/runtime.hpp>
#include <tela/control_visual.hpp>
#include <variant>
#include <tela/input_regions.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template <typename F> void rejects(F declare, const char* message) {
    try { tela::Document d; declare(d); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error(message);
}
// Fixed advance so caret positions are exact in the test.
float fixed(std::string_view text, float size) { return static_cast<float>(tela::text_edit::characters(text, text.size())) * 10 * size / 16; }

// The application side: owns the values and redeclares after every change, like a real tool.
struct App {
    tela::Runtime runtime;
    std::string name, secret = "pw", notes = "one\ntwo\nthree";
    float volume = .5f;
    int picked = -1, submitted = 0, changes = 0;
    int rows = 30;
    App() {
        runtime.viewport({"host", "view", 1, 0, 0, 400, 600, 1, true, true});
        runtime.text_measure(fixed);
        declare();
    }
    void declare() {
        tela::Document d;
        d.text_field("name", name, {.placeholder = "Name",
            .on_change = [this](const std::string& v) { name = v; ++changes; declare(); },
            .on_submit = [this] { ++submitted; }}, {.width = 200, .padding = 4, .positioned = true});
        d.text_field("secret", secret, {.secret = true,
            .on_change = [this](const std::string& v) { secret = v; declare(); }},
            {.width = 200, .padding = 4, .positioned = true, .y = 40});
        d.text_area("notes", notes, [this](const std::string& v) { notes = v; declare(); },
            {.width = 200, .padding = 4, .positioned = true, .y = 80, .lines = 2});
        d.slider("volume", volume, [this](float v) { volume = v; declare(); },
            {.width = 208, .padding = 4, .positioned = true, .y = 150});
        d.separator("rule", {.positioned = true, .y = 190});
        d.status("ok", "Ready", tela::Tone::good, {.positioned = true, .y = 210});
        d.scroll("list", [&] {
            for (int i = 0; i < rows; ++i)
                d.list_item("row-" + std::to_string(i), "Row " + std::to_string(i), "", i == picked,
                            [this, i] { picked = i; declare(); }, {.height = 20, .padding = 2});
        }, {.height = 100, .padding = 0, .gap = 0, .positioned = true, .y = 260});
        d.text_field("inside", "", {.on_change = [](const std::string&) {}},
                     {.width = 100, .padding = 4, .positioned = true, .y = 400});
        runtime.document(std::move(d));
    }
    const tela::PlacedElement& at(const std::string& id) const {
        const auto& all = runtime.elements();
        return *std::find_if(all.begin(), all.end(), [&](const auto& p) { return p.element.id == id; });
    }
    std::uint64_t pointer_sequence = 0, gesture = 0, keys = 0;
    void press(float x, float y, std::uint32_t modifiers = 0) { pointer(tela::PointerPhase::down, x, y, modifiers); }
    void release(float x, float y) { pointer(tela::PointerPhase::up, x, y); }
    void click(float x, float y) { press(x, y); release(x, y); }
    void move(float x, float y) { pointer(tela::PointerPhase::move, x, y); }
    void pointer(tela::PointerPhase phase, float x, float y, std::uint32_t modifiers = 0) {
        tela::HostPointerEvent e;
        e.sequence = ++pointer_sequence; e.viewport_revision = 1;
        if (phase == tela::PointerPhase::down) ++gesture;
        e.gesture_id = gesture; e.phase = phase;
        e.button = phase == tela::PointerPhase::move ? tela::PointerButton::none : tela::PointerButton::primary;
        e.desktop_x = static_cast<int>(x); e.desktop_y = static_cast<int>(y); e.modifiers = modifiers;
        runtime.pointer(e, tela::InputSource::native);
    }
    bool wheel(float x, float y, float notches) {
        tela::HostPointerEvent e;
        e.sequence = ++pointer_sequence; e.viewport_revision = 1; e.phase = tela::PointerPhase::wheel;
        e.desktop_x = static_cast<int>(x); e.desktop_y = static_cast<int>(y); e.wheel_y = notches;
        return runtime.pointer(e, tela::InputSource::native);
    }
    bool key(tela::Key k, std::uint32_t modifiers = 0) { return runtime.key({++keys, k, true, modifiers}); }
    bool type(const std::string& text) { return runtime.text({++keys, text}); }
    const tela::ElementState& state(const std::string& id) const { return *runtime.state(id); }
};
}

int main() { try {
    // Declaration: defaults and bounds.
    {
        tela::Document d;
        d.text_field("f", "x", {});
        d.slider("s", 3.f, {});
        d.separator("line");
        d.status("st", "ok", tela::Tone::bad);
        d.video("v", "Remote", "stream-1");
        const auto& e = d.elements();
        require(e[0].kind == tela::ElementKind::text_field && e[0].input == tela::InputPolicy::exclusive, "inputs default to exclusive");
        require(e[1].value.fraction == 1, "slider fraction is clamped to [0,1]");
        require(e[2].input == tela::InputPolicy::passthrough && e[3].input == tela::InputPolicy::passthrough, "marks are draw-only");
        require(e[3].value.tone == tela::Tone::bad && e[4].value.stream == "stream-1", "status tone and video stream are kept");
        rejects([](auto& d) { d.text_field("f", std::string(16385, 'a'), {}); }, "oversized value rejected");
        rejects([](auto& d) { d.text_field("f", "\xC3", {}); }, "invalid UTF-8 value rejected");
        rejects([](auto& d) { d.text_field("f", "a\nb", {}); }, "text field is a single line");
        rejects([](auto& d) { d.slider("s", NAN, {}); }, "non-finite slider rejected");
        rejects([](auto& d) { d.video("v", "", std::string(513, 'k')); }, "oversized stream key rejected");
        rejects([](auto& d) { d.scroll("s", {}); }, "scroll requires a declaration");
    }
    // Layout: separator, scroll clip, offset clamp, fill height.
    {
        tela::Document d;
        d.separator("line");
        d.scroll("box", [&] { for (int i = 0; i < 10; ++i) d.text("t" + std::to_string(i), "x", {.height = 30, .padding = 0}); },
                 {.height = 100, .padding = 0, .gap = 0});
        d.scroll("rest", [&] { d.text("r", "x"); }, {.padding = 0});
        const tela::Theme theme;
        auto placed = tela::arrange(d, 300, 400, theme, {{"box", 1000.f}});
        require(placed[0].bounds.height == 17, "separator reserves a line between paddings");
        const auto& box = placed[1];
        require(box.content_height == 300 && box.scroll == 200, "scroll offset is clamped to content minus visible height");
        require(placed[2].bounds.y == box.bounds.y - 200, "children move up by the scroll offset");
        require(placed[2].clip.height == 0 && placed[11].clip.height == 30, "children outside the scroll are clipped away");
        const auto& rest = placed[12];
        require(rest.element.id == "rest" && rest.bounds.y + rest.bounds.height == 400, "a scroll without height fills the remaining height");
    }
    App app;
    // Focus by click, caret from the click position, Tab order.
    const auto name = app.at("name");   // a copy: declare() rebuilds the placed elements
    require(app.runtime.focused().empty(), "nothing is focused at first");
    app.name = "abcdef"; app.declare();
    app.click(name.bounds.x + 4 + 31, name.bounds.y + 10);
    require(app.runtime.focused() == "name" && app.state("name").caret == 3, "click focuses the field and places the caret");
    require(app.key(tela::Key::tab) && app.runtime.focused() == "secret", "Tab moves to the next control");
    require(app.state("secret").caret == 2, "Tab puts the caret after the value");
    require(app.key(tela::Key::tab, tela::modifier_shift) && app.runtime.focused() == "name", "Shift+Tab moves back");
    app.click(380, 230);
    require(app.runtime.focused().empty(), "a press outside any control blurs");
    require(!app.type("x"), "text without focus goes nowhere");
    // Typing goes through the application's value.
    app.runtime.focus("name");
    require(app.type("g") && app.name == "abcdefg" && app.state("name").caret == 7, "committed text is inserted and reported");
    require(app.type("\xE3\x81\x82") && app.name == "abcdefg\xE3\x81\x82", "IME committed text is inserted whole");
    require(app.key(tela::Key::backspace) && app.name == "abcdefg", "backspace removes the multi-byte character");
    require(!app.type("a\nb") || app.name == "abcdefgab", "a text field drops line breaks from committed text");
    app.key(tela::Key::home);
    app.key(tela::Key::right, tela::modifier_shift);
    app.key(tela::Key::right, tela::modifier_shift);
    require(app.state("name").anchor == 0 && app.state("name").caret == 2, "Shift+arrow extends a selection");
    app.type("Z");
    require(app.name.rfind("Zcdefg", 0) == 0, "typing replaces the selection");
    app.key(tela::Key::a, tela::modifier_control);
    app.key(tela::Key::delete_forward);
    require(app.name.empty(), "Ctrl+A then Delete clears the field");
    app.key(tela::Key::enter);
    require(app.submitted == 1, "Enter submits a text field");
    require(!app.runtime.text({app.keys, "late"}) && app.name.empty(), "a stale text sequence is ignored");
    require(!app.type(std::string(4097, 'a')), "oversized committed text is refused");
    require(!app.type("\xC3"), "invalid UTF-8 committed text is refused");
    // An application that keeps its value unchanged keeps the caret inside it.
    app.name = "abc"; app.declare();
    app.key(tela::Key::end);
    app.name = "a"; app.declare();
    require(app.state("name").caret == 1, "a shorter redeclared value clamps the caret");
    // Text area: newlines, vertical moves, first row follows the caret.
    app.runtime.focus("notes");
    require(app.state("notes").caret == app.notes.size(), "caret starts at the end of the area");
    app.key(tela::Key::up);
    require(app.state("notes").caret == 7, "Up stops at the end of a shorter previous line");
    app.key(tela::Key::left);
    app.key(tela::Key::enter);
    require(app.notes == "one\ntw\no\nthree", "Enter inserts a newline in a text area");
    const auto view = tela::field_view(app.at("notes").element, app.at("notes").bounds,
        {app.state("notes").caret, app.state("notes").anchor}, true, app.runtime.theme(), fixed);
    require(view.first_line == 1 && view.lines.size() == 2, "only the reserved rows around the caret are shown");
    require(!app.key(tela::Key::up) || app.runtime.focused() == "notes", "Up stays inside the area");
    require(app.key(tela::Key::escape) && app.runtime.focused().empty(), "Escape blurs");
    // Secret fields show bullets but edit the real value.
    {
        const auto secret = app.at("secret").element;
        require(tela::field_display(secret) == "\xE2\x80\xA2\xE2\x80\xA2", "a secret shows one bullet per character");
        require(tela::field_display_offset(secret, 1) == 3, "secret caret maps to the bullet boundary");
        app.runtime.focus("secret");
        app.type("\xE3\x81\x82");
        require(app.secret == "pw\xE3\x81\x82", "typing into a secret edits the real value");
        require(tela::field_display(app.at("secret").element).size() == 9, "the display has no secret bytes");
    }
    // Slider: press and drag set the fraction through on_set; keys step it.
    {
        const auto slider = app.at("volume");
        const float left = slider.bounds.x + 4;
        app.press(left + 50, slider.bounds.y + 10);
        require(std::abs(app.volume - .25f) < .01f && app.runtime.focused() == "volume", "press sets the fraction and focuses");
        app.move(left + 150, slider.bounds.y + 30);
        require(std::abs(app.volume - .75f) < .01f, "dragging follows the pointer even off the track");
        app.release(left + 150, slider.bounds.y + 30);
        app.key(tela::Key::home);
        require(app.volume == 0, "Home moves the slider to zero");
        app.key(tela::Key::right);
        require(std::abs(app.volume - .05f) < .001f, "arrow keys step the slider");
    }
    // List items activate like buttons; the wheel scrolls; scroll survives redeclaration.
    {
        const auto row = app.at("row-1");
        app.click(row.bounds.x + 5, row.bounds.y + 5);
        require(app.picked == 1 && app.at("row-1").element.value.selected, "a list item click runs its action");
        require(app.wheel(50, 300, -1) && app.state("list").scroll == 72, "a wheel notch scrolls three rows");
        require(app.at("row-0").bounds.y == 260 - 72, "children follow the scroll offset");
        app.declare();
        require(app.state("list").scroll == 72, "scroll position survives redeclaration");
        app.wheel(50, 300, -100);
        require(app.state("list").scroll == 500, "scroll stops at the end of the content");
        app.rows = 5; app.declare();
        require(app.state("list").scroll == 0, "content that shrank clamps the offset");
        require(!app.wheel(50, 500, -1), "the wheel outside a scroll is not consumed");
        require(!tela::exclusive_regions(app.runtime.elements()).empty(), "controls take exclusive regions on overlays");
    }
    // The look is decided from the placed result alone, so every backend draws the same items.
    {
        app.runtime.focus("secret");
        const tela::TextMeasure measure = fixed;
        const auto secret = app.at("secret");
        const auto items = tela::control_visual({secret, app.runtime.state("secret"), true, app.runtime.theme(), measure});
        bool border = false, caret = false, bullets = false;
        for (const auto& item : items) {
            if (const auto* r = std::get_if<tela::VisualRect>(&item)) {
                border |= r->border > 0 && r->color == app.runtime.theme().accent;
                caret |= r->border == 0 && r->color == app.runtime.theme().text;
            }
            if (const auto* t = std::get_if<tela::VisualText>(&item))
                bullets = t->text.find("pw") == std::string::npos && t->text.rfind("\xE2\x80\xA2", 0) == 0;
        }
        require(border && caret && bullets, "a focused secret field shows a focus border, a caret and only bullets");
        const auto status = tela::control_visual({app.at("ok"), nullptr, false, app.runtime.theme(), measure});
        require(std::holds_alternative<tela::VisualEllipse>(status.front()) &&
                std::get<tela::VisualEllipse>(status.front()).color == app.runtime.theme().good, "status draws its tone mark first");
        require(!tela::is_control(tela::ElementKind::button) && tela::is_control(tela::ElementKind::video), "controls are told apart from the base elements");
    }
    // Tab into a control below a scrolled container reveals it.
    {
        tela::Runtime r;
        r.viewport({"host", "view", 1, 0, 0, 300, 300, 1, true, true});
        tela::Document d;
        d.scroll("s", [&] {
            for (int i = 0; i < 10; ++i) d.text_field("f" + std::to_string(i), "", {}, {.height = 40, .padding = 4});
        }, {.height = 100, .padding = 0, .gap = 0});
        r.document(std::move(d));
        for (int i = 0; i < 5; ++i) r.key({static_cast<std::uint64_t>(i + 1), tela::Key::tab, true, 0});
        require(r.focused() == "f4", "Tab walks the fields in order");
        require(r.state("s")->scroll == 100, "focusing a hidden field scrolls it into view");
        require(!r.key({1, tela::Key::tab, true, 0}), "a stale key sequence is ignored");
        r.disconnect();
        require(r.focused().empty(), "disconnect drops focus");
    }
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
