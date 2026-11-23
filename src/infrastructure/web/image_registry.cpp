// @spec SPEC-TL-WEB
#include "image_registry.hpp"
#include "json_writer.hpp"

namespace tela::web {
namespace {
constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void base64(std::string& out, const unsigned char* data, std::size_t size) {
    for (std::size_t i = 0; i < size; i += 3) {
        const std::uint32_t a = data[i], b = i + 1 < size ? data[i + 1] : 0, c = i + 2 < size ? data[i + 2] : 0;
        const std::uint32_t triple = (a << 16) | (b << 8) | c;
        out.push_back(alphabet[(triple >> 18) & 63]);
        out.push_back(alphabet[(triple >> 12) & 63]);
        out.push_back(i + 1 < size ? alphabet[(triple >> 6) & 63] : '=');
        out.push_back(i + 2 < size ? alphabet[triple & 63] : '=');
    }
}
unsigned char straight(unsigned char premultiplied, unsigned char alpha) {
    if (alpha == 0) return 0;
    const unsigned value = (premultiplied * 255u + alpha / 2u) / alpha;
    return static_cast<unsigned char>(value > 255 ? 255 : value);
}
}
std::string image_rgba_base64(const Image& image) {
    std::vector<unsigned char> rgba(image.pixels.size());
    for (std::size_t i = 0; i + 3 < image.pixels.size(); i += 4) {
        const auto a = image.pixels[i + 3];
        rgba[i] = straight(image.pixels[i + 2], a);
        rgba[i + 1] = straight(image.pixels[i + 1], a);
        rgba[i + 2] = straight(image.pixels[i], a);
        rgba[i + 3] = a;
    }
    std::string out;
    out.reserve((rgba.size() + 2) / 3 * 4);
    base64(out, rgba.data(), rgba.size());
    return out;
}
std::string ImageRegistry::id(const std::shared_ptr<const Image>& image) {
    if (!image || image->pixels.size() > max_bytes_) return {};
    auto& entry = entries_[image.get()];
    // A freed image's address may be reused by a different image; that one needs its own ID.
    if (!entry.id.empty() && entry.image.lock() == image) return entry.id;
    entry.image = image;
    entry.id = "i" + std::to_string(++next_);
    JsonWriter message;
    message.begin_object().key("type").string("image").key("id").string(entry.id)
        .key("width").integer(static_cast<std::uint64_t>(image->width))
        .key("height").integer(static_cast<std::uint64_t>(image->height))
        .key("rgba").string(image_rgba_base64(*image)).end_object();
    pending_.push_back(message.release());
    return entry.id;
}
std::vector<std::string> ImageRegistry::take_pending() {
    std::vector<std::string> out;
    out.swap(pending_);
    return out;
}
void ImageRegistry::prune() {
    for (auto it = entries_.begin(); it != entries_.end();)
        it = it->second.image.expired() ? entries_.erase(it) : std::next(it);
}
void ImageRegistry::reset() {
    entries_.clear();
    pending_.clear();
}
}
