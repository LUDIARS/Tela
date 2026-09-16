// @spec Dedicated source viewer
#include "source_content.hpp"

SourceContent::SourceContent(tela::SourceView source) : source_(std::move(source)) {}

void SourceContent::refresh(tela::Runtime& runtime) {
    const auto& view = runtime.viewport();
    const auto& theme = runtime.theme();
    const Revision revision{view.width, view.height, view.dpi_scale, theme.font_size,
        theme.line_height, view.visible, source_.first_line(), source_.column()};
    if(declared_ == revision) return;
    const auto rows = tela::source_page_rows(view, theme);
    const auto columns = tela::source_page_columns(view, theme);
    runtime.document(tela::source_document(source_, view, theme,
        [this, rows, columns](tela::SourceAction action) { source_.navigate(action, rows, columns); }));
    declared_ = revision;
}
