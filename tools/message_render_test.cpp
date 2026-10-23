#include "../HCMInternal/pch.h"
#include "../HCMInternal/MessagesGUI.h"
#include "../HCMInternal/ModuleHookManager.h"
#include "imgui.h"
#include "../HCMInternal/ImGuiManager.h"
// No application manager: exercise the production font fallback.
ImGuiManager* ImGuiManager::instance=nullptr;
#include <cassert>

struct NoSettingsIO final : ISettingsSerialiser {
    void serialise(std::vector<std::shared_ptr<SerialisableSetting>>&) override {}
    void deserialise(std::vector<std::shared_ptr<SerialisableSetting>>&) override {}
};
static void set(const std::shared_ptr<BinarySetting<bool>>& option,bool value) {
    option->GetValueDisplay()=value;option->UpdateValueWithInput();
}
int main() {
    ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    io.DisplaySize={640,480};io.DeltaTime=1.0f/60;
    io.Fonts->AddFontDefault();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    auto event=std::make_shared<RenderEvent>();
    auto settings=std::make_shared<SettingsStateAndEvents>(std::make_shared<NoSettingsIO>());
    auto messages=std::make_shared<MessagesGUI>(SimpleMath::Vector2{10,10},event);
    messages->setSettings(settings);
    auto add=[&](const char* text){static_cast<IMessagesGUI*>(messages.get())->addMessage(text);};
    auto frame=[&] { ImGui::NewFrame();event->operator()(SimpleMath::Vector2{640,480});ImGui::Render();return ImGui::GetDrawData()->TotalVtxCount; };
    set(settings->showScreenMessages,true);set(settings->hideHUDToggle,false);set(settings->h2NativeHideHUD,false);
    ModuleHookManager::observeH2Gameplay(false);ModuleHookManager::observeGameplay(L"halo3.dll");
    add("Action/status test");assert(frame()>0);
    set(settings->hideHUDToggle,true);assert(frame()==0);
    set(settings->hideHUDToggle,false);assert(frame()>0 && settings->showScreenMessages->GetValue());
    set(settings->showScreenMessages,false);assert(frame()==0);
    set(settings->hideHUDToggle,true);set(settings->hideHUDToggle,false);assert(frame()==0 && !settings->showScreenMessages->GetValue());
    set(settings->showScreenMessages,true);assert(frame()>0);
    // Inactive selections never suppress the active game's messages.
    set(settings->h2NativeHideHUD,true);assert(frame()>0);
    ModuleHookManager::observeGameplay(L"");ModuleHookManager::observeH2Gameplay(true);assert(frame()==0);
    set(settings->hideHUDToggle,true);set(settings->h2NativeHideHUD,false);assert(frame()>0);
    ModuleHookManager::observeH2Gameplay(false);assert(frame()>0); // menu, neither HUD is hidden
    for(auto title:{L"halo3.dll",L"halo3odst.dll",L"haloreach.dll"}) {
        ModuleHookManager::observeGameplay(title);assert(frame()==0);
        set(settings->hideHUDToggle,false);assert(frame()>0);
        set(settings->hideHUDToggle,true);
    }
    // Messages continue to expire while hidden; unhide cannot reveal stale backlog.
    Sleep(12050);assert(frame()==0);set(settings->hideHUDToggle,false);assert(frame()==0);
    add("Fresh status after unhide");assert(frame()>0);
    messages.reset();settings.reset();ImGui::DestroyContext();
    std::cout<<"PASS: real MessagesGUI/ImGui draw vertices, Hide HUD suppression/restoration, independent Show Screen Messages preference, per-title selection isolation, menu visibility, hidden expiry and fresh messages; no texture resources or settings I/O\n";
}
