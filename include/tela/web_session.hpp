// @implements SPEC-TL-WEB
// @spec Web rendering
#pragma once
#include <tela/runtime.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tela {
// Upper bounds of what one browser connection may send and receive.
struct WebLimits {
    std::size_t frame_bytes{2u << 20};  // one frame message
    std::size_t elements{4096};         // elements per frame
    std::size_t input_bytes{16384};     // one input message
    std::size_t text_bytes{4096};       // committed text in one input message
    std::size_t image_bytes{1u << 20};  // decoded RGBA of one image; larger images are not sent
};
// Serves one Runtime to one browser page. It has no transport: the embedder carries the
// strings (WebSocket, pipe, stdio) and decides who may connect. The browser receives only the
// placed result; secret field contents are replaced by one bullet per character.
class WebSession {
public:
    explicit WebSession(Runtime&, WebLimits = {});
    ~WebSession();
    WebSession(const WebSession&) = delete;
    WebSession& operator=(const WebSession&) = delete;
    // One input message (pointer / key / text / resize / focus). False when it was malformed,
    // over a limit or stale; the reason is in last_error() and nothing reached the runtime.
    bool receive(std::string_view message);
    const std::string& last_error() const noexcept;
    // Messages to send now: images the page has not seen, then a frame when what it shows
    // changed. Empty while nothing changed or before the first resize.
    std::vector<std::string> poll();
    // The page went away: cancels gestures and forgets what was sent, so a new page gets everything.
    void disconnect();
    std::uint64_t revision() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
