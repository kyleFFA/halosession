#include "pch.h"
#include "HCMInternalGUI.h"
#include "PCMTheme.h"
#include "GlobalKill.h"
#include "GUISimpleButton.h"
#include "MCCStateHook.h"
#include "imgui.h"

#include "PcmVersion.h"
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
    if(quickMenu)quickMenu->cancel();
    pauseGameRequest.reset();

	p_currentGameGUIElements = &mGUIStore->getTopLevelGUIElements(newState.currentGameState);
    ++sketchState.contentRevision;
	LOG_ONCE(PLOG_VERBOSE << "unlocking currentGameGUIElementsMutex");
}





void HCMInternalGUI::primaryRender()
{
	LOG_ONCE_CAPTURE(PLOG_VERBOSE << "onGameStateChange locking currentGameGUIElementsMutex @ 0x" << std::hex << pMutex, pMutex = &currentGameGUIElementsMutex);
	std::unique_lock<std::mutex> lock(currentGameGUIElementsMutex);
    SessionSketch::render(sketchState,mFullScreenSize,*mIsCursorShowing,m_WindowOpen,[&](int tab) {
        if(tab==3) {if(quickMenu)quickMenu->settings();return;}
        if(tab==2) {
            ImGui::TextWrapped("Credits: Burnt / Poorboy");
            ImGui::Spacing();
            ImGui::TextWrapped("Based on Poorboys / HCM by Poorboy, Burnt-o and Harc");
            ImGui::Spacing();
            ImGui::TextWrapped("KyleFFA (badstrafe)");
            return;
        }
        mGUIMCCState.render();
        bool any=false;
        for(auto& element:*p_currentGameGUIElements) {
            const bool control=element->getName()=="Control";
            if((tab==1)==control){element->render(*mHotkeyRenderer);any=true;}
        }
        if(!any)ImGui::TextWrapped("No practice controls are available for this title.");
    });
    SessionSketch::clamp(sketchState,mFullScreenSize);
    if(quickMenu)quickMenu->frame(m_WindowOpen || !errorsToDisplay.empty() || ImGui::GetIO().WantTextInput || ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId),sketchState);
    onGUIShowingPausesGameChanged(mSettings->GUIShowingPausesGame->GetValueDisplay());
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


void HCMInternalGUI::onGUIShowingBlocksGameInputChanged(bool& newval)
{
	if (newval && m_WindowOpen && mControlServices->blockGameInputService.has_value() && !blockGameInputRequest)
	{
		blockGameInputRequest = mControlServices->blockGameInputService.value()->makeScopedRequest();
	}
	else if (!newval && m_WindowOpen && blockGameInputRequest)
	{
		blockGameInputRequest.reset();
	}
}

void HCMInternalGUI::onGUIShowingPausesGameChanged(bool& newval)
{
    const auto current=mccStateHook->getCurrentMCCState();
    const bool solo=current.currentGameState==GameState(GameState::Value::Halo2) && current.currentPlayState==PlayState::Ingame && QuickMenu::verifiedSoloCampaign();
    const bool wanted=newval && m_WindowOpen && solo;
    if(wanted && mControlServices->pauseGameService.has_value() && !pauseGameRequest)
        pauseGameRequest=mControlServices->pauseGameService.value()->makeScopedRequest();
    else if(!wanted)pauseGameRequest.reset();
}

void HCMInternalGUI::onWindowJustOpened()
{
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

	if (freeCursorRequest)
		freeCursorRequest.reset();

	if (blockGameInputRequest)
		blockGameInputRequest.reset();

	if (pauseGameRequest)
		pauseGameRequest.reset();
}

