#pragma once
#include <tela/placement.hpp>
#include <string>
// @spec Overlay lifecycle
// @spec Spec view
// @spec Overlay placement
struct Options {std::string font,pipe="tela-scene",file="transitions.tela",sceneOverlay,specView,graph,report;bool probe{},transitionsGiven{},attachProbeTarget{};int seconds{},fontSize{};tela::Placement placement{tela::Placement::inside};};
Options options(int argc, char** argv);
