#include <tela/document.hpp>
#include <tela/layout.hpp>
#include <stdexcept>
int main() {
    tela::Document ui;
    ui.text("outside", "Host annotation");
    try {
        ui.panel("failed", [&] {
            ui.button("child", "Edit");
            ui.text("outside", "Duplicate");
        });
        return 1;
    } catch (const std::invalid_argument&) {}
    if (ui.elements().size() != 1) return 2;
    ui.panel("valid", [&] { ui.button("child", "Edit"); });
    if (ui.elements().back().parent != "valid") return 3;
    if (ui.elements().back().input != tela::InputPolicy::exclusive) return 4;
    ui.text("after", "Annotation");
    if (!ui.elements().back().parent.empty()) return 5;

    // A text box reserves the rows it declares, so a wrapped label keeps the lines past the
    // first break instead of losing them.
    tela::Document rows;
    rows.text("one", "single");
    rows.text("many", "wrapped", {.lines = 3});
    const tela::Theme theme;
    const auto placed = tela::arrange(rows, 200, 200, theme);
    if (placed.size() != 2) return 6;
    if (placed[0].bounds.height != theme.line_height + 2*placed[0].element.layout.padding) return 7;
    if (placed[1].bounds.height != theme.line_height*3 + 2*placed[1].element.layout.padding) return 8;
    try { tela::Document bad; bad.text("rows", "value", {.lines = 0}); return 9; }
    catch (const std::invalid_argument&) {}
    try { tela::Document bad; bad.text("rows", "value", {.lines = 65}); return 10; }
    catch (const std::invalid_argument&) {}
    return 0;
}
