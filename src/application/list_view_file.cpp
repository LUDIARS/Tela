// @implements SPEC-TL-LIST-VIEW
// @spec List view
#include <tela/list_view.hpp>
#include <tela/utf8.hpp>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>

namespace tela {
namespace {
constexpr std::string_view magic = "TELA_LIST_VIEW 1";

std::vector<std::string_view> split_fields(std::string_view line, char separator) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    for(std::size_t at = 0; at <= line.size(); ++at) {
        if(at == line.size() || line[at] == separator) {
            parts.push_back(line.substr(start, at - start));
            start = at + 1;
        }
    }
    return parts;
}

// Values are shown as one line: no control characters (the writer replaces tabs and newlines).
std::string read_value(std::string_view text) {
    if(text.size() > list_view_max_value_bytes) throw std::invalid_argument("List value exceeds 4096 bytes");
    if(!is_valid_utf8(text)) throw std::invalid_argument("List value must be valid UTF-8");
    for(const char ch : text) {
        const auto byte = static_cast<unsigned char>(ch);
        if(byte < 0x20 || byte == 0x7f) throw std::invalid_argument("List value contains control characters");
    }
    return std::string(text);
}

std::string read_single(const std::vector<std::string_view>& fields, const char* record) {
    if(fields.size() != 2) throw std::invalid_argument(std::string("List ") + record + " takes one value");
    return read_value(fields[1]);
}

ListRow read_row(const std::vector<std::string_view>& fields) {
    if(fields.size() < 3) throw std::invalid_argument("List row needs a mark and at least one column");
    if(fields.size() - 2 > list_view_max_columns) throw std::invalid_argument("List row exceeds 8 columns");
    ListRow result;
    result.mark = list_mark_from_name(fields[1]);
    for(std::size_t i = 2; i < fields.size(); ++i) result.columns.push_back(read_value(fields[i]));
    return result;
}
}

const char* list_mark_name(ListMark mark) noexcept {
    switch(mark) {
    case ListMark::alert: return "alert";
    case ListMark::muted: return "muted";
    case ListMark::normal: break;
    }
    return "normal";
}

ListMark list_mark_from_name(std::string_view name) {
    if(name == "normal") return ListMark::normal;
    if(name == "alert") return ListMark::alert;
    if(name == "muted") return ListMark::muted;
    throw std::invalid_argument("List row mark must be normal, alert or muted");
}

ListView parse_list_view(std::string_view text) {
    auto lines = split_fields(text, '\n');
    // A final LF ends the last line instead of starting an empty one.
    if(!lines.empty() && lines.back().empty()) lines.pop_back();
    if(lines.empty() || lines.front() != magic) throw std::invalid_argument("Unsupported list view file");
    std::optional<std::string> title, status;
    ListView view;
    for(std::size_t i = 1; i < lines.size(); ++i) {
        const auto line = lines[i];
        if(line.empty()) continue;
        const auto fields = split_fields(line, '\t');
        const auto record = fields.front();
        if(record == "title") {
            if(title) throw std::invalid_argument("Duplicate list title");
            title = read_single(fields, "title");
        } else if(record == "status") {
            if(status) throw std::invalid_argument("Duplicate list status");
            status = read_single(fields, "status");
        } else if(record == "row") {
            if(view.rows.size() == list_view_max_rows) throw std::invalid_argument("List exceeds 256 rows");
            view.rows.push_back(read_row(fields));
        } else {
            throw std::invalid_argument("Unknown list view record");
        }
    }
    if(!title || !status) throw std::invalid_argument("List title and status are required");
    view.title = std::move(*title);
    view.status = std::move(*status);
    return view;
}

ListView load_list_view(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path) > list_view_max_file_bytes) throw std::invalid_argument("List view file exceeds 4 MiB");
    std::ifstream input(path, std::ios::binary);
    if(!input) throw std::runtime_error("List view file cannot be opened");
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if(input.bad()) throw std::runtime_error("List view read failed");
    return parse_list_view(text);
}
}
