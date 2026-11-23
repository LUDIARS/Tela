// @spec SPEC-TL-CONTROLS
#include "controls_gallery.hpp"
#include <cstdio>

ControlsGallery::ControlsGallery(tela::Runtime& runtime) : runtime_(runtime) { declare(); }

void ControlsGallery::declare() {
    tela::Document d;
    d.panel("root", [&] {
        d.text("title", "Tela controls");
        d.text_field("name", name_, {.placeholder = "Name",
            .on_change = [this](const std::string& value) { name_ = value; declare(); },
            .on_submit = [this] { submitted_ = name_; declare(); }});
        d.text_field("password", password_, {.placeholder = "Password", .secret = true,
            .on_change = [this](const std::string& value) { password_ = value; declare(); }});
        d.text_area("notes", notes_, [this](const std::string& value) { notes_ = value; declare(); }, {.lines = 3});
        d.panel("volume-row", [&] {
            char label[32];
            std::snprintf(label, sizeof label, "Volume %3d%%", static_cast<int>(volume_ * 100 + .5f));
            d.text("volume-label", label, {.width = 140});
            d.slider("volume", volume_, [this](float value) { volume_ = value; declare(); });
        }, {.padding = 0, .flow = tela::Flow::row});
        d.separator("rule");
        d.status("submitted", submitted_.empty() ? "Not submitted (Enter in Name)" : "Submitted: " + submitted_,
                 submitted_.empty() ? tela::Tone::neutral : tela::Tone::good);
        d.scroll("items", [&] {
            for (int i = 0; i < 20; ++i) {
                const auto id = "item-" + std::to_string(i);
                d.list_item(id, "Item " + std::to_string(i + 1), i % 3 ? "" : "every third", i == selected_,
                            [this, i] { selected_ = i; declare(); }, {.padding = 4});
            }
        }, {.height = 160, .padding = 2, .gap = 0});
        d.video("preview", "Preview: no stream attached", "preview", {.width = 320, .height = 120});
    });
    runtime_.document(std::move(d));
}
