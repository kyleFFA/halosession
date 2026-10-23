#pragma once
#include "imgui.h"
#include <d3d11.h>
#include <functional>
namespace SessionSketch {
struct State {ImVec2 position{12,12},size{810,900};int tab=0;bool initialized=false;int contentRevision=0;};
void addFonts();
void beginFrame();
void endFrame();
void prepare(ID3D11Device* device);
void reset();
bool ready();
void clamp(State& state,ImVec2 screen);
bool hit(const State& state,ImVec2 point);
void backdrop(const State& state);
ImFont* font();
// Art is drawn without an input window; only the cloud-center controls capture input.
void render(State& state,ImVec2 screen,bool inputs,bool& open,const std::function<void(int)>& contents);
}
