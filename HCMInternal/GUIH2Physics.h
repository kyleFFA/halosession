#pragma once
#include "IGUIElement.h"
#include "SettingsStateAndEvents.h"
#include "Halo2Physics.h"
#include "PCMTheme.h"
#include "PracticeDashboard.h"

// Poorboys Checkpoint Manager: the widgets HCM's own ones do not cover (the Poorboy's Mods section, the style picker).

// The live line from the mod: armed or not, the body's speed against the 5.45 a step has to pass (bar), the map.
class GUIH2PhysicsStatus : public IGUIElement
{
public:
 GUIH2PhysicsStatus(GameState game,ToolTipCollection tooltip):IGUIElement(game,std::nullopt,tooltip){}
 void render(HotkeyRenderer&)override{
  H3sbStatus status{};status.size=sizeof status;const bool reported=Halo2Physics::readStatus(&status)==1;
  const auto view=practiceui::describe(Halo2Physics::isLoaded(),reported,status,GetTickCount64(),Halo2Physics::lastError());
  const float start=ImGui::GetCursorPosY();practiceui::render(view);currentHeight=ImGui::GetCursorPosY()-start;renderTooltip();
 }
 std::string_view getName()override{return "Halo 2 physics status";}
};
class GUIH2ArmStatistics : public IGUIElement
{
 std::string stats;
public:
 explicit GUIH2ArmStatistics(GameState game):IGUIElement(game,std::nullopt,ToolTipCollection("Keep armed statistics")){}
 void render(HotkeyRenderer&)override{
  const float start=ImGui::GetCursorPosY();
  if(ImGui::Button("Arm statistics")){stats=Halo2Physics::keepArmedStats(mImplGame);ImGui::OpenPopup("Keep armed statistics");}
  if(ImGui::BeginPopup("Keep armed statistics")){ImGui::TextUnformatted(stats.c_str());if(ImGui::Button("Refresh"))stats=Halo2Physics::keepArmedStats(mImplGame);ImGui::SameLine();if(ImGui::Button("Reset counters")){Halo2Physics::resetKeepArmedStats(mImplGame);stats=Halo2Physics::keepArmedStats(mImplGame);}ImGui::EndPopup();}
  currentHeight=ImGui::GetCursorPosY()-start;
 }
 std::string_view getName()override{return "Arm statistics";}
};

// Barriers: the mod's four steps by name (HCM's GUIComboEnum shows enum identifiers), with the cycle hotkey in front.
class GUIH2PhysicsBarriers : public IGUIElement
{
	std::weak_ptr<BinarySetting<int>> mSettingWeak;
	std::vector<std::thread> mUpdateSettingThreads;
	static constexpr const char* kLabels[4] = {
		"The game's own",
		"Soft ceilings off",
		"+ out-of-bounds kill off",
		"+ kill volumes (pits) off",
	};

public:
	GUIH2PhysicsBarriers(GameState implGame, ToolTipCollection tooltip, std::optional<RebindableHotkeyEnum> hotkey, std::shared_ptr<BinarySetting<int>> setting)
		: IGUIElement(implGame, hotkey, tooltip), mSettingWeak(setting)
	{
		this->currentHeight = GUIFrameHeightWithSpacing;
	}

	void render(HotkeyRenderer& hotkeyRenderer) override
	{
		auto setting = mSettingWeak.lock();
		if (!setting) return;
		hotkeyRenderer.renderHotkey(mHotkey);
		ImGui::SameLine();
		const int current = std::clamp(setting->GetValueDisplay(), 0, 3);
		ImGui::SetNextItemWidth(230.0f);
		if (ImGui::BeginCombo("Barriers##h2p", kLabels[current]))
		{
			for (int n = 0; n < 4; n++)
			{
				if (ImGui::Selectable(kLabels[n], n == current) && n != current)
				{
					setting->GetValueDisplay() = n;
					auto& t = mUpdateSettingThreads.emplace_back(std::thread([setting]() { setting->UpdateValueWithInput(); }));
					t.detach();
				}
				if (n == current) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		renderTooltip();
	}

	~GUIH2PhysicsBarriers()
	{
		for (auto& thread : mUpdateSettingThreads)
			if (thread.joinable()) thread.join();
	}

	std::string_view getName() override { return "Barriers##h2p"; }
};

// The overlay's theme (PCMTheme.h), saved with the settings; switching takes effect on the next frame.
class GUIPCMThemePicker : public IGUIElement
{
	std::weak_ptr<BinarySetting<int>> mSettingWeak;
	std::vector<std::thread> mUpdateSettingThreads;

public:
	GUIPCMThemePicker(GameState implGame, ToolTipCollection tooltip, std::shared_ptr<BinarySetting<int>> setting)
		: IGUIElement(implGame, std::nullopt, tooltip), mSettingWeak(setting)
	{
		this->currentHeight = GUIFrameHeightWithSpacing;
		PCMTheme::request(setting->GetValue());       // the one saved last time
	}

	void render(HotkeyRenderer&) override
	{
		auto setting = mSettingWeak.lock();
		if (!setting) return;
		const int current = std::clamp(setting->GetValueDisplay(), 0, (int)PCMTheme::Count - 1);
		ImGui::SetNextItemWidth(300.0f);
		if (ImGui::BeginCombo("Style##pcm", PCMTheme::name(current)))
		{
			for (int n = 0; n < PCMTheme::Count; n++)
			{
				if (ImGui::Selectable(PCMTheme::name(n), n == current) && n != current)
				{
					setting->GetValueDisplay() = n;
					PCMTheme::request(n);
					auto& t = mUpdateSettingThreads.emplace_back(std::thread([setting]() { setting->UpdateValueWithInput(); }));
					t.detach();
				}
				if (n == current) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		renderTooltip();
	}

	~GUIPCMThemePicker()
	{
		for (auto& thread : mUpdateSettingThreads)
			if (thread.joinable()) thread.join();
	}

	std::string_view getName() override { return "Style##pcm"; }
};
