// @implements SPEC-TL-LIST-VIEW
// @spec List view
#pragma once
#include <tela/document.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace tela {
// How a row asks to be drawn. Tela does not know what the rows mean; the writer decides
// which ones need attention (alert), which are ordinary and which are less certain (muted).
enum class ListMark { normal, alert, muted };

struct ListRow {
    ListMark mark{ListMark::normal};
    std::vector<std::string> columns;
    bool operator==(const ListRow&) const = default;
};

// A read-only list another program writes as `TELA_LIST_VIEW 1`: a title, one status line and
// marked rows. Tela only reads it; the writer remains the source of truth.
struct ListView {
    std::string title, status;
    std::vector<ListRow> rows;
    bool operator==(const ListView&) const = default;
};

inline constexpr std::size_t list_view_max_rows = 256;
inline constexpr std::size_t list_view_max_columns = 8;
inline constexpr std::size_t list_view_max_value_bytes = 4096;
inline constexpr std::uintmax_t list_view_max_file_bytes = 4 * 1024 * 1024;

const char* list_mark_name(ListMark) noexcept;
// Throws std::invalid_argument for a name other than normal / alert / muted.
ListMark list_mark_from_name(std::string_view);

// Parses `TELA_LIST_VIEW 1`. An invalid text throws std::invalid_argument and yields no partial list.
ListView parse_list_view(std::string_view text);
// Reads and parses the file. A file above list_view_max_file_bytes is rejected before reading.
ListView load_list_view(const std::filesystem::path&);

// Logical size of the surface, the grip it uses and how many rows it shows at once.
struct ListViewFrame {
    float width{520};
    std::size_t visible_rows{12};
    std::string grip;
    bool operator==(const ListViewFrame&) const = default;
};

// Height the frame needs for its header and visible rows at the theme's line height.
float list_view_height(const ListViewFrame&, const Theme&);

// Declares a translucent backdrop, the title, the status line, the grip (exclusive) and up to
// `visible_rows` rows. Alert rows get a red band and a leading "!" so they read without colour
// too; muted rows a leading "?". Rows that do not fit are counted in a final "+N more" line.
// Everything except the grip lets input through to the windows below.
Document list_view_document(const ListView&, const ListViewFrame&, const Theme& theme = {});
}
