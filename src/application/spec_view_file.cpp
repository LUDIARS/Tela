// @spec SPEC-TL-SPEC-VIEW
#include <tela/spec_view.hpp>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace tela {
namespace {
constexpr std::uintmax_t max_file_bytes = 4 * 1024 * 1024;

void finish_row(std::istringstream& row) {
    row >> std::ws;
    if(!row.eof()) throw std::invalid_argument("Trailing spec view data");
}
bool read_flag(std::istringstream& row) {
    int flag = -1;
    if(!(row >> flag) || (flag != 0 && flag != 1)) throw std::invalid_argument("Spec view visibility must be 0 or 1");
    return flag == 1;
}
SpecViewInfo read_view(std::istringstream& row) {
    SpecViewInfo info;
    if(!(row >> std::quoted(info.project) >> std::quoted(info.version) >> info.width >> info.height))
        throw std::invalid_argument("Invalid spec view header");
    finish_row(row);
    return info;
}
SpecViewGroup read_group(std::istringstream& row) {
    SpecViewGroup group;
    if(!(row >> std::quoted(group.id) >> std::quoted(group.name))) throw std::invalid_argument("Invalid spec view group");
    group.visible = read_flag(row);
    finish_row(row);
    return group;
}
SpecViewCard read_card(std::istringstream& row) {
    SpecViewCard card;
    auto& b = card.bounds;
    if(!(row >> std::quoted(card.group_id) >> std::quoted(card.code) >> std::quoted(card.title)
             >> std::quoted(card.status) >> card.version >> b.x >> b.y >> b.width >> b.height))
        throw std::invalid_argument("Invalid spec view card");
    finish_row(row);
    return card;
}
}

SpecView load_spec_view(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path) > max_file_bytes) throw std::invalid_argument("Spec view file exceeds 4 MiB");
    std::ifstream input(path, std::ios::binary);
    std::string line;
    if(!input || !std::getline(input, line) || line != "TELA_SPEC_VIEW 1")
        throw std::invalid_argument("Unsupported spec view file");
    std::optional<SpecViewInfo> info;
    std::vector<SpecViewGroup> groups;
    std::vector<SpecViewCard> cards;
    while(std::getline(input, line)) {
        if(line.empty()) continue;
        std::istringstream row(line);
        std::string record;
        row >> record;
        if(record == "view") {
            if(info) throw std::invalid_argument("Duplicate spec view header");
            info = read_view(row);
        } else if(record == "group") {
            groups.push_back(read_group(row));
        } else if(record == "card") {
            cards.push_back(read_card(row));
        } else {
            throw std::invalid_argument("Unknown spec view record");
        }
        // Stop reading an oversized file early; the model repeats the limits for in-memory callers.
        if(groups.size() > spec_view_max_groups || cards.size() > spec_view_max_cards)
            throw std::invalid_argument("Spec view limit exceeded");
    }
    if(input.bad()) throw std::runtime_error("Spec view read failed");
    if(!info) throw std::invalid_argument("Spec view header is required");
    return SpecView(std::move(*info), std::move(groups), std::move(cards));
}
}
