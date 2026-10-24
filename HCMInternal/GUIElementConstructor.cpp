#include "pch.h"
#include "GUIElementConstructor.h"
#include "SettingsStateAndEvents.h"
#include "HotkeysEnum.h"
#include "IGUIElement.h"
#include "GUIElementEnum.h"

#include "GUISimpleButton.h"
#include "GUISimpleToggle.h"

#include "GUIInvulnerability.h"
#include "GUIHeading.h"
#include "GUISubHeading.h"
#include "GUIHotkeyOnly.h"
#include "GUIAdvanceTicks.h"


#include "GUIFloat.h"




#include "GUIColourPicker.h"



#include "GUIH2Physics.h"
#include "GUIWorkshopGroups.h"
#include "GUIH2Native.h"




















class GUIElementConstructor::GUIElementConstructorImpl {
private:
	std::set<std::pair<GUIElementEnum, GameState::Value>> cacheFailedServices;

	std::optional<std::shared_ptr<IGUIElement>> createGUIElementAndStoreResult(GUIElementEnum guielementenum, GameState game, std::shared_ptr<IGUIRequiredServices> guireq, std::shared_ptr<OptionalCheatInfo> fail, std::shared_ptr<GUIServiceInfo> info, std::shared_ptr<SettingsStateAndEvents> settings, std::shared_ptr<RuntimeExceptionHandler> exp)
	{
		auto res = createGUIElement(guielementenum, game, guireq, fail, info, settings, exp);
		if (res.has_value()) mStore->mapOfSuccessfullyConstructedGUIElements.at(game).insert(guielementenum);
		return res;
	}

	std::optional<std::shared_ptr<IGUIElement>> createGUIElement(GUIElementEnum guielementenum, GameState game, std::shared_ptr<IGUIRequiredServices> guireq, std::shared_ptr<OptionalCheatInfo> fail, std::shared_ptr<GUIServiceInfo> info, std::shared_ptr<SettingsStateAndEvents> settings, std::shared_ptr<RuntimeExceptionHandler> exp)
	{
		if (!guireq->getSupportedGamesPerGUIElement().contains(guielementenum)) throw HCMInitException("getSupportedGamesPerGUIElement missing element, how did that even happen?");
		if (!guireq->getSupportedGamesPerGUIElement().at(guielementenum).contains(game))
		{
			PLOG_DEBUG << "GUIElementEnum::" << magic_enum::enum_name(guielementenum) << " does not support Game::" << game.toString() << ", skipping construction";
			return std::nullopt; // don't construct for unsupported game
		}

		if (cacheFailedServices.contains(std::make_pair(guielementenum, game))) // ignore elements that have already failed construction
		{
			PLOG_DEBUG << "GUIElementEnum::" << magic_enum::enum_name(guielementenum) << " for Game::" << game.toString() << " already failed construction, skipping construction";
			return std::nullopt;
		}


		// stupid override to make core save/dump stuff only construct for steam since I can't be bothered reworking the macro system to do this properly
		if ((magic_enum::enum_name<GUIElementEnum>(guielementenum).contains("core") || magic_enum::enum_name<GUIElementEnum>(guielementenum).contains("Core")) // is the gui elemenet a core save thing? this is the most icky part of this, doing a stringcheck of the enum name.. what if I want to add another cheat that uses the name "core" to mean something else? eh prolly won't come up
			&& game.operator GameState::Value() == GameState::Value::Halo1 // and is halo 1
			&& mProcType == MCCProcessType::WinStore // and current process is winstore
			)
		{
			PLOG_DEBUG << "GUIElementEnum::" << magic_enum::enum_name(guielementenum) << " for game " << game.toString() << " does not support WinStore version of MCC (steam only), skipping construction";
			return std::nullopt; // don't construct 
		}


		PLOG_DEBUG << "attempting to construct guielement: " << magic_enum::enum_name(guielementenum) << " for game: " << game.toString();
		try 
		{
			// check for failed required services
			if (guireq->getRequiredServicesPerGUIElement().contains(guielementenum))
			{
				std::vector<HCMInitException> failures;
				for (auto reqSer : guireq->getRequiredServicesPerGUIElement().at(guielementenum))
				{
					auto info = fail->getOptionalCheatInfo({ game, reqSer });
					if (info.init == false && info.ex.has_value()) failures.push_back(info.ex.value());
				}

				if (!failures.empty())
				{
					info->addFailure(guielementenum, game, failures);

					// TODO: get rid of this
					PLOG_ERROR << "ERROR: ";
					for (auto& fail : failures)
					{
						PLOG_ERROR << fail.what();
					}

					return std::nullopt;
				}
			}


			typedef  std::vector<std::optional<std::shared_ptr<IGUIElement>>> headerChildElements;
			typedef  std::vector<std::vector<std::optional<std::shared_ptr<IGUIElement>>>> vectorOfHeaderChildElements;
#define createNestedElement(elementEnum) createGUIElementAndStoreResult(elementEnum, game, guireq, fail, info, settings, exp) // optional nested element, recursively calls this function




			// ALL GUI ELEMENTS MUST HAVE A CASE HERE, TOP LEVEL OR NOT
			std::optional<std::shared_ptr<IGUIElement>> x;
			switch (guielementenum)
			{
			// ---- Poorboys Checkpoint Manager: every one of Poorboy's mods, together at the top, apart from Halo Checkpoint
			//      Manager's own sections (Halo2Physics.cpp, GUIH2Physics.h, BottomlessClip.cpp) ----
			
            case GUIElementEnum::h2NativeHeadingGUI:
                return std::make_shared<GUIWorkshopPractice>(game, std::nullopt, headerChildElements{
                    createNestedElement(GUIElementEnum::invulnGUI),
                    createNestedElement(GUIElementEnum::h2NativeClipGUI),
                    createNestedElement(GUIElementEnum::hideHUDToggle),
                    createNestedElement(GUIElementEnum::aiFreezeGUI),
                    createNestedElement(GUIElementEnum::h2NativeTickrateGUI),
                    createNestedElement(GUIElementEnum::h2NativeGridGUI),
                    createNestedElement(GUIElementEnum::h2NativeSaveSpotGUI),
                    createNestedElement(GUIElementEnum::h2NativeBackToSpotGUI),
                    createNestedElement(GUIElementEnum::h2NativeAutoCrouchGUI)});
            case GUIElementEnum::h2NativeGridGUI:
                return std::make_shared<GUIH2NativeGrid>(game, settings);
            case GUIElementEnum::h2NativeSaveSpotGUI:
                return std::make_shared<GUISimpleButton<true>>(game, ToolTipCollection("Original H2: bookmark your position, live view and velocity for this map/session. No checkpoint or save state is captured. Crouch and armed/contact state are not stored. Requires one owned living player on foot in a local/server session. Bind with the .. button."), RebindableHotkeyEnum::spotSaveHotkey, "Save spot", settings->spotSaveEvent);
            case GUIElementEnum::h2NativeBackToSpotGUI:
                return std::make_shared<GUISimpleButton<true>>(game, ToolTipCollection("Teleport the CURRENT local H2 player to the bookmark and restore view/velocity. World objects and other players stay unchanged. A fresh owned biped can use the bookmark after respawn in the same map/session. Clients are blocked; no checkpoint revert or physics bridge."), RebindableHotkeyEnum::spotBackHotkey, "Back to spot", settings->spotBackEvent);
            case GUIElementEnum::h2NativeAutoCrouchGUI:
                return std::make_shared<GUISimpleButton<true>>(game, ToolTipCollection("Original H2 native movement: one 0.2-unit move in your look direction, including pitch, per button press. Aim and velocity stay unchanged. Requires an owned living biped on foot with local/server authority. Bind with the .. button."), RebindableHotkeyEnum::autoCrouchHotkey, "Auto-crouch", settings->autoCrouchEvent);
            case GUIElementEnum::h2NativeTickrateGUI:
                return std::make_shared<GUIH2NativeTickrate>(game, settings);
            case GUIElementEnum::h2NativeClipGUI:
                return std::make_shared<GUISimpleToggle<true>>(game, ToolTipCollection("Halo 2's native bottomless clip flag"), RebindableHotkeyEnum::bottomlessClip, "Bottomless clip", settings->h2NativeClip);
            case GUIElementEnum::aiFreezeGUI:
                return std::make_shared<GUISimpleToggle<true>>(game, ToolTipCollection("Freeze AI using HCM's Halo 2 implementation"), RebindableHotkeyEnum::aiFreeze, "AI Freeze", settings->h2NativeAI);

			case GUIElementEnum::halo2PhysicsHeadingGUI:
				return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIWorkshopPractice>
                    (game, createNestedElement(GUIElementEnum::h2pStatusGUI), headerChildElements
						{
						std::make_shared<GUIH2ArmStatistics>(game),
						createNestedElement(GUIElementEnum::h2pPhysicsToggleGUI),
						createNestedElement(GUIElementEnum::h2pArmGUI),
                        createNestedElement(GUIElementEnum::h2pKeepArmedGUI),
						createNestedElement(GUIElementEnum::autoCrouchGUI),
						createNestedElement(GUIElementEnum::h2pWorldLinesToggleGUI),
						createNestedElement(GUIElementEnum::h2pGridWidthGUI),
						createNestedElement(GUIElementEnum::h2pGridXrayGUI),
						createNestedElement(GUIElementEnum::h2pGridShowStructureGUI),
						createNestedElement(GUIElementEnum::h2pGridShowInstancesGUI),
						createNestedElement(GUIElementEnum::h2pGridStructureColorGUI),
						createNestedElement(GUIElementEnum::h2pGridInstanceColorGUI),
						createNestedElement(GUIElementEnum::h2pGridOpacityGUI),
						createNestedElement(GUIElementEnum::h2pGridNoBloomGUI),
                        createNestedElement(GUIElementEnum::wireViewHeadingGUI),
						createNestedElement(GUIElementEnum::h2pFlyToggleGUI),
						createNestedElement(GUIElementEnum::h2pFlySpeedGUI),
						createNestedElement(GUIElementEnum::h2pFlyHoldGUI),
						createNestedElement(GUIElementEnum::h2pBarriersGUI),
						createNestedElement(GUIElementEnum::h2pTick30ToggleGUI),
						createNestedElement(GUIElementEnum::invulnGUI),
						createNestedElement(GUIElementEnum::h2pCombosToggleGUI),
						createNestedElement(GUIElementEnum::h2pNoFallDeathsToggleGUI),
						createNestedElement(GUIElementEnum::h2pDisableAIGUI),
						createNestedElement(GUIElementEnum::h2pForgeBounceGUI),
						createNestedElement(GUIElementEnum::spotSaveGUI),
						createNestedElement(GUIElementEnum::spotBackGUI),
						createNestedElement(GUIElementEnum::h2pBottomlessClipGUI),
						createNestedElement(GUIElementEnum::hideHUDToggle),
						}));

				case GUIElementEnum::wireViewXrayGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Show edges through collision surfaces. Off lets collision surfaces hide edges; the native scene remains hidden."), std::nullopt, "See through walls (X-ray)##wireView", settings->wireViewXray));
				case GUIElementEnum::wireViewShowStructureGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Show structure edges. Hidden edges still occlude other geometry."), std::nullopt, "Show map edges##wireView", settings->wireViewShowStructure));
				case GUIElementEnum::wireViewShowInstancesGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Show instanced geometry edges, including rocks and props."), std::nullopt, "Show instance edges##wireView", settings->wireViewShowInstances));
				case GUIElementEnum::wireViewBackgroundColorGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIColourPicker<true>>
						(game, ToolTipCollection("Opaque background color; hides the whole native game image"), "Background color##wireView", settings->wireViewBackgroundColor));
				case GUIElementEnum::wireViewStructureColorGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIColourPicker<true>>
						(game, ToolTipCollection("Color of the map's structure edges"), "Map color##wireView", settings->wireViewStructureColor));
				case GUIElementEnum::wireViewInstanceColorGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIColourPicker<true>>
						(game, ToolTipCollection("Color of instanced geometry edges, including rocks and props"), "Instances color##wireView", settings->wireViewInstanceColor));

				case GUIElementEnum::wireViewWidthGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIFloat<SliderParam<float>(0.25f, 4.f)>>
						(game, ToolTipCollection("The map grid's line width in pixels (at 1080p; bigger screens scale it up)"), "Grid line width##wireView", settings->wireViewWidth));

				case GUIElementEnum::wireViewOpacityGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIFloat<SliderParam<float>(0.05f, 1.f)>>
						(game, ToolTipCollection("How solid the map grid's lines are: 1 = solid, lower = see-through"), "Grid opacity##wireView", settings->wireViewOpacity));

                case GUIElementEnum::wireViewToggleGUI:
                    return std::make_shared<GUISimpleToggle<false>>(game, ToolTipCollection("Solid background with collision grid only. Masks the native HUD, weapon, sky and objects. Does not reduce scene rendering. Starts off each session; normal grid settings are preserved."), std::nullopt, "Enable grid-only view", settings->wireViewToggle);
                case GUIElementEnum::wireViewHeadingGUI:
                    return std::make_shared<GUIHeading>(game, ToolTipCollection("Independent grid view on a solid background. Takes visual priority while enabled; disabling restores the normal view."), "Grid-only view", headerChildElements{
                        createNestedElement(GUIElementEnum::wireViewToggleGUI),
                        createNestedElement(GUIElementEnum::wireViewBackgroundColorGUI),
                        createNestedElement(GUIElementEnum::wireViewXrayGUI),
                        createNestedElement(GUIElementEnum::wireViewShowStructureGUI),
                        createNestedElement(GUIElementEnum::wireViewShowInstancesGUI),
                        createNestedElement(GUIElementEnum::wireViewStructureColorGUI),
                        createNestedElement(GUIElementEnum::wireViewInstanceColorGUI),
                        createNestedElement(GUIElementEnum::wireViewWidthGUI),
                        createNestedElement(GUIElementEnum::wireViewOpacityGUI)});
				case GUIElementEnum::autoCrouchGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleButton<true>>
						(game, ToolTipCollection("One-shot move 0.4 units in your look direction, including pitch, matching Halo3Spot. Does not change aim or velocity."), RebindableHotkeyEnum::autoCrouchHotkey, "Auto-crouch", settings->autoCrouchEvent));

				case GUIElementEnum::h2pStatusGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIH2PhysicsStatus>
						(game, ToolTipCollection("Bounce State displays the existing armed flag only. Speed is the Halo 2 body's velocity magnitude in world units per second; pass threshold is a separate sweep diagnostic.")));

				case GUIElementEnum::h2pPhysicsToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Halo 2's player physics, movement and jump. Off: the mod unloads and the game is untouched"), RebindableHotkeyEnum::h2pPhysicsHotkey, "Halo 2 physics", settings->h2pPhysicsToggle));

                case GUIElementEnum::h2pKeepArmedGUI:
                    return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
                        (game, ToolTipCollection("Keeps requesting the local Halo 2 body's armed state while physics is on. Suspends outside gameplay, in detected menus and while this overlay is open. Off stops requests without forcing an unarm. Starts off each session; bind with .."), RebindableHotkeyEnum::h2pKeepArmedHotkey, "Keep armed", settings->h2pKeepArmedToggle));

				case GUIElementEnum::h2pArmGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleButton<true>>
						(game, ToolTipCollection("Puts the Halo 2 body in the armed state now (what standing up inside geometry does), so you can go straight to a bounce spot. Like Halo 2, it is lost if you stand still long enough to fall asleep. Bind it to a key or a controller button with the .. button"), RebindableHotkeyEnum::h2pArmHotkey, "Arm now", settings->h2pArmEvent));

				case GUIElementEnum::h2pWorldLinesToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Draws every collision edge of the whole map into the game view, hidden behind walls: The map structure and instanced geometry (rocks, pillars, props) use the colors selected below. Seams between polygons are where bounces come from. Drawn by this overlay inside the game's own frame (no extra window). Works with Halo 2 physics on or off (the mod is loaded to read the map). Saved; bind it with the .. button"), RebindableHotkeyEnum::h2pWorldLinesHotkey, "Map grid", settings->h2pWorldLinesToggle));

				case GUIElementEnum::h2pGridWidthGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIFloat<SliderParam<float>(0.25f, 4.f)>>
						(game, ToolTipCollection("The map grid's line width in pixels (at 1080p; bigger screens scale it up)"), "Grid line width##h2p", settings->h2pGridWidth));

				case GUIElementEnum::h2pGridStructureColorGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIColourPicker<true>>
						(game, ToolTipCollection("Color of the map's structure edges"), "Map color##h2p", settings->h2pGridStructureColor));
				case GUIElementEnum::h2pGridXrayGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("See grid edges through walls, objects and your weapon. Uses the no-bloom view. Off restores normal occlusion; Grid opacity controls how strong the lines are."), std::nullopt, "See through walls (X-ray)##h2p", settings->h2pGridXray));
				case GUIElementEnum::h2pGridShowStructureGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Show structure edges. Hidden edges still occlude other geometry."), std::nullopt, "Show map edges##h2p", settings->h2pGridShowStructure));
				case GUIElementEnum::h2pGridShowInstancesGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Show instanced geometry edges, including rocks and props."), std::nullopt, "Show instance edges##h2p", settings->h2pGridShowInstances));
				case GUIElementEnum::h2pGridInstanceColorGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIColourPicker<true>>
						(game, ToolTipCollection("Color of instanced geometry edges, including rocks and props"), "Instances color##h2p", settings->h2pGridInstanceColor));

				case GUIElementEnum::h2pGridNoBloomGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Draws the map grid over the finished picture, as in ODST and Reach, instead of inside Halo 3's own 3D render: no bloom or glow on the lines; your gun and arms still hide them. Saved"), std::nullopt, "Grid without bloom##h2p", settings->h2pGridNoBloomToggle));

				case GUIElementEnum::h2pGridOpacityGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIFloat<SliderParam<float>(0.05f, 1.f)>>
						(game, ToolTipCollection("How solid the map grid's lines are: 1 = solid, lower = see-through"), "Grid opacity##h2p", settings->h2pGridOpacity));

				case GUIElementEnum::h2pFlyToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Fly your player where you look: stick forward/back/sideways, jump up, crouch down, through walls. Works online in custom games (others see you fly) and with Halo 2 physics on or off. Always starts off; bind it with the .. button"), RebindableHotkeyEnum::h2pFlyHotkey, "Fly", settings->h2pFlyToggle));

				case GUIElementEnum::h2pFlyHoldGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("The Fly hotkey flies only while you hold it: let go and you drop (instead of a press switching fly on and off). Bind the Fly hotkey with its .. button"), std::nullopt, "Hold the Fly key to fly##h2p", settings->h2pFlyHoldToggle));

				case GUIElementEnum::h2pFlySpeedGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIFloat<SliderParam<float>(1.f, 40.f)>>
						(game, ToolTipCollection("Fly speed in world units a second (running is about 2.25)"), "Fly speed##h2p", settings->h2pFlySpeed));

				case GUIElementEnum::h2pBarriersGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIH2PhysicsBarriers>
						(game, ToolTipCollection("Invisible barriers while Halo 2 physics is on. The hotkey steps through the four"), RebindableHotkeyEnum::h2pBarriersCycleHotkey, settings->h2pBarriers));

				case GUIElementEnum::h2pTick30ToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection(game.operator GameState::Value() == GameState::Value::HaloReach
							? "Reach itself always runs at 60 ticks a second: this switches only the Halo 2 physics, any time (mid-game too). On: Halo 2 at 30 ticks (Xbox), one Halo 2 step every second Reach tick: superbounces, big landings on slopes stop on ledges. Off: Halo 2 at 60 ticks (MCC Halo 2's old 60 tick): you slide down slopes, and bounces (60 tick bouncing) need much bigger drops but go much higher"
							: "Switches the game and the Halo 2 physics between 30 and 60 ticks a second, any time (mid-game too). On: Halo 2 at 30 ticks (Xbox): superbounces, big landings on slopes stop on ledges. Off: Halo 2 at 60 ticks (MCC Halo 2's old 60 tick): you slide down slopes, and bounces (60 tick bouncing) need much bigger drops but go much higher"),
							RebindableHotkeyEnum::h2pTick30Hotkey, "30 tick", settings->h2pTick30Toggle));

				case GUIElementEnum::h2pCombosToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Halo 2's button combos: melee cancels (BXR, BXB, double shot) and melee lock; X during a weapon switch swaps at once (YX); the plasma pistol double grenade (charged shot, then about a second later grenade, grenade).\nWith Halo 2 physics on, Halo 2's lunges too: a melee or sword lunge flies straight at the target with no gravity and keeps going if you cancel it (butterflying: spam melee + reload at an enemy standing on your head while he jumps); sword flying: rifle in hand locked on someone, Y, X, then RT at once: the sword lunges at what the rifle had locked"), RebindableHotkeyEnum::h2pCombosHotkey, "Halo 2 button combos", settings->h2pCombosToggle));

				case GUIElementEnum::h2pBottomlessClipGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Your magazine never empties, plasma batteries don't drain and your grenades never run out (you need at least one of a kind). Reserve ammo still counts and plasma weapons still overheat. Reload first: a part-empty magazine stays as it is"), RebindableHotkeyEnum::bottomlessClip, "Bottomless Clip", settings->bottomlessClipToggle));

				case GUIElementEnum::h2pDisableAIGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Switches the AI off, as the game's own ai_enable false: enemies and allies stand still and don't shoot. For campaign practice and videos: Theatre films replay it exactly (Cheats' Freeze AI does not). Works with Halo 2 physics on or off; starts off every time"), RebindableHotkeyEnum::h2pDisableAIHotkey, "Disable AI", settings->h2pDisableAIToggle));

				case GUIElementEnum::h2pForgeBounceGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("With Halo 2 physics on: Forge objects count as part of the map for the Halo 2 body, so the seams between Forge pieces (and between a piece and the map) bounce like map seams. For building your own bounce maps. Off = Halo 2's own rule (every object is its own body; in Reach Forge pieces are then not solid for the Halo 2 body)"), RebindableHotkeyEnum::h2pForgeBounceHotkey, "Forge bounces", settings->h2pForgeBounceToggle));

				case GUIElementEnum::h2pNoFallDeathsToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Falls never kill or hurt you (campaign and Firefight). Other damage still counts"), RebindableHotkeyEnum::h2pNoFallDeathsHotkey, "No falling deaths", settings->h2pNoFallDeathsToggle));

				case GUIElementEnum::controlHeadingGUI:
				return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIWorkshopAppearance>
					(game, headerChildElements
						{
						createNestedElement(GUIElementEnum::pcmThemeGUI),
						createNestedElement(GUIElementEnum::toggleGUIHotkeyGUI),
						createNestedElement(GUIElementEnum::showScreenMessagesGUI),
                        createNestedElement(GUIElementEnum::messagesFontSize),
						createNestedElement(GUIElementEnum::messagesFontColor),
						createNestedElement(GUIElementEnum::GUISettingsSubheading),
						createNestedElement(GUIElementEnum::togglePauseGUI),
						createNestedElement(GUIElementEnum::togglePauseSettingsSubheading),
						createNestedElement(GUIElementEnum::showGUIFailuresGUI),
						createNestedElement(GUIElementEnum::OBSBypassToggleGUI),
						}));

				case GUIElementEnum::pcmThemeGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIPCMThemePicker>
						(game, ToolTipCollection("Poorboys Checkpoint Manager's look: the three styles from the mockups, or Halo Checkpoint Manager's own"), settings->pcmTheme));

				case GUIElementEnum::toggleGUIHotkeyGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIHotkeyOnly<63>>
						(game, ToolTipCollection("Show or hide Session with one press. Default: backtick / tilde (left of 1 on a US keyboard). Use the binding button to change it; your saved binding is preserved."), RebindableHotkeyEnum::toggleGUI, "Menu toggle"));

                case GUIElementEnum::showScreenMessagesGUI:
                    return std::make_shared<GUISimpleToggle<false>>(game,
                        ToolTipCollection("Show temporary action/status messages on screen. Hide HUD temporarily hides these during gameplay; unhiding restores this preference. Saved between sessions."),
                        std::nullopt, "Show screen messages", settings->showScreenMessages);

				case GUIElementEnum::messagesFontSize:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIFloat<SliderParam(6.f, 120.f)>>
						(game, ToolTipCollection("Font size of messages that show up under this window"), "Message Font Size", settings->messagesFontSize));


				case GUIElementEnum::messagesFontColor:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIColourPicker<true>>
						(game, ToolTipCollection("Color of messages that show up under this window"), "Messages Font Color", settings->messagesFontColor));


				case GUIElementEnum::GUISettingsSubheading:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISubHeading<false>>
						(game, ToolTipCollection("Options related to this main GUI"), "GUI settings", headerChildElements
							{
							createNestedElement(GUIElementEnum::GUIShowingFreesCursor),
							createNestedElement(GUIElementEnum::GUIShowingBlocksInput),
							createNestedElement(GUIElementEnum::GUIShowingPausesGame),
							}));

					case GUIElementEnum::GUIShowingFreesCursor:
						return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
							(game, ToolTipCollection("Automatically free the curson when this GUI is open"), std::nullopt, "Free cursor when GUI open", settings->GUIShowingFreesCursor));

					case GUIElementEnum::GUIShowingBlocksInput:
						return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
							(game, ToolTipCollection("Automatically block game input when this GUI is open"), std::nullopt, "Block input when GUI open", settings->GUIShowingBlocksInput));

					case GUIElementEnum::GUIShowingPausesGame:
						return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
							(game, ToolTipCollection("Pause automatically only in verified solo original-H2 campaign. Multiplayer, co-op, other titles and unknown modes never acquire a menu pause. The quick wheel never pauses."), std::nullopt, "Pause menu in verified solo H2 campaign", settings->GUIShowingPausesGame));

				case GUIElementEnum::togglePauseGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
						(game, ToolTipCollection("Pauses the game simulation"), RebindableHotkeyEnum::togglePause, "Pause game", settings->togglePause));

				case GUIElementEnum::togglePauseSettingsSubheading:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISubHeading<false>>
						(game, ToolTipCollection("Settings for pausing the game"), "Pause settings", headerChildElements
							{
							createNestedElement(GUIElementEnum::advanceTicksGUI),
							createNestedElement(GUIElementEnum::pauseAlsoBlocksInputGUI),
							createNestedElement(GUIElementEnum::pauseAlsoFreesCursorGUI),
							}));


				case GUIElementEnum::advanceTicksGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIAdvanceTicks<true>>
						(game, ToolTipCollection("Advances the game simulation by x ticks while the game is paused"), RebindableHotkeyEnum::advanceTicks, settings->advanceTicksEvent, settings->advanceTicksCount));

					case GUIElementEnum::pauseAlsoBlocksInputGUI:
						return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
							(game, ToolTipCollection("Makes this pause feature also block game input"), std::nullopt, "Pause also blocks game input", settings->pauseAlsoBlocksInput));

					case GUIElementEnum::pauseAlsoFreesCursorGUI:
						return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
							(game, ToolTipCollection("Makes this pause feature also free MCC's cursor"), std::nullopt, "Pause also frees cursor", settings->pauseAlsoFreesCursor));

				case GUIElementEnum::showGUIFailuresGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleButton<false>>
						(game, ToolTipCollection("Shows a modal list of optional cheat services that failed to initialise, sorted by game"), std::nullopt, "Show optional cheat service failures", settings->showGUIFailures));

					


				case GUIElementEnum::OBSBypassToggleGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<false>>
						(game, ToolTipCollection("Causes HCMs GUI to not be captured by recording software like OBS"), std::nullopt, "Bypass OBS Capture", settings->OBSBypassToggle));


				case GUIElementEnum::spotSaveGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleButton<true>>
						(game, ToolTipCollection("Saves where you are: the place, your speed, crouch, which way you look and Halo 2's armed state. Back to spot puts you there again. Unlike a revert it moves only your own player and rolls nothing else back, so it is made for multiplayer jumping (online too). Works with Halo 2 physics on or off. Bind it with the .. button"), RebindableHotkeyEnum::spotSaveHotkey, "Save spot", settings->spotSaveEvent));

				case GUIElementEnum::spotBackGUI:
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleButton<true>>
						(game, ToolTipCollection("Puts you back at the saved spot, looking the same way, at the same speed, armed again if you were. Others online see you there. Objects stay where they are: for crates and the like use Force Checkpoint / Force Revert. Bind it with the .. button"), RebindableHotkeyEnum::spotBackHotkey, "Back to spot", settings->spotBackEvent));

				case GUIElementEnum::invulnGUI:
                    if (game == GameState(GameState::Value::Halo2)) return std::make_shared<GUISimpleToggle<true>>(game, ToolTipCollection("HCM Halo 2 player invulnerability, including fall-timer protection"), RebindableHotkeyEnum::invuln, "Invulnerability", settings->h2NativeInvulnerability);
					return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIInvulnerability>
						(game, ToolTipCollection("Makes the player invulnerable to all damage and kill triggers"), RebindableHotkeyEnum::invuln, "Invulnerability", settings->invulnerabilityToggle
							)); 


				case GUIElementEnum::hideHUDToggle:
				return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUISimpleToggle<true>>
					(game, ToolTipCollection("Hides the HUD and temporary action/status messages during gameplay, while keeping your first person model. Unhiding restores your Show screen messages preference."), RebindableHotkeyEnum::hideHUDToggle, "Hide HUD", game == GameState(GameState::Value::Halo2) ? settings->h2NativeHideHUD : settings->hideHUDToggle));


				case GUIElementEnum::theaterHeadingGUI:
				return std::optional<std::shared_ptr<IGUIElement>>(std::make_shared<GUIHeading>
					(game, ToolTipCollection("this is where theater stuff would go IF I HAD ANY"), "Camera", headerChildElements{ std::nullopt }));


			

			default:
				throw HCMInitException(std::format("You forgot to add a creation case label for GUIElement: {} in {}::{}", magic_enum::enum_name(guielementenum), nameof(GUIElementConstructor), nameof(createGUIElement)));
			}


		}
		catch (HCMInitException ex)
		{
			info->addFailure(guielementenum, game, ex);
			cacheFailedServices.insert(std::make_pair(guielementenum, game));
			return std::nullopt;
		}
	}


	std::shared_ptr<GUIElementStore> mStore;

	MCCProcessType mProcType;

public:
	GUIElementConstructorImpl(std::shared_ptr<IGUIRequiredServices> guireq, std::shared_ptr<OptionalCheatInfo> fail, std::shared_ptr<GUIElementStore> store, std::shared_ptr<GUIServiceInfo> info, std::shared_ptr<SettingsStateAndEvents> settings, MCCProcessType procType, std::shared_ptr<RuntimeExceptionHandler> exp)
		: mStore(store), mProcType(procType)
	{
		// Create all top level GUI elements. Each one will recursively create it's nested elements, if it has any.
		for (auto& [element, game] : guireq->getToplevelGUIElements())
		{


			auto x = createGUIElementAndStoreResult(element, game, guireq, fail, info, settings, exp);
			if (x.has_value())
			{
				store->getTopLevelGUIElementsMutable().at(game).push_back(x.value());
			}
		}
	}
};






GUIElementConstructor::GUIElementConstructor(std::shared_ptr<IGUIRequiredServices> guireq, std::shared_ptr<OptionalCheatInfo> fail, std::shared_ptr<GUIElementStore> store, std::shared_ptr<GUIServiceInfo> info, std::shared_ptr<SettingsStateAndEvents> settings, MCCProcessType procType, std::shared_ptr<RuntimeExceptionHandler> exp) : pimpl(std::make_unique<GUIElementConstructorImpl>(guireq, fail, store, info, settings, procType, exp)) {}
GUIElementConstructor::~GUIElementConstructor() = default;
