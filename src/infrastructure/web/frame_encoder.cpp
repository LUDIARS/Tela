// @spec SPEC-TL-WEB
#include "frame_encoder.hpp"
#include "json_writer.hpp"
#include <tela/field_geometry.hpp>

namespace tela::web {
namespace {
constexpr int protocol_version = 1;
const char* input_name(InputPolicy policy) {
    switch (policy) {
    case InputPolicy::exclusive: return "exclusive";
    case InputPolicy::shared: return "shared";
    default: return "passthrough";
    }
}
const char* tone_name(Tone tone) {
    switch (tone) {
    case Tone::good: return "good";
    case Tone::warning: return "warning";
    case Tone::bad: return "bad";
    default: return "neutral";
    }
}
const char* shape_name(ShapeKind kind) {
    switch (kind) {
    case ShapeKind::ellipse: return "ellipse";
    case ShapeKind::polyline: return "polyline";
    case ShapeKind::image: return "image";
    default: return "rectangle";
    }
}
void point(JsonWriter& w, Point p) { w.begin_array().number(p.x).number(p.y).end_array(); }
void theme(JsonWriter& w, const Theme& t) {
    w.key("theme").begin_object()
        .key("panel").color(t.panel).key("text").color(t.text)
        .key("button").color(t.button).key("pressed").color(t.pressed)
        .key("field").color(t.field).key("accent").color(t.accent).key("muted").color(t.muted)
        .key("good").color(t.good).key("warning").color(t.warning).key("bad").color(t.bad)
        .key("font_size").number(t.font_size).key("line_height").number(t.line_height)
        .end_object();
}
void shape(JsonWriter& w, const Shape& s, ImageRegistry& images) {
    w.begin_object().key("kind").string(shape_name(s.kind)).key("bounds").rect(s.bounds);
    if (s.radius) w.key("radius").number(s.radius);
    if (s.fill.a) w.key("fill").color(s.fill);
    if (s.stroke.width > 0 && s.stroke.color.a) {
        w.key("stroke").begin_object().key("color").color(s.stroke.color).key("width").number(s.stroke.width);
        if (s.stroke.dash) w.key("dash").number(s.stroke.dash).key("gap").number(s.stroke.gap);
        if (s.stroke.glow.radius > 0 && s.stroke.glow.color.a)
            w.key("glow").begin_object().key("color").color(s.stroke.glow.color)
                .key("radius").number(s.stroke.glow.radius).end_object();
        w.end_object();
    }
    if (s.gradient) {
        w.key("gradient").begin_object().key("start");
        point(w, s.gradient->start);
        w.key("end");
        point(w, s.gradient->end);
        w.key("from").color(s.gradient->from).key("to").color(s.gradient->to).end_object();
    }
    if (!s.points.empty()) {
        w.key("points").begin_array();
        for (auto p : s.points) point(w, p);
        w.end_array();
    }
    if (s.opacity != 1) w.key("opacity").number(s.opacity);
    if (s.kind == ShapeKind::image) {
        const auto id = images.id(s.image);
        if (id.empty()) w.key("image").null(); else w.key("image").string(id);
    }
    w.end_object();
}
void field(JsonWriter& w, const Element& e, const ElementState* state) {
    const auto display = field_display(e);
    w.key("value").string(display);
    if (e.value.secret) w.key("secret").boolean(true);
    const auto caret = state ? state->caret : 0, anchor = state ? state->anchor : 0;
    // Code point indices, so the page never needs the hidden bytes of a secret.
    w.key("caret").integer(text_edit::characters(display, field_display_offset(e, caret)));
    w.key("anchor").integer(text_edit::characters(display, field_display_offset(e, anchor)));
}
std::string element(const PlacedElement& placed, const Runtime& runtime, ImageRegistry& images) {
    const auto& e = placed.element;
    const auto* state = runtime.state(e.id);
    JsonWriter w;
    w.begin_object().key("id").string(e.id).key("kind").string(kind_name(e.kind))
        .key("bounds").rect(placed.bounds).key("clip").rect(placed.clip)
        .key("padding").number(e.layout.padding).key("lines").integer(e.layout.lines)
        .key("text_scale").number(e.layout.text_scale).key("input").string(input_name(e.input));
    if (!e.label.empty()) w.key("label").string(e.label);
    if (state && state->pressed) w.key("pressed").boolean(true);
    switch (e.kind) {
    case ElementKind::text_field: case ElementKind::text_area: field(w, e, state); break;
    case ElementKind::slider: w.key("fraction").number(e.value.fraction); break;
    case ElementKind::list_item:
        if (!e.value.text.empty()) w.key("detail").string(e.value.text);
        if (e.value.selected) w.key("selected").boolean(true);
        break;
    case ElementKind::status: w.key("tone").string(tone_name(e.value.tone)); break;
    case ElementKind::scroll: w.key("scroll").number(placed.scroll).key("content_height").number(placed.content_height); break;
    case ElementKind::video: w.key("stream").string(e.value.stream); break;
    case ElementKind::canvas:
        w.key("shapes").begin_array();
        for (const auto& s : e.drawing.shapes()) shape(w, s, images);
        w.end_array();
        break;
    default: break;
    }
    w.end_object();
    return w.release();
}
}
const char* kind_name(ElementKind kind) noexcept {
    switch (kind) {
    case ElementKind::panel: return "panel";
    case ElementKind::text: return "text";
    case ElementKind::button: return "button";
    case ElementKind::canvas: return "canvas";
    case ElementKind::text_field: return "text_field";
    case ElementKind::text_area: return "text_area";
    case ElementKind::slider: return "slider";
    case ElementKind::list_item: return "list_item";
    case ElementKind::separator: return "separator";
    case ElementKind::status: return "status";
    case ElementKind::scroll: return "scroll";
    case ElementKind::video: return "video";
    }
    return "panel";
}
std::string frame_header(std::uint64_t revision) {
    return "{\"type\":\"frame\",\"version\":" + std::to_string(protocol_version) +
           ",\"revision\":" + std::to_string(revision) + ",";
}
EncodedFrame encode_frame(const Runtime& runtime, ImageRegistry& images, const WebLimits& limits) {
    const auto& view = runtime.viewport();
    JsonWriter w;
    // The body continues the header's object, so the first member needs no opening brace.
    w.key("width").number(view.width / view.dpi_scale).key("height").number(view.height / view.dpi_scale)
        .key("dpi").number(view.dpi_scale);
    theme(w, runtime.theme());
    if (runtime.focused().empty()) w.key("focus").null(); else w.key("focus").string(runtime.focused());
    // Room for the header, the closing of the array and object and the truncated flag.
    constexpr std::size_t reserve = 128;
    EncodedFrame frame;
    std::string elements;
    std::size_t count = 0;
    for (const auto& placed : runtime.elements()) {
        if (placed.clip.width <= 0 || placed.clip.height <= 0) continue; // nothing of it shows
        auto encoded = element(placed, runtime, images);
        if (count >= limits.elements || w.size() + elements.size() + encoded.size() + reserve > limits.frame_bytes) {
            frame.truncated = true;
            break;
        }
        if (count++) elements.push_back(',');
        elements += encoded;
    }
    w.key("elements").raw("[" + elements + "]").key("truncated").boolean(frame.truncated);
    frame.body = w.release() + "}";
    return frame;
}
}
