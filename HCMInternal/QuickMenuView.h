#pragma once
#include "SessionSketch.h"
#include <array>
#include <string>
namespace quickmenu {
struct WheelView {std::array<std::string,5> labels;std::array<bool,5> enabled;int selected=-1;};
struct Geometry {ImVec2 center,radius;float font;};
inline Geometry geometry(const SessionSketch::State& s) {return {{s.position.x+s.size.x*.54f,s.position.y+s.size.y*.52f},{s.size.x*.27f,s.size.y*.36f},20.f*s.size.x/900.f};}
void renderWheel(const SessionSketch::State& s,const WheelView& view);
}
