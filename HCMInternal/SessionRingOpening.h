#pragma once
#include "imgui.h"
namespace SessionRingOpening {
// Measured cyan inner rim of session-ringworld-round.png, not the cloud/texture bounds.
// SHA256 C4F14570B885A49E1DC4053204FCE9FD3C8842195DD0D21D966997F1C60C59EA.
// 309 retained radial edge samples. See tools/measure-ring-opening.py and its JSON output.
inline constexpr ImVec2 center{.500179154f,.475353879f};
inline constexpr float radius=.311131824f;
inline constexpr float wheelRadius=.289f; // About 6–8% inset from the measured inner rim.
}
