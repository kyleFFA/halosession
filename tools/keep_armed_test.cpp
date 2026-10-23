#include "../HCMInternal/KeepArmedController.h"
#include <cassert>
#include <iostream>
#include <future>
#include <chrono>
using namespace std::chrono_literals;
int main(){
 // Native ABI regression: Reach reports 1, while ODST reports 2.
 for(auto game : {PhysicsHostGame::Halo3, PhysicsHostGame::Halo3ODST, PhysicsHostGame::HaloReach}) {
  KeepArmedController perGame; perGame.setEnabled(true); perGame.setPhysics(true);
  const int expected=physicsStatusGame(game); int requests=0;
  H3sbStatus observed{}; observed.size=sizeof(observed); observed.mode=3;
  observed.game=game==PhysicsHostGame::Halo3ODST ? 2 : 1; observed.have_body=1; observed.updated_ms=100;
  auto run=[&](bool allowed=true){return perGame.tick(expected,110,[&]{return allowed;},[&](H3sbStatus*s){*s=observed;return 1;},[&]{++requests;return 1;});};
  assert(run()); observed.armed=1; assert(!run()); observed.armed=0; assert(run());
  perGame.setOverlay(true); assert(!run()); perGame.setOverlay(false);
  perGame.setPaused(true); assert(!run()); perGame.setPaused(false);
  assert(!run(false)); observed.updated_ms=1;
  assert(!perGame.tick(expected,500,[]{return true;},[&](H3sbStatus*s){*s=observed;return 1;},[]{assert(false);return 1;}));
  observed.updated_ms=100; observed.game=3; assert(!run());
  assert(requests==2);
 }
 KeepArmedController c;int reads=0,calls=0;bool gameplay=true;int statusResult=1;
 H3sbStatus status{};status.size=sizeof(status);status.mode=3;status.game=1;status.have_body=1;status.updated_ms=100;
 auto tick=[&]{return c.tick(1,110,[&]{return gameplay;},[&](H3sbStatus* s){++reads;*s=status;return statusResult;},[&]{++calls;return 1;});};
 assert(!tick()&&reads==0);c.setEnabled(true);assert(!tick()&&reads==0);c.setPhysics(true);
 status.armed=1;assert(!tick()&&reads==1&&calls==0);
 status.armed=0;assert(tick()&&calls==1);status.armed=1;status.asleep=1;assert(tick()&&calls==2);
 status.asleep=0;assert(!tick()&&calls==2);
 status.armed=0;gameplay=false;assert(!tick());gameplay=true;
 c.setOverlay(true);assert(!tick());c.setOverlay(false);c.setPaused(true);assert(!tick());c.setPaused(false);
 status.have_body=0;assert(!tick());status.have_body=1;assert(tick()); // no cached old biped
 status.game=2;assert(!tick());status.game=1;status.mode=0;assert(!tick());status.mode=3;
 status.updated_ms=0;assert(!tick());status.updated_ms=111;assert(!tick());status.updated_ms=100;
 assert(!c.tick(1,500,[]{return true;},[&](H3sbStatus*s){*s=status;return 1;},[&]{++calls;return 1;}));
 statusResult=0;assert(!tick());statusResult=1;c.setEnabled(false);auto before=calls;assert(!tick()&&calls==before);
 c.setEnabled(true);assert(tick());c.stop();c.setEnabled(true);assert(!tick());
 // Disabling waits for an already executing request, then permits no new requests.
 KeepArmedController concurrent;concurrent.setEnabled(true);concurrent.setPhysics(true);
 std::promise<void> entered,release;auto releaseFuture=release.get_future();
 auto active=std::async(std::launch::async,[&]{return concurrent.tick(1,110,[]{return true;},[&](H3sbStatus*s){*s=status;return 1;},[&]{entered.set_value();releaseFuture.wait();return 1;});});
 entered.get_future().wait();auto off=std::async(std::launch::async,[&]{concurrent.setEnabled(false);});
 assert(off.wait_for(20ms)==std::future_status::timeout);release.set_value();assert(active.get());off.get();
 assert(!concurrent.tick(1,110,[]{return true;},[&](H3sbStatus*s){*s=status;return 1;},[]{assert(false);return 1;}));
 KeepArmedController idle;idle.setEnabled(true);idle.setPhysics(true);
 for(uint64_t i=0;i<18000;++i){auto now=1000+i*1000/60;idle.tick(1,now,[]{return true;},[&](H3sbStatus*s){*s=status;s->armed=0;s->asleep=1;s->speed=0;s->updated_ms=now;return 1;},[]{return 1;});}
 assert(idle.stats().requests==18000&&idle.stats().idleRequests==18000);
 idle.resetStats();assert(idle.stats().requests==0);
 for(uint64_t i=0;i<18000;++i){auto now=1000+i*1000/60;idle.tick(1,now,[]{return true;},[&](H3sbStatus*s){*s=status;s->armed=1;s->asleep=0;s->speed=2;s->updated_ms=now;return 1;},[]{assert(false);return 1;});}
 assert(idle.stats().requests==0&&idle.stats().alreadyArmed==18000);
 std::cout<<"Simulated 5 min/60Hz: idle-cleared every tick = 18000 requests; already-armed moving = 0 requests\n";
 std::cout<<"Keep armed: conditional requests, menu/pause/invalid-body gates, resume, off and teardown passed\n";
}
