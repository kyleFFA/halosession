#pragma once
#include "IGUIElement.h"
#include "SettingsStateAndEvents.h"
#include "H2CollisionViewer.h"
#include "H2MasterTickrate.h"

namespace h2gui
{
    // H2 actions only queue work; appearance is consumed on this render thread.
    inline void toggle(const char* label, const std::shared_ptr<BinarySetting<bool>>& s)
    {
        if (ImGui::Checkbox(label, &s->GetValueDisplay())) s->UpdateValueWithInput();
    }
    inline void slider(const char* label, const std::shared_ptr<BinarySetting<float>>& s, float lo, float hi)
    {
        ImGui::SetNextItemWidth(180.f);
        if (ImGui::SliderFloat(label, &s->GetValueDisplay(), lo, hi)) s->UpdateValueWithInput();
    }
    inline void color(const char* label, const std::shared_ptr<BinarySetting<SimpleMath::Vector4>>& s)
    {
        ImGui::SetNextItemWidth(180.f);
        if (ImGui::ColorEdit3(label, &s->GetValueDisplay().x)) s->UpdateValueWithInput();
    }
}

class GUIH2NativeGrid : public IGUIElement
{
    std::shared_ptr<SettingsStateAndEvents> s;
public:
    GUIH2NativeGrid(GameState game, std::shared_ptr<SettingsStateAndEvents> settings)
        : IGUIElement(game, std::nullopt, ToolTipCollection("Halo 2 collision grid")), s(std::move(settings))
    { currentHeight = GUIFrameHeightWithSpacing * 2; }
    std::string_view getName() override { return "Halo 2 collision grid"; }
    void render(HotkeyRenderer& hotkeys) override
    {
        const float y = ImGui::GetCursorPosY();
        ImGui::PushID("h2NativeGrid");
        hotkeys.renderHotkey(RebindableHotkeyEnum::h2pWorldLinesHotkey); ImGui::SameLine();
        h2gui::toggle("Collision grid", s->h2NativeGrid);
        if (ImGui::TreeNode("Grid appearance")) {
            h2gui::toggle("See through walls (X-ray)", s->h2pGridXray);
            ImGui::SetNextItemWidth(220.f);
            const char* modes[] = {"Game depth (no bloom)", "Game depth (after bloom)", "Collision depth"};
            if (ImGui::Combo("Occlusion", &s->h2GridOcclusion->GetValueDisplay(), modes, 3)) s->h2GridOcclusion->UpdateValueWithInput();
            h2gui::toggle("Show map edges", s->h2pGridShowStructure);
            h2gui::toggle("Show instance edges", s->h2pGridShowInstances);
            h2gui::color("Map color", s->h2pGridStructureColor);
            h2gui::color("Instances color", s->h2pGridInstanceColor);
            h2gui::slider("Line width", s->h2pGridWidth, .25f, 4.f);
            h2gui::slider("Line opacity", s->h2pGridOpacity, .05f, 1.f);
            h2gui::slider("Hidden-line opacity", s->h2GridHiddenAlpha, 0.f, 1.f);
            h2gui::slider("Draw distance", s->h2GridRadius, 5.f, 500.f);
            ImGui::TreePop();
        }
        h2gui::toggle("Grid only - hide game image", s->h2NativeGridOnly);
        if (ImGui::TreeNode("Grid-only appearance")) {
            h2gui::color("Background color", s->wireViewBackgroundColor);
            h2gui::toggle("See through walls (X-ray)", s->wireViewXray);
            h2gui::toggle("Show map edges", s->wireViewShowStructure);
            h2gui::toggle("Show instance edges", s->wireViewShowInstances);
            h2gui::color("Map color", s->wireViewStructureColor);
            h2gui::color("Instances color", s->wireViewInstanceColor);
            h2gui::slider("Line width", s->wireViewWidth, .25f, 4.f);
            h2gui::slider("Line opacity", s->wireViewOpacity, .05f, 1.f);
            ImGui::TreePop();
        }
        if (s->h2NativeGrid->GetValue() || s->h2NativeGridOnly->GetValue()) {
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 430.f);
            ImGui::TextDisabled("%s", H2CollisionViewer::status().c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::PopID();
        currentHeight = ImGui::GetCursorPosY() - y;
    }
};

class GUIH2NativeTickrate : public IGUIElement
{
    std::shared_ptr<SettingsStateAndEvents> s;
public:
    GUIH2NativeTickrate(GameState game, std::shared_ptr<SettingsStateAndEvents> settings)
        : IGUIElement(game, std::nullopt, ToolTipCollection("Native H2 timer and collision scaling; applied when gameplay resumes")), s(std::move(settings))
    { currentHeight = GUIFrameHeightWithSpacing * 3; }
    std::string_view getName() override { return "Halo 2 native tickrate"; }
    void render(HotkeyRenderer& hotkeys) override
    {
        const float y = ImGui::GetCursorPosY();
        ImGui::PushID("h2NativeTiming");
        h2gui::toggle("Native tickrate control", s->h2NativeTickrateEnabled);
        renderTooltip();
        if (s->h2NativeTickrateEnabled->GetValue()) { hotkeys.renderHotkey(RebindableHotkeyEnum::h2pTick30Hotkey); ImGui::SameLine(); h2gui::toggle("30 tick", s->h2NativeTick30); }
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 430.f);
        ImGui::TextDisabled("30 tick off selects native 60. Turn off Native tickrate control to restore stock timing.");
        ImGui::TextDisabled("%s", H2MasterTickrate::status().c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopID();
        currentHeight = ImGui::GetCursorPosY() - y;
    }
};
