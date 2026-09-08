#pragma once
#include <tela/bridge.hpp>
#include <memory>
#include <optional>

namespace tela {
struct PipeEvent {
    std::uint64_t connection{};
    std::uint32_t client_pid{};
    std::optional<BridgeMessage> message; // null = disconnected
    std::string error;
};
// Current-user DACL, remote clients rejected, bounded delivery, cancellation on destruction.
class WindowsPipe {
public:
    explicit WindowsPipe(const std::string& name);
    ~WindowsPipe();
    WindowsPipe(const WindowsPipe&)=delete;
    WindowsPipe& operator=(const WindowsPipe&)=delete;
    std::vector<PipeEvent> drain();
    std::uintptr_t wake_handle() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
