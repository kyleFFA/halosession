#include "MainMenuController.h"
#include "MenuSpotPolicy.h"
#include <cassert>
#include <iostream>
#include <set>
int main(){
 using namespace quickmenu;using namespace menumovement;
 const uint64_t now=1000;BoundSpotContext gate{true,false,false,true,true,true,true,false,false,false,true,false,true,0,now};
 assert(allowBoundSpot(gate,now));
 for(int i=0;i<15;++i){auto bad=gate;switch(i){case 0:bad.open=false;break;case 1:bad.conflict=true;break;case 2:bad.wheelConflict=true;break;case 3:bad.hookReady=false;break;case 4:bad.xinput=false;break;case 5:bad.frameReady=false;break;case 6:bad.valid=false;break;case 7:bad.hotkeysDisabled=true;break;case 8:bad.wheelActive=true;break;case 9:bad.capturing=true;break;case 10:bad.controllerLive=false;break;case 11:bad.busy=true;break;case 12:bad.available=false;break;case 13:bad.wheelUntil=now+1;break;case 14:bad.controllerTime=now-101;break;}assert(!allowBoundSpot(bad,now));}
 auto future=gate;future.controllerTime=now+1;assert(!allowBoundSpot(future,now));
 for(unsigned game:{1u,2u,5u,6u})for(int action:{1,2}){
  Identity id{};id[0]=game;id[11]=42;Pick pick{0,id,111,now};DispatchContext dispatch{true,true,true,true,false,true,false,id,23,111,now};
  assert(allowDispatch(pick,23,true,dispatch,now));int saves=0,backs=0;
  assert(coordinateAction(action,[&]{++saves;},[&]{++backs;}));assert(saves==(action==1)&&backs==(action==2));
  for(int i=0;i<12;++i){auto bad=dispatch;switch(i){case 0:bad.valid=false;break;case 1:bad.frameReady=false;break;case 2:bad.available=false;break;case 3:bad.mainOpen=false;break;case 4:bad.mainConflict=true;break;case 5:bad.controllerLive=false;break;case 6:bad.bindingConflict=true;break;case 7:++bad.identity[11];break;case 8:++bad.identity[3];break;case 9:++bad.revision;break;case 10:++bad.device;break;case 11:bad.controllerTime=now-101;break;}assert(!allowDispatch(pick,23,true,bad,now));}
  assert(!allowDispatch(pick,23,true,dispatch,now+101));assert(!allowDispatch(pick,23,true,dispatch,now-1));
 }
 int unexpected=0;assert(!coordinateAction(3,[&]{++unexpected;},[&]{++unexpected;}));assert(!unexpected);
 // All supported gamepad buttons/triggers/axis directions are binding-agnostic.
 int tested=0;for(int key=ImGuiKey_GamepadStart;key<=ImGuiKey_GamepadRStickDown;++key){
  auto k=ImGuiKey(key);const auto mask=menuToggleMask(k);assert(mask);++tested;
  std::set<ImGuiKey> chord{k};bool down=false,fresh=false;int count=0;
  auto pressed=[&]{return bindingNewPress(chord,[&](auto){return down;},[&](auto){return fresh;});};
  assert(!pressed());down=fresh=true;if(pressed()&&menuSpotBinding(true,true,true,false,false,false,false,false,false))++count;
  fresh=false;for(int tick=0;tick<50;++tick)if(pressed())++count;assert(count==1);
  assert(!menuSpotBinding(true,true,true,false,false,false,false,true,false)); // radial/release owns it
  assert(!menuSpotBinding(true,true,true,false,false,false,false,false,true)); // menu-toggle priority
  assert(!menuSpotBinding(true,true,true,false,false,true,false,false,false)); // popup
  assert(!menuSpotBinding(true,true,true,false,true,false,false,false,false)); // text
  assert(!menuSpotBinding(true,true,true,false,false,false,true,false,false)); // active widget
  assert(!menuSpotBinding(true,false,true,false,false,false,false,false,false)); // other mods unchanged
  assert(!menuSpotBinding(true,true,true,true,false,false,false,false,false)); // mouse gameplay blocked
  XboxState raw{1,65535,255,255,32767,-32768,32767,-32768},filtered=raw;consumeMenuToggle(filtered,mask);
  if(mask&65535)assert(!(filtered.buttons&mask));if(mask&(1u<<16))assert(!filtered.lt);if(mask&(1u<<17))assert(!filtered.rt);if(mask&(1u<<18))assert(!filtered.lx);if(mask&(1u<<19))assert(!filtered.ly);if(mask&(1u<<20))assert(!filtered.rx);if(mask&(1u<<21))assert(!filtered.ry);
 }
 // Real menu controller: radial wins over a bound action, right stick never looks,
 // and a radial pick dispatches once through the SAME coordinate event router.
 for(int action:{1,2}){Selection wheel;Controller controller;Identity id{};id[0]=6;WheelBinding binding{true,3,.28f,1,false,true};XboxState raw{};
  controller.sample(wheel,raw,true,true,false,false,true,id,8,1,binding,true);
  raw.buttons=0x100;raw.ry=30000;auto held=controller.sample(wheel,raw,true,true,false,false,true,id,8,2,binding,true);assert(held.wheel&&!held.game.rx&&!held.game.ry&&!held.game.buttons);
  raw.buttons=0;auto released=controller.sample(wheel,raw,true,true,false,false,true,id,8,3,binding,true);assert(!released.wheel&&!released.look);auto pick=wheel.take(id,3,true);assert(pick&&!wheel.take(id,3,true));int count=0;coordinateAction(action,[&]{++count;},[&]{++count;});assert(count==1);
 }
 std::cout<<"PASS: "<<tested<<" controller binding inputs; one fresh press/held suppression; all four title routes use the same coordinate events; guarded queue/dispatch cancellation; UI/radial/toggle priority; reserved gameplay inputs; radial one-shot/right-stick exclusivity.\n";
}
