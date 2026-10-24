#include "pch.h"
#include "SettingsStateAndEvents.h"
#include "HotkeyDefinitions.h"
#include "HotkeyEventsLambdas.h"
#include <cassert>
#include <iostream>
struct Memory: ISettingsSerialiser {
 pugi::xml_document doc;
 void serialise(std::vector<std::shared_ptr<SerialisableSetting>>& all)override{doc.reset();auto node=doc.append_child("Setting");for(auto& option:all)if(option->getOptionName().starts_with("speedOverlay"))option->serialise(node);}
 void deserialise(std::vector<std::shared_ptr<SerialisableSetting>>& all)override{auto node=doc.child("Setting");for(auto& option:all)if(option->getOptionName().starts_with("speedOverlay"))if(auto input=node.child(option->getOptionName().c_str()))option->deserialise(input);}
};
int main(){
 ImGui::CreateContext();auto memory=std::make_shared<Memory>();auto settings=std::make_shared<SettingsStateAndEvents>(memory);assert(!settings->speedOverlayVisible->GetValue());assert(std::fabs(settings->speedOverlayHeight->GetValue()-.045f)<.001f);
 {HotkeyDefinitions definitions(settings);HotkeyEventsLambdas handlers(settings);auto hotkey=definitions.getAllEventOnPressHotkeys().at(RebindableHotkeyEnum::speedOverlayHotkey);assert(hotkey->getBindings().empty());hotkey->setBindings({{ImGuiKey_F6},{ImGuiKey_GamepadFaceUp}});assert(hotkey->getBindings().size()==2);const bool menu=settings->GUIWindowOpen->GetValue();hotkey->invokeEvent();assert(settings->speedOverlayVisible->GetValue()&&settings->GUIWindowOpen->GetValue()==menu);hotkey->invokeEvent();assert(!settings->speedOverlayVisible->GetValue());}
 settings->speedOverlayVisible->GetValueDisplay()=true;settings->speedOverlayVisible->UpdateValueWithInput();settings->speedOverlayHeight->GetValueDisplay()=.5f;settings->speedOverlayHeight->UpdateValueWithInput();settings->speedOverlayColor->GetValueDisplay()={.2f,.4f,.7f,.35f};settings->speedOverlayColor->UpdateValueWithInput();settings->speedOverlayPosition->GetValueDisplay()={.15f,.8f};settings->speedOverlayPosition->UpdateValueWithInput();settings.reset();
 auto restored=std::make_shared<SettingsStateAndEvents>(memory);assert(restored->speedOverlayVisible->GetValue());assert(restored->speedOverlayHeight->GetValue()==.5f);assert(restored->speedOverlayColor->GetValue().w==.35f);assert(restored->speedOverlayPosition->GetValue().x==.15f);restored->speedOverlayHeight->GetValueDisplay()=2;restored->speedOverlayHeight->UpdateValueWithInput();assert(restored->speedOverlayHeight->GetValue()==.5f);restored->speedOverlayPosition->GetValueDisplay()={NAN,1};restored->speedOverlayPosition->UpdateValueWithInput();assert(restored->speedOverlayPosition->GetValue().x==.15f);restored.reset();ImGui::DestroyContext();std::cout<<"PASS: actual settings round-trip, bound toggle/event registration, unbound default, menu independence, color/opacity/position/half-size persistence and invalid value rejection.\n";
}
