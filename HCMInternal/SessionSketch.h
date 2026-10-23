#pragma once
#include "imgui.h"
#include <d3d11.h>
#include <functional>
#include <array>
#include "SessionAppearanceDefaults.h"
#include <filesystem>
#include <string>
#include <vector>
namespace SessionSketch {
struct WheelAppearance {ImVec4 background=SessionAppearanceDefaults::wheelBackground,segment=SessionAppearanceDefaults::wheelSegment,selected=SessionAppearanceDefaults::wheelSelected;};
struct State {ImVec2 position=SessionAppearanceDefaults::referencePosition,size=SessionAppearanceDefaults::menuSize;int tab=0;bool initialized=false;int contentRevision=0;bool animateClouds=SessionAppearanceDefaults::animateClouds;float backgroundOpacity=SessionAppearanceDefaults::backgroundOpacity;WheelAppearance wheel;std::array<float,4> contentHeights{},contentWidths{};int measuredRevision=-1;};
// The art, controls and wheel use one circle even when given an old rectangular size.
struct Layout {ImVec2 center,artMin,artMax,controlsMin,controlsSize;float diameter;};
Layout layout(const State& state);
struct NavigationNode {ImVec2 center,minimum,maximum;float radius;bool left;};
NavigationNode navigationNode(const State& state,int index);
// Skin APIs are renderer-thread operations; requests commit at beginFrame.
struct SkinOpening {ImVec2 center;float radius,wheelRadius;};
struct SkinChoice {std::string id,name;bool nodeTexture=false,plaqueTexture=false;};
SkinOpening skinOpening();
SkinChoice activeSkin();
std::vector<SkinChoice> availableSkins();
void setSkinDirectory(const std::filesystem::path& directory);
void refreshSkins();
void requestSkin(std::string id); // Applied only at the next beginFrame, before draw commands reference textures.
std::string skinMessage();
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
