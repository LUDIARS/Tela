// @implements SPEC-TL-LIST-VIEW
// @spec List view
#include "list_file_source.hpp"
#include <tela/utf8.hpp>
#include <exception>
#include <system_error>

namespace {
// System and filesystem messages can be in the local code page on Windows. Only text that is
// already UTF-8 reaches the declaration; anything else is replaced by the fixed reason.
std::string readable_reason(const std::string& detail, const std::string& fallback) {
    return tela::is_valid_utf8(detail) && detail.size() <= 512 ? detail : fallback;
}
}

tela::ListView unavailable_list(const std::filesystem::path&, const std::string& reason) {
    tela::ListView view;
    view.title = "Tela list";
    view.status = reason;
    return view;
}

ListFileSource::ListFileSource(std::filesystem::path file, Clock::duration interval)
    : file_(std::move(file)), interval_(interval), view_(unavailable_list(file_, "Waiting for the list file")) {}

const tela::ListView& ListFileSource::current_view() const noexcept { return view_; }

void ListFileSource::show(tela::ListView next) {
    if(next == view_) return;
    view_ = std::move(next);
    changed_ = true;
}

bool ListFileSource::poll(Clock::time_point now) {
    if(!checked_ || now - *checked_ >= interval_) {
        checked_ = now;
        std::error_code error;
        const auto written = std::filesystem::last_write_time(file_, error);
        const auto size = error ? 0 : std::filesystem::file_size(file_, error);
        if(error) {
            stamp_.reset();
            show(unavailable_list(file_, "Waiting for the list file"));
        } else if(const Stamp stamp{written, size}; stamp_ != stamp) {
            stamp_ = stamp;
            try {
                show(tela::load_list_view(file_));
            } catch(const std::exception& failure) {
                // Read again on the next change only; a broken file stays broken until replaced.
                show(unavailable_list(file_, "List file is not usable: " + readable_reason(failure.what(), "read failed")));
            }
        }
    }
    const bool changed = changed_;
    changed_ = false;
    return changed;
}
