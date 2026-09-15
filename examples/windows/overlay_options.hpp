#pragma once
#include <string>
// @spec Overlay lifecycle
struct Options {std::string font,pipe="tela-scene",file="transitions.tela",sceneOverlay,report;bool probe{},transitionsGiven{};int seconds{};};
Options options(int argc, char** argv);
