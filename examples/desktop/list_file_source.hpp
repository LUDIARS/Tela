// @implements SPEC-TL-LIST-VIEW
// @spec List view
#pragma once
#include <tela/list_view.hpp>
#include <chrono>
#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
// Follows one list file the writer replaces from time to time. It looks at the file's time and
// size at most once per interval and reads it again only when they change. A file that is
// missing or invalid is shown as such: the previous rows are never kept on screen as if current.
class ListFileSource {
public:
    using Clock = std::chrono::steady_clock;
    ListFileSource(std::filesystem::path file, Clock::duration interval);
    // True when current_view() changed since the last call.
    bool poll(Clock::time_point now);
    const tela::ListView& current_view() const noexcept;
private:
    struct Stamp {
        std::filesystem::file_time_type written;
        std::uintmax_t size{};
        bool operator==(const Stamp&) const = default;
    };
    void show(tela::ListView next);
    std::filesystem::path file_;
    Clock::duration interval_;
    std::optional<Clock::time_point> checked_;
    std::optional<Stamp> stamp_;
    tela::ListView view_;
    bool changed_{};
};
// The list shown in place of the file's rows when it cannot be used.
tela::ListView unavailable_list(const std::filesystem::path& file, const std::string& reason);
