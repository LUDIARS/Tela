// @spec Dedicated source viewer
#include <tela/source_view.hpp>
#include <algorithm>
#include <stdexcept>

namespace tela {
namespace {
// Validate canonical UTF-8 including overlong sequences, surrogates and the Unicode limit.
std::size_t utf8_character_bytes(std::string_view text, std::size_t at) {
    const auto lead = static_cast<unsigned char>(text[at]);
    if(lead < 0x80) return 1;
    const std::size_t count = lead >= 0xc2 && lead <= 0xdf ? 2 :
        lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
    if(!count || at + count > text.size()) throw std::invalid_argument("Source must be valid UTF-8");
    unsigned code = lead & (0x7f >> count);
    for(std::size_t i = 1; i < count; ++i) {
        const auto next = static_cast<unsigned char>(text[at + i]);
        if((next & 0xc0) != 0x80) throw std::invalid_argument("Source must be valid UTF-8");
        code = (code << 6) | (next & 0x3f);
    }
    if((count == 2 && code < 0x80) || (count == 3 && code < 0x800) ||
       (count == 4 && code < 0x10000) || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
        throw std::invalid_argument("Source must be valid UTF-8");
    return count;
}
}

const std::string& SourceView::name() const noexcept { return name_; }
std::size_t SourceView::line_count() const noexcept { return lines_.size(); }
std::size_t SourceView::first_line() const noexcept { return first_; }
std::size_t SourceView::target_line() const noexcept { return target_; }
std::size_t SourceView::column() const noexcept { return column_; }

SourceView::SourceView(std::string name, std::string_view text, std::size_t target_line)
    : name_(std::move(name)), target_(target_line) {
    if(name_.size() > 4096) throw std::invalid_argument("Source path is too long");
    if(text.size() > source_max_bytes) throw std::invalid_argument("Source exceeds 8 MiB");
    if(text.starts_with("\xef\xbb\xbf")) text.remove_prefix(3);
    std::string line;
    std::size_t columns = 0, line_bytes = 0;
    const auto finish = [&] {
        if(lines_.size() >= 200000) throw std::invalid_argument("Source exceeds 200000 lines");
        max_columns_ = std::max(max_columns_, columns);
        lines_.push_back(std::move(line));
        line.clear(); columns = 0; line_bytes = 0;
    };
    for(std::size_t at = 0; at < text.size();) {
        const char ch = text[at];
        if(ch == '\r' || ch == '\n') {
            if(ch == '\r' && at + 1 < text.size() && text[at + 1] == '\n') ++at;
            ++at; finish(); continue;
        }
        const auto length = utf8_character_bytes(text, at);
        line_bytes += length;
        if(line_bytes > 65536) throw std::invalid_argument("Source line exceeds 65536 bytes");
        if(ch == '\t') {
            const auto spaces = 4 - columns % 4;
            line.append(spaces, ' '); columns += spaces;
        } else {
            if(static_cast<unsigned char>(ch) < 32 || ch == 127)
                throw std::invalid_argument("Source contains unsupported control characters");
            line.append(text.substr(at, length)); ++columns;
        }
        at += length;
    }
    finish();
    if(!target_ || target_ > lines_.size()) throw std::invalid_argument("Requested source line does not exist");
    first_ = target_;
}

std::string SourceView::slice(std::size_t line, std::size_t columns) const {
    if(!line || line > lines_.size()) throw std::out_of_range("Source line does not exist");
    const auto& text = lines_[line - 1];
    std::size_t at = 0, skipped = 0;
    while(at < text.size() && skipped < column_) { at += utf8_character_bytes(text, at); ++skipped; }
    const auto begin = at;
    for(std::size_t count = 0; at < text.size() && count < std::min<std::size_t>(columns, 2048); ++count)
        at += utf8_character_bytes(text, at);
    return text.substr(begin, at - begin);
}

void SourceView::navigate(SourceAction action, std::size_t page_rows, std::size_t page_columns) {
    const auto rows = std::clamp<std::size_t>(page_rows, 1, 200);
    const auto columns = std::clamp<std::size_t>(page_columns, 1, 24);
    switch(action) {
    case SourceAction::previous: first_ -= std::min(first_ - 1, rows); break;
    case SourceAction::next: first_ += std::min(lines_.size() - first_, rows); break;
    case SourceAction::left: column_ -= std::min(column_, columns); break;
    case SourceAction::right: column_ = std::min(column_ + columns, max_columns_ ? max_columns_ - 1 : 0); break;
    case SourceAction::first: first_ = 1; column_ = 0; break;
    case SourceAction::target: first_ = target_; column_ = 0; break;
    }
}
}
