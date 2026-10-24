#include "MainMenuController.h"
#include <cassert>
#include <iostream>
using namespace menumovement;
int main(){
 const WheelBinding binding{true,7,.28f,1,false}; // A/Cross, consumed by the main-menu filter.
 quickmenu::Identity identity{};identity[0]=42;quickmenu::Selection wheel;Controller controller;
 quickmenu::XboxState neutral{7,0,0,0,0,0,0,0};uint64_t now=100;
 auto sample=[&](quickmenu::XboxState raw,bool move=true,bool look=true,bool valid=true,bool connected=true,bool menu=true){return controller.sample(wheel,raw,connected,menu,move,look,valid,identity,19,now++,binding);};
 auto result=sample(neutral);assert(result.move&&result.look&&!result.wheel);
 auto raw=neutral;raw.lx=21000;raw.ly=-12000;raw.rx=19000;raw.ry=-23000;raw.buttons=0x0200;raw.lt=70;raw.rt=240;
 result=sample(raw);assert(result.move&&result.look&&result.game.lx==raw.lx&&result.game.ly==raw.ly&&result.game.rx==raw.rx&&result.game.ry==raw.ry);assert(!result.game.buttons&&!result.game.lt&&!result.game.rt&&result.game.packet==raw.packet);
 result=sample(raw,true,false);assert(result.move&&!result.look&&!result.game.rx&&!result.game.ry);
 // No camera or walking when the movement option is off, even if camera is checked.
 result=sample(neutral,false,true);assert(!result.move&&!result.look);
 sample(neutral);raw=neutral;raw.lx=22000;raw.ry=30000;raw.buttons=0x1000;
 result=sample(raw);assert(result.wheel&&result.capture&&!result.look&&!result.game.rx&&!result.game.ry&&result.game.lx==raw.lx&&!result.game.buttons);
 for(int i=0;i<12;++i){result=sample(raw);assert(result.wheel&&!result.look&&!result.game.rx&&!result.game.ry);assert(!wheel.take(identity,now,true));}
 // Release executes once while still deflected, but that stick cannot jump into camera look.
 raw.buttons=0;result=sample(raw);assert(!result.wheel&&!result.look&&!result.game.ry);auto pick=wheel.take(identity,now,true);assert(pick&&pick->slot==0);assert(!wheel.take(identity,now,true));
 result=sample(raw);assert(!result.look&&!result.game.ry);sample(neutral);result=sample(raw);assert(result.look&&result.game.ry==raw.ry);
 // Radial still works over the main menu with gameplay movement and camera both disabled.
 sample(neutral,false,false);raw.buttons=0x1000;result=sample(raw,false,false);assert(result.wheel&&!result.move&&!result.look);raw.buttons=0;sample(raw,false,false);assert(wheel.take(identity,now,true));sample(neutral,false,false);
 // Conflicting UI/focus, disconnect, changed title/player, timeout and close cancel without actions.
 for(int cause=0;cause<5;++cause){sample(neutral);raw.buttons=0x1000;result=sample(raw);assert(result.wheel);
  if(cause==0)sample(raw,true,true,false);
  if(cause==1)sample(raw,true,true,true,false);
  if(cause==2){++identity[1];sample(raw);}
  if(cause==3){now+=201;wheel.expire(now);sample(raw);}
  if(cause==4)sample(raw,true,true,true,true,false);
  raw.buttons=0;sample(raw);assert(!wheel.take(identity,now,true));assert(!sample(raw).look);sample(neutral);
 }
 // Every valid config combination preserves exclusive wheel ownership and strips actions.
 for(bool move:{false,true})for(bool look:{false,true})for(bool valid:{false,true})for(bool connected:{false,true}){
  quickmenu::Selection selection;Controller policy;uint64_t tick=1;
  for(int i=0;i<30;++i){auto input=neutral;if(i>=2&&i<20){input.buttons=0x1000;input.rx=27000;input.ry=15000;input.lx=10000;}
   auto output=policy.sample(selection,input,connected,true,move,look,valid,identity,3,tick++,binding);
   assert(!output.game.buttons&&!output.game.lt&&!output.game.rt);
   if(output.wheel||output.capture)assert(!output.look&&!output.game.rx&&!output.game.ry);
   if(!move||!look||!valid||!connected)assert(!output.look);
  }
 }
 // Menu visibility, prior input flag and close intent are never overwritten by wheel presentation.
 PresentationState presentation;assert(presentation.sample(true,false,true,false).showMain);assert(!presentation.sample(true,true,true,false).showMain);assert(!presentation.sample(true,false,true,true).inputs);assert(presentation.sample(true,false,true,false).inputs);assert(!presentation.sample(false,false,true,false).showMain);assert(!presentation.sample(true,false,false,false).inputs);
 for(bool movement:{false,true})for(bool active:{false,true})for(bool solo:{false,true})assert(pauseMain(true,true,solo,movement,active)==(solo&&!movement&&!active));
 auto route=decide(true,false,true,true,false,true,false,true);assert(!route.controllerBlocked&&route.keyboardMouseBlocked);route=decide(true,false,true,true,false,true,true,true);assert(route.controllerBlocked);
 assert(blocksHotkey(true,true,false,true,false,true));assert(!blocksHotkey(true,true,false,false,false,true));
 std::cout<<"PASS: optional standard-menu camera; exclusive radial stick ownership; one-shot release/recenter; wheel with movement off; conflict/disconnect/title/timeout/close cancellation; presentation restoration; pause and hotkey priorities.\n";
}
