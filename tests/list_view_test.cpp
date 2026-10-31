// @implements SPEC-TL-LIST-VIEW
// @spec List view
#include "../examples/desktop/list_file_source.hpp"
#include "../examples/desktop/list_overlay_options.hpp"
#include <tela/list_view.hpp>
#include <tela/utf8.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << text;
    if(!output) throw std::runtime_error("cannot write fixture");
}
template<class F> void rejects(F&& action, const char* why) {
    try { action(); } catch(const std::invalid_argument&) { return; }
    throw std::runtime_error(why);
}

// The shape Excubitor's network watch writes (its renderListView): a title, a status and marked
// rows with tab-separated columns. Japanese text is UTF-8.
const std::string exported =
    "TELA_LIST_VIEW 1\n"
    "title\tExcubitor \xe9\x80\x9a\xe4\xbf\xa1\xe5\x85\x88\n"
    "status\t\xe5\x9b\xbd\xe5\xa4\x96 2 \xe4\xbb\xb6\n"
    "row\talert\tUS\tapi.example.com\t203.0.113.5:443\t10:42:03\n"
    "row\tmuted\t??\t(unknown)\t192.0.2.9:443\t10:41:58\n"
    "row\tnormal\tJP\tjp.example\t198.51.100.1:443\t10:41:40\n";

const tela::Element* find(const tela::Document& document, const std::string& id) {
    const auto& elements = document.elements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) { return element.id == id; });
    return found == elements.end() ? nullptr : &*found;
}

void parsing() {
    const auto view = tela::parse_list_view(exported);
    require(view.title == "Excubitor \xe9\x80\x9a\xe4\xbf\xa1\xe5\x85\x88", "title is read");
    require(view.status == "\xe5\x9b\xbd\xe5\xa4\x96 2 \xe4\xbb\xb6", "status is read");
    require(view.rows.size() == 3, "rows are read in order");
    require(view.rows[0].mark == tela::ListMark::alert && view.rows[1].mark == tela::ListMark::muted
            && view.rows[2].mark == tela::ListMark::normal, "marks are read");
    require(view.rows[0].columns == std::vector<std::string>{"US", "api.example.com", "203.0.113.5:443", "10:42:03"},
            "columns are split on tabs");
    require(tela::parse_list_view("TELA_LIST_VIEW 1\ntitle\t\nstatus\t\n").rows.empty(), "an empty list and empty values are allowed");
    require(tela::parse_list_view("TELA_LIST_VIEW 1\n\ntitle\tT\nstatus\tS\nrow\tnormal\t\n").rows[0].columns
            == std::vector<std::string>{""}, "blank lines are skipped and an empty column is kept");
    require(tela::list_mark_from_name("alert") == tela::ListMark::alert
            && std::string(tela::list_mark_name(tela::ListMark::muted)) == "muted", "mark names round-trip");
}

void rejecting() {
    for(const char* broken : {
        "",
        "TELA_LIST_VIEW 2\ntitle\tT\nstatus\tS\n",
        "TELA_LIST_VIEW 1\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\tT\n",
        "TELA_LIST_VIEW 1\ntitle\tT\ntitle\tU\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\tT\nstatus\tS\nstatus\tU\n",
        "TELA_LIST_VIEW 1\ntitle\tT\textra\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\tT\nstatus\tS\nrow\twarning\tx\n",
        "TELA_LIST_VIEW 1\ntitle\tT\nstatus\tS\nrow\talert\n",
        "TELA_LIST_VIEW 1\ntitle\tT\nstatus\tS\nrow\talert\t1\t2\t3\t4\t5\t6\t7\t8\t9\n",
        "TELA_LIST_VIEW 1\ntitle\tT\nstatus\tS\nwidget\tx\n",
        "TELA_LIST_VIEW 1\r\ntitle\tT\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\tT\r\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\t\xff\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\t\xc0\xaf\nstatus\tS\n",
        "TELA_LIST_VIEW 1\ntitle\t\xed\xa0\x80\nstatus\tS\n",
    }) {
        rejects([&] { tela::parse_list_view(broken); }, "invalid list text is rejected whole");
    }
}

void limits() {
    std::string many = "TELA_LIST_VIEW 1\ntitle\tT\nstatus\tS\n";
    for(std::size_t i = 0; i < tela::list_view_max_rows; ++i) many += "row\tnormal\tx\n";
    require(tela::parse_list_view(many).rows.size() == tela::list_view_max_rows, "256 rows are accepted");
    rejects([&] { tela::parse_list_view(many + "row\tnormal\tx\n"); }, "257 rows are rejected");
    const std::string longest(tela::list_view_max_value_bytes, 'a');
    require(tela::parse_list_view("TELA_LIST_VIEW 1\ntitle\t" + longest + "\nstatus\tS\n").title.size() == 4096,
            "a 4096-byte value is accepted");
    rejects([&] { tela::parse_list_view("TELA_LIST_VIEW 1\ntitle\t" + longest + "a\nstatus\tS\n"); },
            "a 4097-byte value is rejected");
    require(tela::is_valid_utf8("\xf0\x9f\x98\x80") && !tela::is_valid_utf8("\xf4\x90\x80\x80")
            && !tela::is_valid_utf8("\xe3\x81"), "UTF-8 validation covers 4-byte, out-of-range and truncated input");
}

void composition() {
    const auto view = tela::parse_list_view(exported);
    tela::ListViewFrame frame{520, 2, "list.grip"};
    const tela::Theme theme;
    const auto document = tela::list_view_document(view, frame, theme);
    require(find(document, "list.backdrop") && find(document, "list.title") && find(document, "list.status"),
            "backdrop, title and status are declared");
    const auto* first = find(document, "list.row.0");
    require(first && first->label == "! US   api.example.com   203.0.113.5:443   10:42:03", "an alert row starts with !");
    require(find(document, "list.row.0.band") && !find(document, "list.row.1.band"), "only alert rows get a band");
    require(find(document, "list.row.1")->label.rfind("? ", 0) == 0, "a muted row starts with ?");
    require(!find(document, "list.row.2"), "rows beyond the frame are not declared");
    require(find(document, "list.more") && find(document, "list.more")->label == "+1 more", "hidden rows are counted");
    const auto* grip = find(document, "list.grip");
    require(grip && grip->input == tela::InputPolicy::exclusive, "the grip takes input");
    for(const auto& element : document.elements())
        if(element.id != "list.grip") require(element.input == tela::InputPolicy::passthrough, "everything else lets input through");
    require(tela::list_view_height(frame, theme) > tela::list_view_height({520, 1, ""}, theme), "height grows with rows");

    tela::ListView wide;
    wide.rows.push_back({tela::ListMark::normal, std::vector<std::string>(8, std::string(4096, 'w'))});
    const auto cut = tela::list_view_document(wide, frame, theme);
    require(find(cut, "list.row.0")->label.size() <= 8192, "a very wide row is cut under the declaration limit");
    std::vector<std::string> multibyte(8, "");
    for(auto& column : multibyte) for(int i = 0; i < 1365; ++i) column += "\xe3\x81\x82";
    wide.rows[0].columns = multibyte;
    require(tela::is_valid_utf8(find(tela::list_view_document(wide, frame, theme), "list.row.0")->label),
            "the cut keeps whole UTF-8 characters");
}

void following(const std::filesystem::path& path) {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    ListFileSource source(path, std::chrono::seconds(1));
    auto now = ListFileSource::Clock::time_point{};
    source.poll(now);
    require(source.current_view().rows.empty() && source.current_view().status == "Waiting for the list file", "a missing file waits");
    write(path, exported);
    now += std::chrono::milliseconds(500);
    require(!source.poll(now), "the file is not looked at again within the interval");
    now += std::chrono::milliseconds(600);
    require(source.poll(now) && source.current_view().rows.size() == 3, "a new file is read");
    now += std::chrono::seconds(2);
    require(!source.poll(now), "an unchanged file is not read again");
    write(path, "TELA_LIST_VIEW 1\nbroken\n");
    now += std::chrono::seconds(2);
    require(source.poll(now), "a replaced file is noticed");
    require(source.current_view().rows.empty() && source.current_view().status.rfind("List file is not usable: ", 0) == 0,
            "a broken file shows why and drops the previous rows");
    std::filesystem::remove(path);
    now += std::chrono::seconds(2);
    require(source.poll(now) && source.current_view().status == "Waiting for the list file", "a removed file waits again");
}

void options() {
    const auto parse = [](std::vector<const char*> args) {
        args.insert(args.begin(), "tela_list_overlay");
        return listOverlayOptions(static_cast<int>(args.size()), const_cast<char**>(args.data()));
    };
    const auto chosen = parse({"--font", "f.ttf", "--file", "l.telalist", "--corner", "bottom-left", "--width", "600",
                               "--rows", "20", "--font-size", "14", "--seconds", "30", "--exclude-from-capture"});
    require(chosen.font == "f.ttf" && chosen.file == "l.telalist" && chosen.width == 600 && chosen.rows == 20
            && chosen.font_size == 14.f && chosen.seconds == 30 && chosen.exclude_from_capture
            && chosen.corner == tela::DesktopCorner::bottom_left, "options are read");
    const auto defaults = parse({"--font", "f.ttf", "--file", "l"});
    require(defaults.seconds == 0 && defaults.rows == 12 && !defaults.font_size, "a run lasts until stopped by default");
    for(const auto& bad : std::vector<std::vector<const char*>>{
        {"--file", "l"}, {"--font", "f"}, {"--font", "f", "--file", "l", "--rows", "0"},
        {"--font", "f", "--file", "l", "--width", "100"}, {"--font", "f", "--file", "l", "--font-size", "7"},
        {"--font", "f", "--file", "l", "--seconds", "0"}, {"--font", "f", "--file", "l", "--rows", "2x"},
        {"--font", "f", "--file", "l", "--color", "red"}, {"--font", "f", "--file"},
    }) {
        rejects([&] { parse(bad); }, "invalid options are rejected");
    }
}
}

int main(int argc, char** argv) {
    if(argc != 2) return 1;
    const std::filesystem::path path = argv[1];
    try {
        parsing();
        rejecting();
        limits();
        composition();
        following(path);
        options();
        return 0;
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        return 2;
    }
}
