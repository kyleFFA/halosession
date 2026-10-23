#include "../HCMInternal/PracticeHotkeys.h"
#include <cassert>
int main()
{
    // Old bindings must stay inert regardless of what a config file contains.
    for (auto key : {RebindableHotkeyEnum::forceCheckpoint, RebindableHotkeyEnum::forceRevert,
        RebindableHotkeyEnum::forceDoubleRevert, RebindableHotkeyEnum::injectCheckpoint,
        RebindableHotkeyEnum::dumpCheckpoint, RebindableHotkeyEnum::forceCoreSave,
        RebindableHotkeyEnum::forceCoreLoad, RebindableHotkeyEnum::injectCore,
        RebindableHotkeyEnum::dumpCore, RebindableHotkeyEnum::naturalCheckpointDisable,
        RebindableHotkeyEnum::forceTeleport, RebindableHotkeyEnum::forceMissionRestart,
        RebindableHotkeyEnum::freeCamera, RebindableHotkeyEnum::h2pJumpSpotsHotkey})
        assert(!isPracticeHotkey(key));
    for (auto key : {RebindableHotkeyEnum::toggleGUI, RebindableHotkeyEnum::invuln,
        RebindableHotkeyEnum::hideHUDToggle, RebindableHotkeyEnum::h2pTick30Hotkey,
        RebindableHotkeyEnum::h2pFlyHotkey, RebindableHotkeyEnum::h2pArmHotkey,
        RebindableHotkeyEnum::spotSaveHotkey, RebindableHotkeyEnum::spotBackHotkey,
        RebindableHotkeyEnum::autoCrouchHotkey, RebindableHotkeyEnum::h2pKeepArmedHotkey})
        assert(isPracticeHotkey(key));
    assert(!isPracticeHotkey(static_cast<RebindableHotkeyEnum>(9999)));
    std::cout << "Practice hotkeys accept requested controls and reject legacy actions\n";
}

