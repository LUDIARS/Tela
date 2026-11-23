// @implements SPEC-TL-GPU
// @spec GPU rendering
// GPU-free half of the GPU backend: draw command conversion, clipping, text placement, layer
// order and texel conversion. The Vulkan half needs a device and is checked with tela_gpu_controls.
#include <tela/gpu_frame.hpp>
#include <tela/control_visual.hpp>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
float fixed(std::string_view text, float size) { return static_cast<float>(text.size()) * size / 2; }

tela::Runtime runtime_with(const std::function<void(tela::Document&)>& declare, float dpi = 1, int width = 400, int height = 300) {
    tela::Runtime runtime;
    runtime.viewport({"host", "view", 1, 0, 0, width, height, dpi, true, true});
    runtime.text_measure(fixed);
    tela::Document d;
    declare(d);
    runtime.document(std::move(d));
    return runtime;
}
const tela::PlacedElement& placed(const tela::Runtime& runtime, const std::string& id) {
    for (const auto& p : runtime.elements()) if (p.element.id == id) return p;
    throw std::runtime_error("element not placed: " + id);
}
std::vector<tela::GpuQuad> quads(const tela::GpuFrame& frame) {
    std::vector<tela::GpuQuad> out;
    for (const auto& step : frame.steps) if (const auto* q = std::get_if<tela::GpuQuad>(&step)) out.push_back(*q);
    return out;
}
std::vector<tela::LayerText> texts(const tela::GpuFrame& frame) {
    std::vector<tela::LayerText> out;
    for (const auto& layer : frame.layers)
        for (const auto& item : layer.items) if (const auto* t = std::get_if<tela::LayerText>(&item)) out.push_back(*t);
    return out;
}
bool is_layer(const tela::GpuStep& step, std::size_t index) {
    const auto* draw = std::get_if<tela::GpuLayerDraw>(&step);
    return draw && draw->index == index;
}

void panel_and_button_are_quads_in_physical_pixels() {
    const auto runtime = runtime_with([](tela::Document& d) {
        d.panel("root", [] {}, {.width = 100, .height = 50, .positioned = true});
        d.button("go", "", {}, {.width = 40, .height = 20, .positioned = true, .x = 120});
    }, 2, 400, 200);
    const auto frame = tela::plan_gpu_frame(runtime);
    require(frame.width == 400 && frame.height == 200, "frame has the physical viewport size");
    const auto q = quads(frame);
    require(q.size() == 2, "panel and button each give one quad");
    require(q[0].rect == tela::Rect{0, 0, 200, 100} && q[0].clip == q[0].rect, "panel quad is its clip scaled by DPI");
    require(q[0].color == runtime.theme().panel, "panel quad uses the panel color");
    require(q[1].rect == tela::Rect{240, 0, 80, 40} && q[1].color == runtime.theme().button, "button quad is scaled and unpressed");
    require(frame.layers.empty(), "nothing needs a layer");
}

void clip_matches_the_cpu_pixel_test() {
    require(tela::gpu_clip({0.5f, 0.5f, 10, 10}, 1) == tela::Rect{1, 1, 10, 10}, "fractional clip starts at the first admitted pixel");
    require(tela::gpu_clip({0, 0, 10.2f, 4}, 1) == tela::Rect{0, 0, 11, 4}, "a partial last column is admitted");
    require(tela::gpu_clip({1, 1, 3, 3}, 1.5f) == tela::Rect{2, 2, 4, 4}, "DPI scales before snapping");
    require(tela::gpu_clip({5, 5, 0, 3}, 1).width == 0, "empty clip stays empty");
}

void label_is_placed_inside_padding_as_the_cpu_raster_does() {
    for (float dpi : {1.f, 1.5f}) {
        const auto runtime = runtime_with([](tela::Document& d) {
            d.text("title", "Hello GPU", {.width = 120, .positioned = true, .x = 10, .y = 20});
        }, dpi);
        const auto& p = placed(runtime, "title");
        const auto frame = tela::plan_gpu_frame(runtime);
        const auto t = texts(frame);
        require(t.size() == 1 && t[0].text == "Hello GPU" && t[0].wrap, "a label becomes one wrapped layer text");
        const float pad = p.element.layout.padding * dpi;
        require(t[0].box.x == std::floor(p.bounds.x * dpi + pad) && t[0].box.y == std::floor(p.bounds.y * dpi + pad),
                "label box starts at the floored padded corner");
        require(t[0].box.width == static_cast<float>(static_cast<int>(p.bounds.width * dpi - 2 * pad)), "label box width");
        require(t[0].font_px == runtime.theme().font_size * dpi, "label font is scaled by DPI");
        require(t[0].clip == tela::gpu_clip(p.clip, dpi), "label is clipped to its element");
        require(frame.steps.size() == 1 && is_layer(frame.steps[0], 0), "the layer is drawn once");
    }
}

void control_text_uses_control_visual_positions() {
    const float dpi = 1.25f;
    const auto runtime = runtime_with([](tela::Document& d) {
        d.list_item("row", "Item", "detail", true, {}, {.width = 200, .padding = 4, .positioned = true, .y = 30});
    }, dpi);
    const auto& p = placed(runtime, "row");
    const tela::ControlLook look{p, runtime.state("row"), false, runtime.theme(), runtime.text_measure()};
    std::vector<tela::VisualText> expected;
    for (const auto& item : tela::control_visual(look))
        if (const auto* v = std::get_if<tela::VisualText>(&item)) expected.push_back(*v);
    const auto t = texts(tela::plan_gpu_frame(runtime));
    require(t.size() == expected.size() && t.size() == 2, "label and detail become layer texts");
    for (std::size_t i = 0; i < t.size(); ++i) {
        require(t[i].text == expected[i].text && !t[i].wrap, "control text is one unwrapped line");
        require(t[i].box.x == expected[i].origin.x * dpi && t[i].box.y == expected[i].origin.y * dpi, "same origin as control_visual");
        require(t[i].line_px == expected[i].line_height * dpi && t[i].font_px == expected[i].font_size * dpi, "same size as control_visual");
        require(t[i].clip == tela::intersect(tela::gpu_clip(expected[i].clip, dpi), tela::gpu_clip(p.clip, dpi)),
                "text clip is the control area within the element clip");
    }
}

void scrolled_out_items_are_not_drawn() {
    const auto runtime = runtime_with([](tela::Document& d) {
        d.scroll("list", [&] {
            for (int i = 0; i < 20; ++i) d.list_item("i" + std::to_string(i), "Row", "", false, {}, {.padding = 2});
        }, {.width = 200, .height = 60, .padding = 0, .gap = 0, .positioned = true});
    });
    const auto frame = tela::plan_gpu_frame(runtime);
    const auto& list = placed(runtime, "list");
    for (const auto& q : quads(frame))
        require(tela::intersect(q.clip, tela::gpu_clip(list.clip, 1)) == q.clip, "every quad stays inside the scroll");
    std::size_t rows = 0;
    for (const auto& t : texts(frame)) {
        require(t.clip.y >= list.bounds.y && t.clip.y + t.clip.height <= list.bounds.y + list.bounds.height, "text inside the scroll");
        ++rows;
    }
    require(rows > 0 && rows < 20, "only visible rows produce text");
}

void quad_over_text_closes_the_layer_first() {
    const auto declare = [](tela::Document& d) {
        d.text("under", "below", {.width = 100, .positioned = true});
        d.button("over", "", {}, {.width = 100, .height = 30, .positioned = true});
        d.text("top", "above", {.width = 100, .positioned = true, .y = 5});
    };
    const auto runtime = runtime_with(declare);
    const auto two = tela::plan_gpu_frame(runtime, {2});
    require(two.steps.size() == 3 && is_layer(two.steps[0], 0) && std::holds_alternative<tela::GpuQuad>(two.steps[1]) &&
            is_layer(two.steps[2], 1), "text, covering quad, text keep their order across two layers");
    const auto one = tela::plan_gpu_frame(runtime, {1});
    require(one.steps.size() == 1 && is_layer(one.steps[0], 0), "with one layer the quad joins it");
    require(one.layers[0].items.size() == 3 && std::holds_alternative<tela::LayerShapes>(one.layers[0].items[1]),
            "the quad is rasterized between the two texts");
}

void quad_beside_text_keeps_one_layer() {
    const auto runtime = runtime_with([](tela::Document& d) {
        d.text("label", "left", {.width = 100, .positioned = true});
        d.button("right", "", {}, {.width = 100, .height = 30, .positioned = true, .x = 150});
    });
    const auto frame = tela::plan_gpu_frame(runtime);
    require(frame.steps.size() == 2 && std::holds_alternative<tela::GpuQuad>(frame.steps[0]) && is_layer(frame.steps[1], 0),
            "a quad that covers no text stays on the GPU before the single layer");
}

void canvas_shapes_split_between_quads_and_layers() {
    tela::Drawing drawing;
    drawing.rectangle({0, 0, 20, 10}, {200, 0, 0, 255}, {{0, 0, 0, 0}, 0}, 3);
    drawing.ellipse({30, 0, 20, 10}, {0, 200, 0, 255});
    drawing.gradient_rectangle({60, 0, 20, 10}, {{0, 0}, {20, 0}, {0, 0, 0, 255}, {255, 255, 255, 255}});
    drawing.rectangle({0, 20, 20, 10}, {0, 0, 200, 255}, {{255, 255, 255, 255}, 2});
    const auto runtime = runtime_with([&](tela::Document& d) {
        d.canvas("art", drawing, {.width = 100, .height = 40, .positioned = true, .x = 5, .y = 5});
    }, 2);
    const auto frame = tela::plan_gpu_frame(runtime);
    const auto q = quads(frame);
    require(q.size() == 1 && q[0].rect == tela::Rect{10, 10, 40, 20} && q[0].radius == 6, "a plain fill is a rounded quad");
    require(frame.layers.size() == 1 && frame.layers[0].items.size() == 1, "the other shapes share one layer item");
    const auto& shapes = std::get<tela::LayerShapes>(frame.layers[0].items[0]);
    require(shapes.drawing.shapes().size() == 3 && shapes.scale == 2 && shapes.origin == tela::Point{5, 5},
            "ellipse, gradient and stroked rectangle are rasterized in canvas coordinates");
}

void status_dot_is_a_round_quad_and_secrets_stay_hidden() {
    const auto runtime = runtime_with([](tela::Document& d) {
        d.status("ok", "Ready", tela::Tone::good, {.width = 200, .positioned = true});
        d.text_field("pw", "hunter2", {.secret = true}, {.width = 200, .positioned = true, .y = 50});
    });
    const auto frame = tela::plan_gpu_frame(runtime);
    bool dot = false;
    for (const auto& q : quads(frame))
        if (q.color == runtime.theme().good) dot = q.rect.width == q.rect.height && q.radius == q.rect.width / 2;
    require(dot, "the status mark is a circle quad");
    for (const auto& t : texts(frame)) require(t.text.find("hunter2") == std::string::npos, "secret bytes never reach drawing");
}

void unchanged_runtime_plans_the_same_frame() {
    const auto runtime = runtime_with([](tela::Document& d) {
        d.text("a", "same", {.width = 100, .positioned = true});
        d.button("b", "", {}, {.width = 100, .height = 30, .positioned = true});
    });
    require(tela::plan_gpu_frame(runtime) == tela::plan_gpu_frame(runtime), "planning is deterministic, so unchanged layers skip uploads");
}

void texels_are_straight_rgba() {
    tela::PixelSurface surface{2, 1, {0, 64, 128, 128, 0, 0, 0, 0}};
    std::vector<unsigned char> rgba(3 * 1 * 4, 7);
    tela::layer_texels(surface, rgba.data(), 3);
    require(rgba[0] == 255 && rgba[1] == 128 && rgba[2] == 0 && rgba[3] == 128, "premultiplied BGRA becomes straight RGBA");
    require(rgba[4] == 0 && rgba[5] == 0 && rgba[6] == 0 && rgba[7] == 0, "transparent stays zero");
    require(rgba[8] == 7 && rgba[11] == 7, "texels beyond the surface row are untouched");
}
}

int main() {
    try {
        tela::GpuReveal reveal;
        reveal.begin(false);
        require(reveal.alpha() == 96, "new detail is visible immediately");
        reveal.advance(0.06f);
        require(reveal.alpha() == 24, "overlay eases out halfway through");
        reveal.begin(false);
        require(reveal.alpha() == 96, "repeat selection restarts the reveal");
        reveal.advance(1.f);
        require(!reveal.active() && reveal.alpha() == 0, "no redraw after completion");
        reveal.begin(true);
        require(!reveal.active(), "reduced motion presents the destination directly");
        panel_and_button_are_quads_in_physical_pixels();
        clip_matches_the_cpu_pixel_test();
        label_is_placed_inside_padding_as_the_cpu_raster_does();
        control_text_uses_control_visual_positions();
        scrolled_out_items_are_not_drawn();
        quad_over_text_closes_the_layer_first();
        quad_beside_text_keeps_one_layer();
        canvas_shapes_split_between_quads_and_layers();
        status_dot_is_a_round_quad_and_secrets_stay_hidden();
        unchanged_runtime_plans_the_same_frame();
        texels_are_straight_rgba();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "gpu frame contract passed\n";
    return 0;
}
