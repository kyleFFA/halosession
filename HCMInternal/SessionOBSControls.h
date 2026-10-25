#pragma once
#include "imgui.h"
#include "SessionOBSBypass.h"
namespace sessionobs {
inline bool controls(bool& requested){
 ImGui::PushID("obs-capture");
 const bool changed=ImGui::Checkbox("Bypass OBS Capture",&requested);
 ImGui::TextWrapped("%s",text(status.load()));
 ImGui::TextWrapped("OBS Game Capture only, with Capture third-party overlays OFF. Display/Window Capture and other game render-hook effects are not excluded. Verify a recording; Session cannot verify what OBS records.");
 ImGui::TextWrapped("Halo 3 Map grid uses the post-bloom frame renderer while this route is verified. Its bloom can look different; saved Grid without bloom and X-ray options are unchanged.");
 ImGui::PopID();return changed;
}
}
