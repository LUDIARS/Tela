// @spec SPEC-TL-SOURCE-VIEW
#include <tela/source_view.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
template<class F> void rejects(F action) {
    bool rejected = false;
    try { action(); } catch(const std::invalid_argument&) { rejected = true; }
    require(rejected, "invalid source was accepted");
}
void model() {
    tela::SourceView view("example.cpp", "\xef\xbb\xbf" "a\r\n\t\xe6\x97\xa5\xe6\x9c\xac\nlast", 2);
    require(view.line_count() == 3 && view.first_line() == 2, "CRLF and initial target");
    require(view.slice(2, 5) == "    \xe6\x97\xa5", "tabs and UTF-8 slicing");
    view.navigate(tela::SourceAction::next, 100);
    require(view.first_line() == 3, "next clamps at final line");
    view.navigate(tela::SourceAction::previous, 100);
    require(view.first_line() == 1, "previous clamps at first line");
    view.navigate(tela::SourceAction::target, 10);
    require(view.first_line() == 2, "target restores reference");
    tela::SourceView empty("empty", "");
    require(empty.line_count() == 1 && empty.slice(1, 100).empty(), "empty source is readable");
    for(const auto& broken : {std::string("a\0b", 3), std::string("\xc0\xaf"), std::string("\xed\xa0\x80"),
        std::string("\xf4\x90\x80\x80"), std::string("\xe3\x81"), std::string(65537, 'x')})
        rejects([&] { tela::SourceView invalid("bad", broken); });
    rejects([] { tela::SourceView invalid("bad", "abc", 0); });
    rejects([] { tela::SourceView invalid("bad", "abc", 2); });
    rejects([] { tela::SourceView invalid("bad", std::string(tela::source_max_bytes + 1, 'x')); });
    tela::SourceView wide("wide", std::string(30, 'a') + "\xe6\x97\xa5\xe6\x9c\xac");
    wide.navigate(tela::SourceAction::right, 1);
    require(wide.column() == 24 && wide.slice(1, 7) == std::string(6, 'a') + "\xe6\x97\xa5", "horizontal Unicode slice");
    wide.navigate(tela::SourceAction::left, 1);
    require(wide.column() == 0, "horizontal origin restored");
    wide.navigate(tela::SourceAction::right, 1, 1);
    require(wide.column() == 1, "narrow windows never skip undisplayed characters");
}
void document() {
    std::string lines;
    for(int i = 0; i < 5000; ++i) lines += "line\n";
    tela::SourceView view("many.cpp", lines, 3000);
    tela::Viewport viewport;
    viewport.width = 1100; viewport.height = 800; viewport.dpi_scale = 1; viewport.visible = true;
    const auto rows = tela::source_page_rows(viewport, {});
    bool activated = false;
    const auto doc = tela::source_document(view, viewport, {}, [&](tela::SourceAction action) {
        activated = action == tela::SourceAction::next;
    });
    require(doc.elements().size() <= 9 + 2 * rows, "only visible rows are declared");
    const auto button = std::find_if(doc.elements().begin(), doc.elements().end(),
        [](const auto& item) { return item.id == "source/nav/1"; });
    require(button != doc.elements().end(), "next-page control exists");
    button->action(); require(activated, "control emits navigation intent");
    require(std::any_of(doc.elements().begin(), doc.elements().end(),
        [](const auto& item) { return item.id == "source/target"; }), "target highlighted");
    viewport.visible = false;
    require(tela::source_document(view, viewport, {}, {}).elements().empty(), "hidden host declares nothing");
}
void file(const std::filesystem::path& path) {
    const std::string original = "first\r\nsecond";
    { std::ofstream out(path, std::ios::binary); out << original; }
    const auto view = tela::load_source(path, 2);
    require(view.slice(2, 100) == "second", "file loaded at line");
    std::ifstream input(path, std::ios::binary);
    const std::string after{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    require(after == original, "reading does not modify the file");
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "fixture path required");
        model(); document(); file(argv[1]);
    } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
