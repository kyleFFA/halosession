#include "../HCMInternal/pch.h"
#include "../HCMInternal/PracticeHotkeys.h"
#include "../HCMInternal/H2TickrateLifecycle.h"
#include "../HCMInternal/H2TimingPair.h"
#include <cassert>

int main()
{
    for (auto key : magic_enum::enum_values<RebindableHotkeyEnum>()) {
        for (unsigned game : {2u,5u,6u}) assert(isPracticeHotkeyForGame(key,game)==isPracticeHotkey(key));
        if (isPracticeHotkeyForGame(key,4)) assert(key==RebindableHotkeyEnum::toggleGUI || key==RebindableHotkeyEnum::togglePause);
    }
    for (auto key : {RebindableHotkeyEnum::h2pPhysicsHotkey,RebindableHotkeyEnum::h2pCombosHotkey,
        RebindableHotkeyEnum::h2pArmHotkey,RebindableHotkeyEnum::h2pKeepArmedHotkey,
        RebindableHotkeyEnum::h2pFlyHotkey,RebindableHotkeyEnum::h2pBarriersCycleHotkey,RebindableHotkeyEnum::h2pForgeBounceHotkey}) assert(!isPracticeHotkeyForGame(key,1));
    for (auto key : {RebindableHotkeyEnum::invuln,RebindableHotkeyEnum::bottomlessClip,RebindableHotkeyEnum::hideHUDToggle,
        RebindableHotkeyEnum::aiFreeze,RebindableHotkeyEnum::h2pTick30Hotkey,RebindableHotkeyEnum::h2pWorldLinesHotkey,
        RebindableHotkeyEnum::autoCrouchHotkey,RebindableHotkeyEnum::spotSaveHotkey,RebindableHotkeyEnum::spotBackHotkey}) assert(isPracticeHotkeyForGame(key,1));
    assert(!isPracticeHotkeyForGame(RebindableHotkeyEnum::togglePause,4));
    using namespace h2timing;
    // Stock -> 30 does not churn bodies; 30 -> native 60 must rebuild; disabling
    // native 60 restores stock 60/30 rather than leaving 60-scaled bodies behind.
    assert(target(Mode::Stock).rate==60 && target(Mode::Stock).scalar==30);
    assert(target(Mode::Native30).rate==30 && target(Mode::Native30).scalar==30);
    assert(!rebuildRequired(30,Mode::Native30));
    assert(rebuildRequired(30,Mode::Native60));
    assert(rebuildRequired(60,Mode::Stock));
    assert(rebuildRequired(60,Mode::Native30));
    assert(!rebuildRequired(60,Mode::Native60));
    // A level with uncertain body construction cannot skip the repair pass.
    assert(rebuildRequired(std::numeric_limits<float>::quiet_NaN(),Mode::Stock));
    assert(rebuildRequired(std::numeric_limits<float>::quiet_NaN(),Mode::Native60));
    assert(!canRelease(false,true,true,false)); // unresolved timer is NOT restored
    assert(!canRelease(true,true,false,false)); // failed rebuild cannot unload
    assert(canRelease(true,true,true,false));
    assert(canRelease(false,false,false,true)); // confirmed module destruction
    Pair pair{60,1.f/60};
    auto reader=[&](Pair& p){p=pair;return true;};
    int writes=0;
    auto partialFailure=[&](const Pair& p){pair.rate=p.rate;if(++writes==1)return false;pair.dt=p.dt;return true;};
    assert(writePair(30,reader,partialFailure)==WriteResult::RolledBack);
    assert(pair.rate==60 && pair.dt==1.f/60);
    assert(writePair(30,reader,[](const Pair&){return false;})==WriteResult::Uncertain);
    pair={60,1.f/30};writes=0;
    assert(writePair(30,reader,partialFailure)==WriteResult::Refused && writes==0);
    // A successful store is insufficient: test verified application, dropped
    // stores, corrupt readback and rollback that only claims to have succeeded.
    pair={60,1.f/60};
    auto writer=[&](const Pair& p){pair=p;return true;};
    assert(writePair(30,reader,writer)==WriteResult::Applied && matches(pair,30));
    assert(writePair(60,reader,writer)==WriteResult::Applied && matches(pair,60));
    writes=0;
    auto droppedStore=[&](const Pair& p){if(++writes>1)pair=p;return true;};
    assert(writePair(30,reader,droppedStore)==WriteResult::RolledBack && matches(pair,60));
    writes=0;
    auto corruptWriteAndRollback=[&](const Pair& p){++writes;pair={p.rate,0.5f};return true;};
    assert(writePair(30,reader,corruptWriteAndRollback)==WriteResult::Uncertain && writes==2);
    pair={60,1.f/60};int reads=0;
    auto failedConfirmation=[&](Pair& p){p=pair;return ++reads!=2;};
    assert(writePair(30,failedConfirmation,writer)==WriteResult::RolledBack && matches(pair,60));
    pair={60,1.f/60};reads=0;
    auto unreadableAfterWrite=[&](Pair& p){p=pair;return ++reads==1;};
    assert(writePair(30,unreadableAfterWrite,writer)==WriteResult::Uncertain);
    pair={60,1.f/60};writes=0;
    assert(writePair(0,reader,partialFailure)==WriteResult::Refused && writes==0);
    pair={60,std::numeric_limits<float>::quiet_NaN()};
    assert(writePair(30,reader,partialFailure)==WriteResult::Refused && writes==0);
    std::cout << "PASS: original-H2/H2A/three-game hotkey isolation and native timing restoration decisions\n";
}
