#pragma once
#include "SessionSketch.h"
#include "SessionRingOpening.h"
#include <array>
#include <string>
namespace quickmenu {
struct WheelView {std::array<std::string,5> labels;std::array<bool,5> enabled;int selected=-1;};
struct Geometry {ImVec2 center,radius;float font;};
inline Geometry geometry(const SessionSketch::State& s) {const auto l=SessionSketch::layout(s);const float r=l.diameter*SessionSketch::skinOpening().wheelRadius;return {l.center,{r,r},20.f*l.diameter/900.f*s.textScale};}
void renderWheel(const SessionSketch::State& s,const WheelView& view);
}
