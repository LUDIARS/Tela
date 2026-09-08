#pragma once
#include <string>
// @spec Overlay lifecycle
struct Options {std::string font,pipe="tela-scene",file="transitions.tela",report;bool probe{};int seconds{};};
Options options(int argc, char** argv);
