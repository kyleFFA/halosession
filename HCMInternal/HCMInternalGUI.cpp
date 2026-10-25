#include "pch.h"
#include "SessionOBSControls.h"
#include "HCMInternalGUI.h"
#include "PCMTheme.h"
#include "GlobalKill.h"
#include "GUISimpleButton.h"
#include "MCCStateHook.h"
#include "imgui.h"

#include "PcmVersion.h"
#include "MenuMovementPolicy.h"
#include "Halo2Physics.h"
#define addTooltip(x) if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip(x)



ImVec2 minimumWindowSize{ 500, 500 };

void HCMInternalGUI::adjustWindowPosition()
{
	// aspect ratio adjustment to main gui position so that it appears over the top of the games ammo counter ui
	constexpr float ratio16to9 = 16.f / 9.f;
	float currentRatio = mFullScreenSize.x / mFullScreenSize.y;

	if (currentRatio > ratio16to9) // need to move HCM window pos to the right
	{
		// if the aspect radio were 16:9, how wide would the screen be?
		float widthIf16to9 = mFullScreenSize.y * ratio16to9;
		float currentExtraWidth = mFullScreenSize.x - widthIf16to9;
		PLOG_DEBUG << "currentExtraWidth: " << currentExtraWidth;

		constexpr float magicAdjustmentFactor = 0.6;


		float delta = currentExtraWidth * 0.5;// halve it since each side gets half the extra width
		delta = delta * magicAdjustmentFactor; // idk

		mWindowPos = SimpleMath::Vector2(10 + delta, 25);
	}
	else
	{
		mWindowPos = SimpleMath::Vector2(10, 25);
	}
}


void HCMInternalGUI::initializeHCMInternalGUI()
{
	ImGui::SetNextWindowCollapsed(false);
	windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

	PLOG_VERBOSE << "actual screen size: " << mFullScreenSize.x << ", " << mFullScreenSize.y;
	if (mFullScreenSize.x > minimumWindowSize.x && mFullScreenSize.y > minimumWindowSize.y) // check for really small screens or getScreenSize returning a junk value, then we just use default 500, 500
	{
		// adjust vertical window height to be 2/3rds of screen
		minimumWindowSize.y = mFullScreenSize.y / 3 * 2;
	}

	adjustWindowPosition();

}



void HCMInternalGUI::onImGuiRenderEvent(SimpleMath::Vector2 screenSize)
{
	std::unique_lock<std::mutex> lock(mDestructionGuard);

	if (mFullScreenSize != screenSize) // recalculate our gui canvas size
	{
		mFullScreenSize = screenSize;
		if (screenSize.x > minimumWindowSize.x && screenSize.y > minimumWindowSize.y) // check for really small screens or getScreenSize returning a junk value, then we just use default 500, 500
		{
			// adjust vertical window height to be 2/3rds of screen
			minimumWindowSize.y = screenSize.y / 3 * 2;
		}

		adjustWindowPosition();
	}


	if (!m_HCMInternalGUIinitialized)
	{
		onGameStateChange(mccStateHook->getCurrentMCCState());

		PLOG_INFO << "Initializing HCMInternalGUI";
		try
		{
			initializeHCMInternalGUI();
			m_HCMInternalGUIinitialized = true;
		}
		catch (HCMInitException& ex)
		{
			PLOG_FATAL << "Failed to initialize HCMInternalGUI, info: " << std::endl
				<< ex.what() << std::endl
				<< "HCM will now automatically close down";
			GlobalKill::killMe();
			return;
		}
	}


	renderErrorDialog();
	primaryRender();

}


static bool errorDialogShowing = false;
void HCMInternalGUI::renderErrorDialog()
{
	if (errorsToDisplay.empty())
	{
		return;
	}

	if (!errorDialogShowing)
	{
		ImGui::OpenPopup("Error!");
		errorDialogShowing = true;
	}

	SimpleMath::Vector2 size = SimpleMath::Vector2(300, 0);
	SimpleMath::Vector2 pos = SimpleMath::Vector2(mFullScreenSize / 2.f) - (size / 2.f);

	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowPos(pos);

	if (ImGui::BeginPopupModal("Error!", NULL, ImGuiWindowFlags_NoResize))
	{

		ImGui::TextWrapped("HCM encountered an error!");
		ImGui::TextWrapped(errorsToDisplay.front().what());
		ImGui::TextWrapped("");
		ImGui::TextWrapped("Technical details: ");

		ImGui::BeginChild("trace", ImVec2(ImGui::GetWindowSize().x - 15, 150), NULL);
		ImGui::TextWrapped("The error occured here:");
		ImGui::TextWrapped(errorsToDisplay.front().source().c_str());
		ImGui::TextWrapped("");
		ImGui::TextWrapped("Full stack trace:");
		ImGui::TextWrapped(errorsToDisplay.front().trace().c_str());
		ImGui::EndChild();

		if (ImGui::Button("OK"))
		{
			errorDialogShowing = false;
			errorsToDisplay.erase(errorsToDisplay.begin());
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Copy to clipboard"))
		{
			std::string copyText = std::format("{}\n\n{}\n\n{}", errorsToDisplay.front().what(), errorsToDisplay.front().source(), errorsToDisplay.front().trace());
			ImGui::SetClipboardText(copyText.c_str());
		}
		ImGui::EndPopup();
	}

}

//https://www.modernescpp.com/index.php/dealing-with-mutation-locking/


void HCMInternalGUI::onGameStateChange(const MCCState& newState)
{

	LOG_ONCE_CAPTURE(PLOG_VERBOSE << "onGameStateChange locking currentGameGUIElementsMutex @ 0x" << std::hex << pMutex, pMutex = &currentGameGUIElementsMutex);
	std::unique_lock<std::mutex> lock(currentGameGUIElementsMutex);
    overlayEditing=false;overlayTransition=GetTickCount64();
    if(quickMenu)quickMenu->cancel();
    pauseGameRequest.reset();
    if(quickMenu)quickMenu->mainMenu(false,false,true);
    menuMovementRequest.reset();

	p_currentGameGUIElements = &mGUIStore->getTopLevelGUIElements(newState.currentGameState);
    ++sketchState.contentRevision;
	LOG_ONCE(PLOG_VERBOSE << "unlocking currentGameGUIElementsMutex");
}





void HCMInternalGUI::primaryRender()
{
	LOG_ONCE_CAPTURE(PLOG_VERBOSE << "onGameStateChange locking currentGameGUIElementsMutex @ 0x" << std::hex << pMutex, pMutex = &currentGameGUIElementsMutex);
	std::unique_lock<std::mutex> lock(currentGameGUIElementsMutex);
    bool mouseHeld=false;for(bool down:ImGui::GetIO().MouseDown)mouseHeld|=down;
    auto presentation=menuPresentation.sample(m_WindowOpen,quickMenu&&quickMenu->mainMenuWheelActive(),*mIsCursorShowing,mouseHeld);
    sketchState.workshopDashboard=true;
    if(presentation.showMain)SessionSketch::render(sketchState,mFullScreenSize,presentation.inputs,m_WindowOpen,[&](int tab) {
        if(tab==3) {if(quickMenu)quickMenu->settings();return;}
        if(tab==2) {
            ImGui::TextWrapped("Credits: Burnt / Poorboy");
            ImGui::Spacing();
            ImGui::TextWrapped("Based on Poorboys / HCM by Poorboy, Burnt-o and Harc");
            ImGui::Spacing();
            ImGui::TextWrapped("KyleFFA (badstrafe)");
            return;
        }
        bool any=false;
        if(tab==1){mGUIMCCState.render();
            if(sessionobs::controls(mSettings->OBSBypassToggle->GetValueDisplay()))mSettings->OBSBypassToggle->UpdateValueWithInput();
            renderSpeedOverlayControls();for(auto& element:*p_currentGameGUIElements){element->renderControlPanel(*mHotkeyRenderer);if(element->getName()=="Practice dashboard")any=true;}}
        else for(auto& element:*p_currentGameGUIElements){if(element->getName()!="Control"){element->render(*mHotkeyRenderer);if(element->getName()=="Practice dashboard")any=true;}}
        if(!any)ImGui::TextWrapped("Live telemetry is unavailable for this title.");
    },[&]{for(auto& element:*p_currentGameGUIElements)element->renderAppearancePanel(*mHotkeyRenderer);});
    SessionSketch::clamp(sketchState,mFullScreenSize);


    mWindowSize={sketchState.size.x,sketchState.size.y};
    mWindowPos={sketchState.position.x,sketchState.position.y};

	// do stuff if window just opened
	if (m_WindowOpen && !m_windowOpenLastFrame)
	{
		onWindowJustOpened();
	}
	// do stuff if window just closed
	else if (!m_WindowOpen && m_windowOpenLastFrame)
	{
		onWindowJustClosed();
	}

    updateMenuInput();
    renderSpeedOverlay();
    if(quickMenu)quickMenu->frame(!errorsToDisplay.empty() || ImGui::GetIO().WantTextInput || ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId) || ImGui::IsAnyItemActive() || mouseHeld,sketchState);
    onGUIShowingPausesGameChanged(mSettings->GUIShowingPausesGame->GetValueDisplay());
	m_windowOpenLastFrame = m_WindowOpen;

	LOG_ONCE(PLOG_VERBOSE << "unlocking currentGameGUIElementsMutex");

	//// little colour picker for gui stuff
	//ImGuiStyle* style = &ImGui::GetStyle();
	//ImVec4 newCol = style->Colors[ImGuiCol_ChildBg];
	//if (ImGui::ColorPicker4("col", &newCol.x))
	//{
	//	style->Colors[ImGuiCol_ChildBg] = newCol;
	//}

}

void HCMInternalGUI::onGUIShowingFreesMCCCursorChanged(bool& newval)
{
	if (newval && m_WindowOpen && mControlServices->freeMCCSCursorService.has_value() && !freeCursorRequest)
	{
		freeCursorRequest = mControlServices->freeMCCSCursorService.value()->makeScopedRequest();
	}
	else if (!newval && m_WindowOpen && freeCursorRequest)
	{
		freeCursorRequest.reset();
	}
}


void HCMInternalGUI::onGUIShowingBlocksGameInputChanged(bool&){
 // Setting callbacks may run on a widget worker thread. primaryRender applies the policy every frame.
}
static bool menuKeysHeld(){
 const auto& io=ImGui::GetIO();for(bool down:io.MouseDown)if(down)return true;
 for(int key=ImGuiKey_NamedKey_BEGIN;key<ImGuiKey_GamepadStart;++key)if(ImGui::IsKeyDown(ImGuiKey(key)))return true;return false;
}
void HCMInternalGUI::updateMenuInput(){
 menuReleaseGuard=menumovement::retainReleaseGuard(menuReleaseGuard,m_WindowOpen,menuKeysHeld());
 bool mouseHeld=false;for(bool down:ImGui::GetIO().MouseDown)mouseHeld|=down;
 const bool modal=!errorsToDisplay.empty()||ImGui::GetIO().WantTextInput||ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId);
 const bool wheelConflict=modal||ImGui::IsAnyItemActive()||mouseHeld;
 const bool conflict=sketchState.fullControllerPassthrough?modal:wheelConflict;
 bool ready=quickMenu&&quickMenu->mainMenu(m_WindowOpen,sketchState.allowMenuMovement,conflict,sketchState.allowMenuCamera,sketchState.fullControllerPassthrough,wheelConflict);
 auto decision=menumovement::decide(m_WindowOpen,sketchState.allowMenuMovement||sketchState.fullControllerPassthrough,ready,mSettings->GUIShowingBlocksInput->GetValue(),false,false,conflict,quickMenu&&quickMenu->mainMenuWheelAvailable());
 if(!mControlServices->blockGameInputService)return;auto service=mControlServices->blockGameInputService.value();
 if(menuReleaseGuard)decision.keyboardMouseBlocked=true;
 // Acquire the stricter guard before releasing the other token on every transition.
 if(decision.controllerBlocked){if(!blockGameInputRequest)blockGameInputRequest=service->makeScopedRequest();menuMovementRequest.reset();}
 else if(decision.keyboardMouseBlocked){if(!menuMovementRequest)menuMovementRequest=service->makeMenuMovementRequest();blockGameInputRequest.reset();}
 else{menuMovementRequest.reset();blockGameInputRequest.reset();}
}

void HCMInternalGUI::onGUIShowingPausesGameChanged(bool& newval)
{
    const auto current=mccStateHook->getCurrentMCCState();
    const bool solo=current.currentGameState==GameState(GameState::Value::Halo2) && current.currentPlayState==PlayState::Ingame && QuickMenu::verifiedSoloCampaign();
    const bool wanted=menumovement::pauseMain(newval,m_WindowOpen,solo,sketchState.allowMenuMovement||sketchState.fullControllerPassthrough,quickMenu&&quickMenu->mainMenuWheelActive());
    if(wanted && mControlServices->pauseGameService.has_value() && !pauseGameRequest)
        pauseGameRequest=mControlServices->pauseGameService.value()->makeScopedRequest();
    else if(!wanted)pauseGameRequest.reset();
}

void HCMInternalGUI::onWindowJustOpened()
{
    menuReleaseGuard=false;
	mSettings->GUIWindowOpen->GetValueDisplay() = m_WindowOpen;
	mSettings->GUIWindowOpen->UpdateValueWithInput();

	if (mSettings->GUIShowingFreesCursor->GetValue() && mControlServices->freeMCCSCursorService.has_value())
		freeCursorRequest = mControlServices->freeMCCSCursorService.value()->makeScopedRequest();

	if (mSettings->GUIShowingBlocksInput->GetValue() && mControlServices->blockGameInputService.has_value())
		blockGameInputRequest = mControlServices->blockGameInputService.value()->makeScopedRequest();

	onGUIShowingPausesGameChanged(mSettings->GUIShowingPausesGame->GetValueDisplay());
}

void HCMInternalGUI::onWindowJustClosed()
{
	mSettings->GUIWindowOpen->GetValueDisplay() = m_WindowOpen;
	mSettings->GUIWindowOpen->UpdateValueWithInput();

    menuReleaseGuard=menuKeysHeld();
    if(menuReleaseGuard&&mControlServices->blockGameInputService&&!menuMovementRequest)menuMovementRequest=mControlServices->blockGameInputService.value()->makeMenuMovementRequest();
    if(!menuReleaseGuard)menuMovementRequest.reset();
    if(quickMenu)quickMenu->mainMenu(false,false,true);
	if (freeCursorRequest)
		freeCursorRequest.reset();

	if (blockGameInputRequest)
		blockGameInputRequest.reset();

	if (pauseGameRequest)
		pauseGameRequest.reset();
}

void HCMInternalGUI::renderSpeedOverlayControls(){
 if(!ImGui::CollapsingHeader("Speed Overlay##section",ImGuiTreeNodeFlags_DefaultOpen))return;
 ImGui::PushID("speed-overlay-options");
 mHotkeyRenderer->renderHotkey(RebindableHotkeyEnum::speedOverlayHotkey);ImGui::SameLine();
 if(ImGui::Checkbox("Speed Overlay",&mSettings->speedOverlayVisible->GetValueDisplay()))mSettings->speedOverlayVisible->UpdateValueWithInput();
 ImGui::TextWrapped("Independent speed text during gameplay. Bind with ..; locked display does not capture input or pause. Uses existing physics telemetry only; unsupported or stale data shows --.");
 auto& height=mSettings->speedOverlayHeight->GetValueDisplay();float percent=height*100.f;if(ImGui::SliderFloat("Digit height (% screen)",&percent,1.5f,50.f,"%.1f%%")){height=percent/100.f;mSettings->speedOverlayHeight->UpdateValueWithInput();}
 auto& color=mSettings->speedOverlayColor->GetValueDisplay();if(ImGui::ColorEdit3("Overlay text color",&color.x))mSettings->speedOverlayColor->UpdateValueWithInput();if(ImGui::SliderFloat("Overlay opacity",&color.w,0.f,1.f,"%.2f"))mSettings->speedOverlayColor->UpdateValueWithInput();
 auto& position=mSettings->speedOverlayPosition->GetValueDisplay();if(ImGui::SliderFloat2("Position (X / Y)",&position.x,0.f,1.f,"%.2f"))mSettings->speedOverlayPosition->UpdateValueWithInput();
 const bool canEdit=mSettings->speedOverlayVisible->GetValue()&&mControlServices->freeMCCSCursorService&&mControlServices->blockGameInputService;
 ImGui::BeginDisabled(!canEdit);ImGui::Checkbox("Edit overlay position",&overlayEditing);ImGui::EndDisabled();
 ImGui::TextWrapped("Edit: close the main menu, drag the text, then click Lock position or press Escape. Only this mode captures keyboard/mouse input; controller gameplay and radial priority are retained. Position, size, color, opacity and visibility are saved on normal Session shutdown.");
 ImGui::TextWrapped("Half-screen digit height is supported through a 4K output. Physical height is bounded at 1080 pixels. Size changes settle for 0.25 seconds before a background font build; digits are never enlarged past their cached raster. Longer values shrink to fit the screen.");
 if(!speedOverlay.status().empty())ImGui::TextWrapped("%s",speedOverlay.status().c_str());
 ImGui::PopID();
}

void HCMInternalGUI::renderSpeedOverlay(){
 const auto current=mccStateHook->getCurrentMCCState();const auto game=unsigned(current.currentGameState.operator GameState::Value());
 speedoverlay::Options options;options.visible=mSettings->speedOverlayVisible->GetValue();options.height=mSettings->speedOverlayHeight->GetValue();const auto color=mSettings->speedOverlayColor->GetValue();options.color={color.x,color.y,color.z,color.w};const auto position=mSettings->speedOverlayPosition->GetValue();options.position={position.x,position.y};
 const bool canEdit=mControlServices->freeMCCSCursorService&&mControlServices->blockGameInputService;if(!canEdit)overlayEditing=false;
 DWORD focused=0;GetWindowThreadProcessId(GetForegroundWindow(),&focused);if(focused!=GetCurrentProcessId())overlayEditing=false;
 H3sbStatus status{};status.size=sizeof status;const bool candidate=options.visible&&current.currentPlayState==PlayState::Ingame&&(game==2||game==5||game==6);const bool verified=candidate&&quickMenu&&quickMenu->verifiedPlayer(game);const bool reported=verified&&Halo2Physics::readStatus(&status)==1;
 auto value=speedoverlay::reading(game,current.currentPlayState==PlayState::Ingame,verified,Halo2Physics::isLoaded(),reported,status,GetTickCount64(),overlayTransition);
 const bool interactive=errorsToDisplay.empty()&&!ImGui::GetIO().WantTextInput&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId)&&!mSettings->quickMenuCapturing.load();
 speedOverlay.render(options,value,ImGui::GetIO().DisplaySize,SessionSketch::renderDevice(),overlayEditing,interactive);
 if(options.position.x!=position.x||options.position.y!=position.y){mSettings->speedOverlayPosition->GetValueDisplay()={options.position.x,options.position.y};mSettings->speedOverlayPosition->UpdateValueWithInput();}
 const auto requests=speedoverlay::capture(options.visible,overlayEditing,canEdit,menuKeysHeld(),overlayWasEditing,overlayReleaseGuard);
 if(requests.cursor){if(!overlayCursorRequest)overlayCursorRequest=mControlServices->freeMCCSCursorService.value()->makeScopedRequest();}else overlayCursorRequest.reset();
 if(requests.pointer&&mControlServices->blockGameInputService){if(!overlayPointerRequest)overlayPointerRequest=mControlServices->blockGameInputService.value()->makeMenuMovementRequest();}else overlayPointerRequest.reset();
}
