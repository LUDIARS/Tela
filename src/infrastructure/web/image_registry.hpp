#pragma once
// @implements SPEC-TL-WEB
// @spec Web rendering
#include <tela/drawing.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tela::web {
// Gives each canvas image an ID and sends its pixels to the page once. Images are compared by
// identity, as Drawing does, so redeclaring the same image object costs nothing.
class ImageRegistry {
public:
    explicit ImageRegistry(std::size_t max_bytes) : max_bytes_(max_bytes) {}
    // Empty when the image exceeds the byte limit; the page then draws nothing for it.
    std::string id(const std::shared_ptr<const Image>&);
    // Image messages registered since the last call.
    std::vector<std::string> take_pending();
    // Forgets images that no declaration holds any more.
    void prune();
    void reset();
private:
    struct Entry { std::weak_ptr<const Image> image; std::string id; };
    std::size_t max_bytes_;
    std::unordered_map<const Image*, Entry> entries_;
    std::vector<std::string> pending_;
    std::uint64_t next_{};
};
// RGBA (straight alpha) base64 of a premultiplied BGRA image, as canvas ImageData expects.
std::string image_rgba_base64(const Image&);
}
