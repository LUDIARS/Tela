#pragma once
#include <string>
// @spec Overlay lifecycle
// @spec Spec view
struct Options {std::string font,pipe="tela-scene",file="transitions.tela",sceneOverlay,specView,report;bool probe{},transitionsGiven{},attachProbeTarget{};int seconds{};};
Options options(int argc, char** argv);
