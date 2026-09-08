#include <tela/document.hpp>
int main() {
    tela::Document ui;
    ui.panel("transition", [&] {
        ui.text("destination", "Scene transition specification");
        ui.button("edit", "Edit transition");
    });
    // This sample builds a document only; it does not open an overlay window.
    return ui.elements().size() == 3 ? 0 : 1;
}
