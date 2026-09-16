#pragma once
#include <tela/placement.hpp>
#include <tela/runtime.hpp>
#include <windows.h>
// @spec Overlay placement
// Reports the viewport for a standalone target the session attached to. `inside` follows the
// host's client area; the other placements sit beside the window at the content's own size,
// so read-only content is readable at 1:1 instead of being shrunk into the host.
void synchronizeAttached(tela::Runtime& runtime, HWND target, tela::Placement placement,
                         float width, float height);
