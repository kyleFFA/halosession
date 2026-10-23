#pragma once
#include <atomic>
#include <cstdint>

// Loader reference churn is not title entry. Only MCC gameplay events may
// request activation; every inactive event must be retired before re-entry.
class H2TransitionState {
    std::atomic<bool> requested{false};
    std::atomic<bool> running{false};
    std::atomic<uint64_t> retireRevision{0};
public:
    void observe(bool playing) {
        if (!playing) { running=false; ++retireRevision; }
        requested=playing;
    }
    bool wantsGameplay() const { return requested.load(); }
    bool allowsEffects() const { return running.load(); }
    uint64_t retirement() const { return retireRevision.load(); }
    bool activate(uint64_t completed) {
        if (!requested || completed!=retireRevision.load()) return false;
        running=true;
        if (!requested || completed!=retireRevision.load()) { running=false; return false; }
        return true;
    }
    void suspend() { running=false; }
};
