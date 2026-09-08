// @implements SPEC-TL-TRANSITIONS
// @spec Transition authoring
#pragma once
#include <tela/transitions.hpp>
#include <functional>
#include <memory>

// Modeless system text-entry surface; persistence and validation stay in Tela's
// transition application layer. Does not block IPC/Scene view processing.
class TransitionEditor {
public:
    TransitionEditor();
    ~TransitionEditor();
    void open(tela::Transition, std::function<void(tela::Transition)> save);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
