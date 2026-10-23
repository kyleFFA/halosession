#include "pch.h"
#include "GUIRequiredServices.h"


// ALL GUI ELEMENTS, nested or not, are allowed to go in here - but if they have no required services, you don't have to
const std::map <GUIElementEnum, std::vector<OptionalCheatEnum>> GUIRequiredServices::requiredServicesPerGUIElement =
{
{GUIElementEnum::h2NativeSaveSpotGUI, {OptionalCheatEnum::H2NativeMovement}},
{GUIElementEnum::h2NativeBackToSpotGUI, {OptionalCheatEnum::H2NativeMovement}},
{GUIElementEnum::h2NativeAutoCrouchGUI, {OptionalCheatEnum::H2NativeMovement}},
{GUIElementEnum::h2NativeGridGUI, {OptionalCheatEnum::H2CollisionViewer}},
{GUIElementEnum::h2NativeTickrateGUI, {OptionalCheatEnum::H2MasterTickrate}},
{GUIElementEnum::h2NativeClipGUI, {OptionalCheatEnum::BottomlessClip}},
{GUIElementEnum::aiFreezeGUI, {OptionalCheatEnum::AIFreeze}},
{GUIElementEnum::autoCrouchGUI, {OptionalCheatEnum::ForceTeleport, OptionalCheatEnum::GetPlayerViewAngle, OptionalCheatEnum::GetBipedsVehicleDatum}},
{GUIElementEnum::GUIShowingPausesGame, {OptionalCheatEnum::TogglePause}},
{GUIElementEnum::GUIShowingFreesCursor, {OptionalCheatEnum::ToggleFreeCursor}},
{GUIElementEnum::GUIShowingBlocksInput, {OptionalCheatEnum::ToggleBlockInput}},
{GUIElementEnum::togglePauseGUI, {OptionalCheatEnum::TogglePause}},
{GUIElementEnum::advanceTicksGUI, {OptionalCheatEnum::AdvanceTicks, OptionalCheatEnum::TogglePause}},
{GUIElementEnum::pauseAlsoFreesCursorGUI, {OptionalCheatEnum::TogglePause, OptionalCheatEnum::ToggleFreeCursor}},
{GUIElementEnum::pauseAlsoBlocksInputGUI, {OptionalCheatEnum::TogglePause, OptionalCheatEnum::ToggleBlockInput}},
{GUIElementEnum::OBSBypassToggleGUI, {OptionalCheatEnum::OBSBypassCheck}},
{GUIElementEnum::halo2PhysicsHeadingGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pStatusGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pPhysicsToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pKeepArmedGUI, {OptionalCheatEnum::Halo2Physics, OptionalCheatEnum::GameTickEventHook}},
{GUIElementEnum::h2pArmGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pFlyToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pFlySpeedGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pFlyHoldGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pBarriersGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pTick30ToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pCombosToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pNoFallDeathsToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pDisableAIGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pForgeBounceGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::wireViewToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pWorldLinesToggleGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pGridWidthGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pGridOpacityGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pGridNoBloomGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::spotSaveGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::spotBackGUI, {OptionalCheatEnum::Halo2Physics}},
{GUIElementEnum::h2pBottomlessClipGUI, {OptionalCheatEnum::BottomlessClip}},
{GUIElementEnum::invulnGUI, {OptionalCheatEnum::Invulnerability}},
{GUIElementEnum::hideHUDToggle, {OptionalCheatEnum::HideHUD}},

};




#define MAKE_MAPPAIR(r, element, i, game) {GUIElementEnum::element, GameState::Value::game},
#define MAKE_PAIRWISE_SET(r, d, seq) 	BOOST_PP_SEQ_FOR_EACH_I(MAKE_MAPPAIR, BOOST_PP_TUPLE_ELEM(0, seq), BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_TUPLE_ELEM(1, seq)))
#define MAKE_ALL_PAIRWISE(seq) BOOST_PP_SEQ_FOR_EACH(MAKE_PAIRWISE_SET, _, seq)



const std::vector<std::pair< GUIElementEnum, GameState>> GUIRequiredServices::toplevelGUIElements =
{
	MAKE_ALL_PAIRWISE(TOPGUIELEMENTS_ANDSUPPORTEDGAMES)
};




#define MAKE_GAMESTATE(s, data, game) GameState::Value::game
#define MAKE_MAPSET(element, games) {GUIElementEnum::element, {BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(MAKE_GAMESTATE, _, games))} },
#define MAKE_MAPSET_SET(r, d, i, seq) 	MAKE_MAPSET(BOOST_PP_TUPLE_ELEM(0, seq), BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_TUPLE_ELEM(1, seq))) 
#define MAKE_ALL_MAPSET(seq) BOOST_PP_SEQ_FOR_EACH_I(MAKE_MAPSET_SET, _, seq) 

const std::map<GUIElementEnum, std::set<GameState>> GUIRequiredServices::supportedGamesPerGUIElement =
{
	MAKE_ALL_MAPSET(RELEASEGUIELEMENTS_ANDSUPPORTEDGAMES1)
	MAKE_ALL_MAPSET(RELEASEGUIELEMENTS_ANDSUPPORTEDGAMES2)
#ifdef HCM_DEBUG
	MAKE_ALL_MAPSET(DEBUGGUIELEMENTS_ANDSUPPORTEDGAMES)
#endif
};




std::set<std::pair<GameState, OptionalCheatEnum>> outRequiredServices;
bool initRequiredServices = false;
const std::set<std::pair<GameState, OptionalCheatEnum>>& GUIRequiredServices::getAllRequiredServices()
{
	if (initRequiredServices) return outRequiredServices;

	for (auto& [element, cheat] : requiredServicesPerGUIElement)
	{
		if (!supportedGamesPerGUIElement.contains(element)) throw HCMInitException("Somehow supportedGames was missing an element");
		for (auto& game : supportedGamesPerGUIElement.at(element))
		{
            const bool practiceGame = game == GameState(GameState::Value::Halo3) || game == GameState(GameState::Value::Halo3ODST) || game == GameState(GameState::Value::HaloReach);
            const bool overlayControl = element == GUIElementEnum::GUIShowingPausesGame || element == GUIElementEnum::GUIShowingFreesCursor || element == GUIElementEnum::GUIShowingBlocksInput || element == GUIElementEnum::togglePauseGUI || element == GUIElementEnum::pauseAlsoFreesCursorGUI || element == GUIElementEnum::pauseAlsoBlocksInputGUI || element == GUIElementEnum::OBSBypassToggleGUI;
            const bool h2Control = game == GameState(GameState::Value::Halo2) &&
                (element == GUIElementEnum::h2NativeSaveSpotGUI || element == GUIElementEnum::h2NativeBackToSpotGUI || element == GUIElementEnum::h2NativeAutoCrouchGUI || element == GUIElementEnum::h2NativeGridGUI || element == GUIElementEnum::h2NativeTickrateGUI ||
                 element == GUIElementEnum::h2NativeClipGUI || element == GUIElementEnum::aiFreezeGUI ||
                 element == GUIElementEnum::invulnGUI || element == GUIElementEnum::hideHUDToggle);
            if (!practiceGame && !overlayControl && !h2Control) continue;

			if (!requiredServicesPerGUIElement.contains(element)) continue; // element has no required services
			auto reqServs = requiredServicesPerGUIElement.at(element);

			for (auto req : reqServs)
			{
				outRequiredServices.insert(std::make_pair( game, req ));
			}
		}
	}


	initRequiredServices = true;
	return outRequiredServices;

	//if (initRequiredServices) return outRequiredServices;

	//for (auto& [element, game] : toplevelGUIElements)
	//{
	//		if (!requiredServicesPerGUIElement.contains(element)) continue; // element has no required services

	//		auto reqServs = requiredServicesPerGUIElement.at(element);

	//		for (auto req : reqServs)
	//		{
	//			outRequiredServices.push_back({ game, req });
	//		}
	//}
	//initRequiredServices = true;
	//return outRequiredServices;
};
