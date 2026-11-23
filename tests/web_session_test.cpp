// @implements SPEC-TL-WEB
// @spec Web rendering
#include <tela/web_session.hpp>
#include "../src/infrastructure/web/json_reader.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
using tela::web::JsonValue;
JsonValue parse(const std::string& message) {
    auto value = tela::web::parse_json(message, {16, 1u << 20, 1u << 22});
    if (!value) throw std::runtime_error("WebSession emitted invalid JSON");
    return *value;
}
const JsonValue* element(const JsonValue& frame, const std::string& id) {
    for (const auto& e : frame.find("elements")->items) if (e.find("id")->text == id) return &e;
    return nullptr;
}
std::string pointer(const char* phase, float x, float y) {
    return std::string("{\"type\":\"pointer\",\"phase\":\"") + phase + "\",\"button\":\"" +
           (std::string(phase) == "move" ? "none" : "primary") + "\",\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + "}";
}
struct App {
    tela::Runtime runtime;
    std::string name, secret = "hunter2";
    std::shared_ptr<const tela::Image> image;
    int rows = 1;
    App() {
        auto pixels = std::make_shared<tela::Image>();
        pixels->width = 1; pixels->height = 1; pixels->pixels = {0, 0, 128, 128}; // premultiplied half red
        image = pixels;
        declare();
    }
    void declare() {
        tela::Document d;
        d.text("title", "Web", {.positioned = true});
        d.text_field("name", name, {.placeholder = "Name",
            .on_change = [this](const std::string& v) { name = v; declare(); }}, {.width = 200, .positioned = true, .y = 40});
        d.text_field("secret", secret, {.secret = true,
            .on_change = [this](const std::string& v) { secret = v; declare(); }}, {.width = 200, .positioned = true, .y = 80});
        tela::Drawing drawing;
        drawing.image({0, 0, 10, 10}, image);
        drawing.rectangle({0, 0, 4, 4}, {255, 0, 0, 255});
        d.canvas("art", drawing, {.width = 20, .height = 20, .positioned = true, .y = 120});
        d.video("remote", "Remote play", "play-1", {.width = 100, .height = 60, .positioned = true, .y = 150});
        d.scroll("list", [&] {
            for (int i = 0; i < rows; ++i) d.text("row" + std::to_string(i), "row", {.height = 20, .padding = 0});
        }, {.height = 40, .padding = 0, .gap = 0, .positioned = true, .y = 220});
        runtime.document(std::move(d));
    }
};
}

int main() { try {
    App app;
    tela::WebSession session(app.runtime);
    require(session.poll().empty(), "nothing is sent before the page reports its size");
    require(!session.receive(pointer("down", 1, 1)) && session.last_error() == "pointer before resize", "input before resize is refused");
    require(session.receive(R"({"type":"resize","width":400,"height":300,"dpi":2})"), "resize is accepted");
    require(app.runtime.viewport().width == 800 && app.runtime.viewport().dpi_scale == 2, "CSS size and device ratio become the viewport");
    auto messages = session.poll();
    require(messages.size() == 2, "first poll sends the image once, then the frame");
    const auto image = parse(messages[0]);
    require(image.find("type")->text == "image" && image.find("width")->number == 1, "image message carries its size");
    require(image.find("rgba")->text == "/wAAgA==", "image pixels are straight RGBA");
    auto frame = parse(messages[1]);
    require(frame.find("type")->text == "frame" && frame.find("version")->number == 1 && frame.find("revision")->number == 1,
            "frame carries type, protocol version and revision");
    require(frame.find("width")->number == 400 && frame.find("height")->number == 300, "frame size is in CSS pixels");
    require(frame.find("theme")->find("font_size")->number == 16 && frame.find("focus")->type == JsonValue::Type::null,
            "frame carries the theme and the focus");
    require(frame.find("truncated")->boolean == false, "a small frame is complete");
    const auto* name = element(frame, "name");
    require(name && name->find("kind")->text == "text_field" && name->find("label")->text == "Name", "elements carry kind and label");
    require(name->find("bounds")->items.size() == 4 && name->find("clip")->items.size() == 4, "elements carry bounds and clip");
    const auto* art = element(frame, "art");
    require(art->find("shapes")->items.size() == 2 && art->find("shapes")->items[0].find("image")->text == image.find("id")->text,
            "canvas shapes reference sent images by ID");
    require(element(frame, "remote")->find("stream")->text == "play-1", "video elements name their stream");
    require(element(frame, "list")->find("content_height")->number == 20, "scroll elements carry their content height");
    require(session.poll().empty(), "nothing is sent while nothing changed");
    // Secret contents never leave the process.
    for (const auto& message : messages) require(message.find("hunter2") == std::string::npos, "secret value is not sent");
    const auto& masked = element(frame, "secret")->find("value")->text;
    require(masked == "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2" &&
            element(frame, "secret")->find("secret")->boolean, "secret is sent as one bullet per character");
    // Input reaches the runtime: click focuses, text edits the application's value.
    require(session.receive(pointer("down", 20, 50)) && session.receive(pointer("up", 20, 50)), "pointer input accepted");
    require(app.runtime.focused() == "name", "a click from the page focuses the field");
    require(session.receive(R"({"type":"text","text":"日本"})") && app.name == "\xE6\x97\xA5\xE6\x9C\xAC", "committed text edits the value");
    require(session.receive(R"({"type":"key","key":"Backspace","pressed":true,"modifiers":0})") && app.name == "\xE6\x97\xA5", "keys edit the value");
    messages = session.poll();
    require(messages.size() == 1, "an edit sends one frame and no image again");
    frame = parse(messages[0]);
    require(frame.find("revision")->number == 2 && frame.find("focus")->text == "name", "the next frame has the next revision and the focus");
    require(element(frame, "name")->find("caret")->number == 1, "caret is sent as a code point index");
    session.receive(R"({"type":"key","key":"Tab","pressed":true})");
    session.receive(R"({"type":"text","text":"x"})");
    for (const auto& message : session.poll()) require(message.find("hunter2") == std::string::npos, "edited secret is not sent");
    require(app.secret == "hunter2x", "typing into a secret from the page edits the real value");
    // Wheel scrolls; focus loss from the page blocks keyboard input.
    app.rows = 10; app.declare();
    require(session.receive(R"({"type":"pointer","phase":"wheel","x":10,"y":230,"wheel_y":-1})") &&
            app.runtime.state("list")->scroll == 72, "wheel from the page scrolls");
    require(session.receive(R"({"type":"focus","focused":false})"), "focus message accepted");
    session.receive(R"({"type":"text","text":"y"})");
    require(app.secret == "hunter2x", "an unfocused page cannot type");
    session.receive(R"({"type":"focus","focused":true})");
    // Malformed and oversized input is refused before it reaches the runtime.
    for (const char* bad : {"", "{", "[]", R"({"type":"jump"})", R"({"type":"key","key":"F13","pressed":true})",
                            R"({"type":"pointer","phase":"down","x":1e9,"y":0})", R"({"type":"pointer","phase":"hover","x":1,"y":1})",
                            R"({"type":"resize","width":20000,"height":10})", R"({"type":"resize","width":100,"height":100,"dpi":9})",
                            R"({"type":"text","text":"a","type":"text"})", R"({"type":"text","text":"\ud800"})",
                            R"({"type":"key","key":"Tab","pressed":true,"modifiers":1.5})", R"({"type":"text"} trailing)"}) {
        require(!session.receive(bad) && !session.last_error().empty(), "malformed input is refused with a reason");
    }
    require(!session.receive(std::string(20000, ' ')), "an oversized message is refused");
    require(!session.receive("{\"type\":\"text\",\"text\":\"" + std::string(5000, 'a') + "\"}"), "oversized committed text is refused");
    require(!session.receive(R"({"type":"resize","width":16000,"height":16000,"dpi":1})"), "a viewport over the pixel budget is refused");
    // Limits on what is sent.
    {
        App small;
        tela::WebSession limited(small.runtime, {.elements = 3});
        limited.receive(R"({"type":"resize","width":400,"height":300})");
        const auto out = limited.poll();
        const auto f = parse(out.back());
        require(f.find("elements")->items.size() == 3 && f.find("truncated")->boolean, "element limit truncates and says so");
        App tiny;
        tela::WebSession bytes(tiny.runtime, {.frame_bytes = 1200, .image_bytes = 0});
        bytes.receive(R"({"type":"resize","width":400,"height":300})");
        const auto clipped = bytes.poll();
        require(clipped.size() == 1 && clipped[0].size() <= 1200, "frame byte limit holds and an over-limit image is not sent");
        require(parse(clipped[0]).find("truncated")->boolean, "byte limit truncation is flagged");
    }
    // Reconnect: a new page gets everything again, revisions keep rising.
    session.disconnect();
    require(app.runtime.focused().empty() && session.poll().empty(), "disconnect drops focus and stops frames");
    session.receive(R"({"type":"resize","width":400,"height":300})");
    messages = session.poll();
    require(messages.size() == 2 && parse(messages[1]).find("revision")->number > 2, "a reconnected page receives images and a full frame");
    // The reader keeps its own limits.
    require(!tela::web::parse_json(R"([[[[[1]]]]])", {4, 64, 16}), "nesting beyond the depth limit is refused");
    require(!tela::web::parse_json(R"({"a":"0123456789abcdefg"})", {4, 64, 16}), "strings beyond the byte limit are refused");
    require(tela::web::parse_json(R"("😀")")->text == "\xF0\x9F\x98\x80", "surrogate pairs decode to one character");
    return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
