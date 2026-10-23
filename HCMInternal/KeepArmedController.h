#pragma once
#include "h3sb_api.h"
#include <mutex>
#include <cstddef>
#include <cmath>

enum class PhysicsHostGame { Halo3, Halo3ODST, HaloReach };
// Bundled h3sb status ABI distinguishes ODST from the shared H3/Reach path.
// This is not MCC's GameState enum: Reach reports 1, not 3.
constexpr int physicsStatusGame(PhysicsHostGame game)
{
    return game == PhysicsHostGame::Halo3ODST ? 2 : 1;
}

// Owns only intent, never a biped address. The native API reacquires its local body.
struct KeepArmedStats { uint64_t ticks=0, suspended=0, alreadyArmed=0, requests=0, accepted=0, idleRequests=0, firstMs=0, lastMs=0; };
class KeepArmedController
{
    std::mutex mutex;
    KeepArmedStats counters;
    bool enabled=false, physics=false, overlay=false, paused=false, stopped=false;
public:
    KeepArmedStats stats(){std::lock_guard lock(mutex);return counters;}
    void resetStats(){std::lock_guard lock(mutex);counters={};}
    void setEnabled(bool v){std::lock_guard lock(mutex);enabled=v;}
    void setPhysics(bool v){std::lock_guard lock(mutex);physics=v;}
    void setOverlay(bool v){std::lock_guard lock(mutex);overlay=v;}
    void setPaused(bool v){std::lock_guard lock(mutex);paused=v;}
    void stop(){std::lock_guard lock(mutex);stopped=true;enabled=false;}
    template<class Gate,class Status,class Arm>
    bool tick(int expectedGame,uint64_t now,Gate allowed,Status status,Arm arm)
    {
        std::lock_guard lock(mutex);
        if(stopped || !enabled)return false;
        ++counters.ticks;if(!counters.firstMs)counters.firstMs=now;counters.lastMs=now;
        if(!physics || overlay || paused || !allowed()){++counters.suspended;return false;}
        H3sbStatus s{};s.size=sizeof(s);
        if(status(&s)!=1 || s.size < offsetof(H3sbStatus,updated_ms)+sizeof(s.updated_ms)
            || s.game!=expectedGame || s.mode<1 || s.mode>3 || !s.have_body
            || !s.updated_ms || now<s.updated_ms || now-s.updated_ms>250 || !std::isfinite(s.speed)){++counters.suspended;return false;}
        if(s.armed && !s.asleep){++counters.alreadyArmed;return false;} // check only; no redundant wake/request
        ++counters.requests;if(std::abs(s.speed)<0.05f)++counters.idleRequests;
        bool accepted=arm()==1;if(accepted)++counters.accepted;return accepted;
    }
};
