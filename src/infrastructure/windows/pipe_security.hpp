// @implements SPEC-TL-BRIDGE
// @spec Unity bridge
#pragma once
// @spec SPEC-TL-BRIDGE
#include <windows.h>
#include <vector>

namespace tela::windows {
class PipeSecurity {
public:
    PipeSecurity();
    ~PipeSecurity();
    SECURITY_ATTRIBUTES* attributes() noexcept { return &attributes_; }
private:
    PSECURITY_DESCRIPTOR descriptor_{};
    SECURITY_ATTRIBUTES attributes_{sizeof(SECURITY_ATTRIBUTES),nullptr,FALSE};
};
}
