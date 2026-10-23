#pragma once

// H2's unmodified engine uses 60 Hz with a collision-shape scalar of 30.
// Native 30/60 control matches both values. Disabling it restores BOTH stock
// values, including already-built bodies if their scalar changed.
namespace h2timing
{
    enum class Mode { Stock, Native30, Native60 };
    constexpr const char* name(Mode mode) {
        return mode==Mode::Native30 ? "Native30" : mode==Mode::Native60 ? "Native60" : "Stock";
    }
    struct Target { short rate; float scalar; };
    constexpr Target target(Mode mode)
    {
        return mode == Mode::Native30 ? Target{30, 30.f} :
               mode == Mode::Native60 ? Target{60, 60.f} : Target{60, 30.f};
    }
    constexpr bool canRelease(bool timerVerified, bool scalarRestored, bool bodiesRestoredOrGone, bool moduleGone)
    { return moduleGone || (timerVerified && scalarRestored && bodiesRestoredOrGone); }
    constexpr bool rebuildRequired(float bodyScalar, Mode requested)
    {
        return bodyScalar != target(requested).scalar;
    }
}
