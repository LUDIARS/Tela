#pragma once
#include <tela/runtime.hpp>
#include <windows.h>
// @spec Overlay lifecycle
void synchronizeProbe(tela::Runtime& runtime, HWND target);
tela::Document probeDocument(int& clicks, tela::Runtime& runtime);
