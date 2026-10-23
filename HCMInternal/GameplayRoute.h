#pragma once
#include "H2TransitionState.h"
#include "RetirementBackoff.h"

// Opt-in routes for tick and invulnerability hooks. Requested flags live on
// the hooks; suspending a route never changes the user's selection.
struct GameplayRoute {
    const wchar_t* module;
    H2TransitionState state;
    HMODULE lease=nullptr;
    uint64_t retiredRevision=0;
    RetirementBackoff retry;
    std::string status;
    explicit GameplayRoute(const wchar_t* name) : module(name) {}
};
