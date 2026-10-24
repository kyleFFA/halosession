#pragma once
#include "IGUIElement.h"
// Route existing element instances; settings, hotkeys, events and service requirements are unchanged.
class GUIWorkshopPractice : public IGUIElement {
 std::shared_ptr<IGUIElement> dashboard;
 std::vector<std::shared_ptr<IGUIElement>> controls;
public:
 GUIWorkshopPractice(GameState game,std::optional<std::shared_ptr<IGUIElement>> status,std::vector<std::optional<std::shared_ptr<IGUIElement>>> children):IGUIElement(game,std::nullopt,ToolTipCollection("Practice dashboard")){
  if(status)dashboard=*status;for(auto& child:children)if(child)controls.push_back(*child);
 }
 void render(HotkeyRenderer& keys)override{float start=ImGui::GetCursorPosY();if(dashboard)dashboard->render(keys);else ImGui::TextWrapped("Live physics telemetry is unavailable for this title.");currentHeight=ImGui::GetCursorPosY()-start;}
 void renderControlPanel(HotkeyRenderer& keys)override{if(ImGui::CollapsingHeader("Controls",ImGuiTreeNodeFlags_DefaultOpen)){for(auto& child:controls){ImGui::PushID(child->getName().data());child->render(keys);ImGui::PopID();}}}
 std::string_view getName()override{return "Practice dashboard";}
 size_t controlCount()const{return controls.size();}
};
class GUIWorkshopAppearance : public IGUIElement {
 std::vector<std::shared_ptr<IGUIElement>> controls;
public:
 GUIWorkshopAppearance(GameState game,std::vector<std::optional<std::shared_ptr<IGUIElement>>> children):IGUIElement(game,std::nullopt,ToolTipCollection("Menu and UI settings")){for(auto& child:children)if(child)controls.push_back(*child);}
 void render(HotkeyRenderer&)override{}
 void renderAppearancePanel(HotkeyRenderer& keys)override{ImGui::Separator();ImGui::TextUnformatted("Menu and UI settings");for(auto& child:controls){ImGui::PushID(child->getName().data());child->render(keys);ImGui::PopID();}}
 std::string_view getName()override{return "Control";}
 size_t controlCount()const{return controls.size();}
};
