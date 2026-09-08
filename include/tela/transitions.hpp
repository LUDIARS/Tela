// @implements SPEC-TL-TRANSITIONS
// @spec Transition authoring
#pragma once
#include <tela/bridge.hpp>
#include <filesystem>

namespace tela {
struct Transition {
    std::string id, source, destination, condition, object_id;
    bool operator==(const Transition&) const = default;
};
class Transitions {
public:
    void set(Transition value);
    void erase(const std::string& id);
    const std::vector<Transition>& entries() const noexcept { return entries_; }
    void load(const std::filesystem::path&);
    void save(const std::filesystem::path&) const;
private:
    std::vector<Transition> entries_;
};
// Application layer; the core runtime knows nothing about screens or Unity IDs.
Document transition_document(const Transitions&, const std::vector<Anchor>&, const Viewport&,
    std::function<void(const Transition&)> edit, std::function<void(const Anchor&)> add);
}
