#pragma once
// Disable every entry first. A failed disable retains all callback ownership;
// no trampoline drain/release is permitted until the entire set is disabled.
template<class Hooks> bool retireHooks(const Hooks& hooks) {
    bool disabled=true;
    for(auto* hook:hooks) if(!hook->prepareDetach()) disabled=false;
    if(!disabled) return false;
    bool restored=true;
    for(auto* hook:hooks) if(!hook->detach()) restored=false;
    return restored;
}
