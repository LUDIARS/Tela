#pragma once
// @implements SPEC-TL-WEB
// @spec Web rendering
#include "image_registry.hpp"
#include <tela/web_session.hpp>
#include <string>

namespace tela::web {
struct EncodedFrame {
    std::string body;    // frame members after "revision", ending with the closing brace
    bool truncated{};    // elements were left out to stay inside the limits
};
// The placed result in logical (CSS) pixels: theme, focus, and every element with its kind,
// bounds, clip, text, shapes and display state. Secret values leave as bullets.
EncodedFrame encode_frame(const Runtime&, ImageRegistry&, const WebLimits&);
// Header of a frame message; `body` from encode_frame completes it.
std::string frame_header(std::uint64_t revision);
const char* kind_name(ElementKind) noexcept;
}
