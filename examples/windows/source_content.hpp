// @spec Dedicated source viewer
#pragma once
#include <tela/source_view.hpp>
#include <tela/runtime.hpp>
#include <optional>

// @implements SPEC-TL-SOURCE-VIEW
class SourceContent {
public:
    explicit SourceContent(tela::SourceView source);
    void refresh(tela::Runtime&);
private:
    tela::SourceView source_;
    struct Revision {
        int width, height;
        float dpi, font, line_height;
        bool visible;
        std::size_t first, column;
        bool operator==(const Revision&) const = default;
    };
    std::optional<Revision> declared_;
};
