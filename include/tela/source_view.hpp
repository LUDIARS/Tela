// @implements SPEC-TL-SOURCE-VIEW
// @spec Dedicated source viewer
#pragma once
#include <tela/document.hpp>
#include <tela/host_contract.hpp>
#include <filesystem>
#include <string_view>

namespace tela {
inline constexpr std::size_t source_max_bytes = 8 * 1024 * 1024;
enum class SourceAction { previous, next, left, right, first, target };

// Owns a read-only snapshot and navigation; never performs file I/O.
// @implements SPEC-TL-SOURCE-VIEW
class SourceView {
public:
    SourceView(std::string name, std::string_view utf8, std::size_t target_line = 1);
    const std::string& name() const noexcept;
    std::size_t line_count() const noexcept;
    std::size_t first_line() const noexcept;
    std::size_t target_line() const noexcept;
    std::size_t column() const noexcept;
    std::string slice(std::size_t line, std::size_t columns) const;
    void navigate(SourceAction, std::size_t page_rows, std::size_t page_columns = 24);
private:
    std::string name_;
    std::vector<std::string> lines_;
    std::size_t first_{1}, target_{1}, column_{}, max_columns_{};
};
SourceView load_source(const std::filesystem::path&, std::size_t target_line = 1);
std::size_t source_page_rows(const Viewport&, const Theme&);
std::size_t source_page_columns(const Viewport&, const Theme&);
Document source_document(const SourceView&, const Viewport&, const Theme&,
    std::function<void(SourceAction)> navigate);
}
