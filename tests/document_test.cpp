#include <tela/document.hpp>
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
    return 0;
}
