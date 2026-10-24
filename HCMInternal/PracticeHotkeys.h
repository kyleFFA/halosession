#pragma once
#include "HotkeysEnum.h"

// Keep legacy definitions for existing services, but never dispatch their input.
// This applies after deserialization too, so old bindings cannot re-enable them.
inline bool isPracticeHotkey(RebindableHotkeyEnum key)
{
    switch (key)
    {
    case RebindableHotkeyEnum::toggleGUI:
    case RebindableHotkeyEnum::speedOverlayHotkey:
    case RebindableHotkeyEnum::togglePause:
    case RebindableHotkeyEnum::advanceTicks:
    case RebindableHotkeyEnum::invuln:
    case RebindableHotkeyEnum::bottomlessClip:
    case RebindableHotkeyEnum::hideHUDToggle:
    case RebindableHotkeyEnum::h2pPhysicsHotkey:
    case RebindableHotkeyEnum::h2pBarriersCycleHotkey:
    case RebindableHotkeyEnum::h2pTick30Hotkey:
    case RebindableHotkeyEnum::h2pCombosHotkey:
    case RebindableHotkeyEnum::h2pNoFallDeathsHotkey:
    case RebindableHotkeyEnum::h2pArmHotkey:
    case RebindableHotkeyEnum::h2pFlyHotkey:
    case RebindableHotkeyEnum::h2pDisableAIHotkey:
    case RebindableHotkeyEnum::h2pForgeBounceHotkey:
    case RebindableHotkeyEnum::h2pWorldLinesHotkey:
    case RebindableHotkeyEnum::spotSaveHotkey:
    case RebindableHotkeyEnum::spotBackHotkey:
    case RebindableHotkeyEnum::h2pKeepArmedHotkey:
    case RebindableHotkeyEnum::autoCrouchHotkey:
        return true;
    default:
        return false;
    }
}

// MCC indicator values: 1 = original H2, 4 = Anniversary multiplayer.
// H2 uses its own actions; movement uses H2NativeMovement; physics-mod keys cannot reach it.
inline bool isPracticeHotkeyForGame(RebindableHotkeyEnum key, unsigned game)
{
    if(key==RebindableHotkeyEnum::speedOverlayHotkey)return true;
    if (game != 1 && game != 4) return isPracticeHotkey(key);
    if (key == RebindableHotkeyEnum::toggleGUI) return true;
    if (game == 4) return false;
    if (key == RebindableHotkeyEnum::togglePause) return true;
    switch (key) {
    case RebindableHotkeyEnum::spotSaveHotkey:
    case RebindableHotkeyEnum::spotBackHotkey:
    case RebindableHotkeyEnum::autoCrouchHotkey:
    case RebindableHotkeyEnum::invuln:
    case RebindableHotkeyEnum::bottomlessClip:
    case RebindableHotkeyEnum::hideHUDToggle:
    case RebindableHotkeyEnum::aiFreeze:
    case RebindableHotkeyEnum::h2pTick30Hotkey:
    case RebindableHotkeyEnum::h2pWorldLinesHotkey:
        return true;
    default: return false;
    }
}
