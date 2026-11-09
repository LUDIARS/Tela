// @implements SPEC-TL-FIELD
// @spec Field
// Contract of the one-line text field: focus, typing, editing keys and how the
// field survives the application declaring the document again.
#include <tela/runtime.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

tela::HostPointerEvent click(std::uint64_t sequence, tela::PointerPhase phase, int x, int y) {
    tela::HostPointerEvent e;
    e.sequence = sequence; e.viewport_revision = 1; e.gesture_id = sequence / 2; e.phase = phase;
    e.button = tela::PointerButton::primary; e.desktop_x = x; e.desktop_y = y;
    return e;
}

// A tiny application: two fields and a button, redeclared after every edit as
// a real tool does.
struct Form {
    tela::Runtime runtime;
    std::string name = "jewel";
    std::string count = "5";
    int edits = 0;
    int saves = 0;

    Form() {
        runtime.viewport({"host", "view", 1, 0, 0, 400, 300, 1, true, true});
        declare();
    }
    void declare() {
        tela::Document d;
        d.field("name", name, [this](const std::string& v) { name = v; ++edits; declare(); }, {.width = 200});
        d.field("count", count, [this](const std::string& v) { count = v; ++edits; declare(); }, {.width = 200});
        d.button("save", "Save", [this] { ++saves; }, {.width = 100});
        runtime.document(std::move(d));
    }
    void press(const std::string& id, std::uint64_t sequence) {
        for (const auto& p : runtime.elements()) {
            if (p.element.id != id) continue;
            const int x = static_cast<int>(p.bounds.x + 4), y = static_cast<int>(p.bounds.y + 4);
            runtime.pointer(click(sequence * 2, tela::PointerPhase::down, x, y), tela::InputSource::native);
            runtime.pointer(click(sequence * 2 + 1, tela::PointerPhase::up, x, y), tela::InputSource::native);
            return;
        }
        throw std::runtime_error("no such element");
    }
};
}

int main() {
    try {
        {
            Form f;
            require(f.runtime.focused().empty(), "nothing is focused at first");
            require(!f.runtime.text("x"), "typing without focus is not consumed");
            require(!f.runtime.key(tela::EditKey::escape), "escape without focus is left to the host");

            f.press("name", 1);
            require(f.runtime.focused() == "name", "a click focuses the field");
            require(f.runtime.text("s"), "typing into the focused field is consumed");
            require(f.name == "jewels" && f.edits == 1, "the edit callback gets the whole new text");
            require(f.runtime.focused() == "name", "focus survives the redeclaration");

            f.runtime.text("\x01\t\x7f");
            require(f.name == "jewels" && f.edits == 1, "control characters are not typed");
            f.runtime.text("\xe5\xae\x9d");  // 宝 (an input method's committed text)
            require(f.name == "jewels\xe5\xae\x9d", "multi-byte characters are typed whole");
            f.runtime.key(tela::EditKey::backspace);
            require(f.name == "jewels", "backspace removes one whole character");
            f.runtime.text("\xff");
            require(f.name == "jewels", "invalid UTF-8 is dropped");

            f.runtime.key(tela::EditKey::escape);
            require(f.name == "jewel", "escape restores the text from when the field was focused");
            require(f.runtime.focused().empty(), "escape leaves the field");

            f.press("count", 2);
            f.runtime.key(tela::EditKey::backspace);
            f.runtime.text("12");
            f.runtime.key(tela::EditKey::enter);
            require(f.count == "12" && f.runtime.focused().empty(), "enter keeps the text and leaves the field");

            f.press("name", 3);
            f.runtime.key(tela::EditKey::tab);
            require(f.runtime.focused() == "count", "tab moves to the next field");
            f.runtime.key(tela::EditKey::tab);
            require(f.runtime.focused() == "name", "tab wraps around");
            f.runtime.key(tela::EditKey::back_tab);
            require(f.runtime.focused() == "count", "shift+tab moves back");

            f.press("save", 4);
            require(f.runtime.focused().empty(), "pressing elsewhere ends editing");
            require(f.saves == 1, "the button still works");
        }
        {
            // A field that disappears loses focus; one that becomes text does too.
            tela::Runtime r;
            r.viewport({"host", "view", 1, 0, 0, 400, 300, 1, true, true});
            tela::Document d;
            d.field("f", "", [](const std::string&) {});
            r.document(std::move(d));
            r.focus("f");
            tela::Document replaced;
            replaced.text("f", "now text");
            r.document(std::move(replaced));
            require(r.focused().empty(), "focus does not move to a non-field");
            bool threw = false;
            try { r.focus("f"); } catch (const std::invalid_argument&) { threw = true; }
            require(threw, "only fields can be focused");
        }
        {
            bool threw = false;
            try { tela::Document d; d.field("f", "", {}); } catch (const std::invalid_argument&) { threw = true; }
            require(threw, "a field needs an edit callback");
            threw = false;
            try { tela::Document d; d.field("f", "", [](const std::string&) {}, {.lines = 2}); }
            catch (const std::invalid_argument&) { threw = true; }
            require(threw, "a field is one line");
        }
        std::cout << "field contract ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
