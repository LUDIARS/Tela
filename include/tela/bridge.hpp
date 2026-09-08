// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
#pragma once
#include <tela/runtime.hpp>
#include <span>
#include <variant>

namespace tela {
struct Anchor { std::string object_id, label; int desktop_x{}, desktop_y{}; bool visible{}; };
enum class BridgeKind : std::uint16_t { hello=1, viewport=2, pointer=3, selection=4, anchors=5, heartbeat=6 };
struct BridgeMessage {
    BridgeKind kind{};
    std::uint64_t generation{}, sequence{}, revision{}, host_window{};
    std::string host, view;
    Viewport viewport;
    HostPointerEvent pointer;
    std::vector<std::string> selection;
    std::vector<Anchor> anchors;
};
constexpr std::size_t bridge_max_frame = 65536;
// Payload excludes the 4-byte little-endian transport length prefix.
BridgeMessage decode_bridge(std::span<const unsigned char>);
std::vector<unsigned char> encode_bridge(const BridgeMessage&);
class BridgeSession {
public:
    explicit BridgeSession(Runtime& runtime) : runtime_(runtime) {}
    bool accept(const BridgeMessage&);
    void disconnect();
    const std::vector<Anchor>& anchors() const noexcept { return anchors_; }
    const std::vector<std::string>& selection() const noexcept { return selection_; }
    std::uintptr_t host_window() const noexcept { return window_; }
private:
    Runtime& runtime_;
    std::uint64_t generation_{}, sequence_{};
    std::uintptr_t window_{};
    std::string host_, view_;
    std::vector<Anchor> anchors_;
    std::vector<std::string> selection_;
};
}
