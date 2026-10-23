#pragma once
#include "imgui.h"
// User-approved appearance defaults captured from the running seasonal preview.
// Only appearance values are published; no live addresses, process data or binding XML.
namespace SessionAppearanceDefaults {
inline constexpr ImVec2 menuSize={1350.0f,900.0f};
inline constexpr ImVec2 referencePosition={1220.0f,617.0f};
inline constexpr ImVec2 referenceViewport={3840.0f,2160.0f};
inline constexpr ImVec2 normalizedFrameCenter={0.493489583f,0.493981481f};
inline constexpr bool animateClouds=true;
inline constexpr float backgroundOpacity=.50f; // Explicit override of the captured 65%.
inline constexpr ImVec4 wheelBackground={0.873417735f,0.873417735f,0.873417735f,0.389999986f};
inline constexpr ImVec4 wheelSegment={0.801687777f,1.0f,0.997489691f,0.0799999982f};
inline constexpr ImVec4 wheelSelected={0.717299581f,0.938319981f,1.0f,0.579999983f};
inline constexpr char skinId[]="default"; // Captured Session V1.
inline constexpr double cloudSpeedMultiplier=1.10; // Relative speed; travel amplitude unchanged.
}
