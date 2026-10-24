#include "MainMenuController.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main(){
 using namespace menumovement;quickmenu::Selection selection;Controller controller;quickmenu::Identity identity{};identity[0]=101;uint64_t now=1;
 quickmenu::XboxState neutral{};WheelBinding binding{true,7,.28f,1,false,true};
 auto sample=[&](quickmenu::XboxState raw,bool full=true,bool eligible=true,bool connected=true,bool menu=true){return controller.sample(selection,raw,connected,menu,false,false,eligible,identity,1,now++,binding,full);};
 auto result=sample(neutral);assert(result.gameplay);
 quickmenu::XboxState raw{19,0x0200,41,202,13000,-14000,18000,-22000};result=sample(raw);assert(result.gameplay&&std::memcmp(&result.game,&raw,sizeof raw)==0);
 // Ordinary mouse-driven widgets cancel radial priority but do not disarm full controller gameplay.
 controller.cancelWheelOnly(false);binding.enabled=false;result=sample(raw);assert(result.gameplay&&std::memcmp(&result.game,&raw,sizeof raw)==0);binding.enabled=true;
 // Reserved activation remains exclusive, even when UI prevents wheel opening.
 binding.enabled=false;auto reserved=raw;reserved.buttons|=0x1000;result=sample(reserved);assert(!result.wheel&&result.gameplay&&result.game.buttons==raw.buttons);binding.enabled=true;sample(neutral);
 raw.buttons=0x1000;result=sample(raw);assert(result.wheel&&!result.gameplay&&!result.look&&!result.game.rx&&!result.game.ry&&!result.game.buttons&&!result.game.lt&&!result.game.rt&&result.game.lx==raw.lx);
 for(int i=0;i<20;++i){result=sample(raw);assert(result.wheel&&!result.game.rx&&!result.game.ry);assert(!selection.take(identity,now,true));}
 raw.buttons=0;result=sample(raw);assert(!result.wheel&&!result.look&&!result.gameplay);assert(selection.take(identity,now,true));assert(!selection.take(identity,now,true));result=sample(raw);assert(!result.gameplay&&!result.game.rx&&!result.game.ry&&!result.game.rt);
 sample(neutral);raw.buttons=0x0200;result=sample(raw);assert(result.gameplay&&std::memcmp(&result.game,&raw,sizeof raw)==0);
 // Off restricts all gameplay; enabling while controls are held cannot suddenly shoot/turn.
 result=sample(raw,false);assert(!result.gameplay&&!result.look&&!result.game.buttons&&!result.game.lt&&!result.game.rt&&!result.game.lx&&!result.game.rx);result=sample(raw,true);assert(!result.gameplay&&!result.move);sample(neutral);assert(sample(raw).gameplay);
 for(int cause=0;cause<6;++cause){sample(neutral);raw.buttons=0x1000;assert(sample(raw).wheel);
  if(cause==0)sample(raw,true,false);
  if(cause==1)sample(raw,true,true,false);
  if(cause==2){++identity[1];sample(raw);}
  if(cause==3){now+=201;selection.expire(now);sample(raw);}
  if(cause==4)sample(raw,true,true,true,false);
  if(cause==5){binding.enabled=false;sample(raw);binding.enabled=true;}
  raw.buttons=0;sample(raw);assert(!selection.take(identity,now,true));assert(!sample(raw).gameplay);sample(neutral);
 }
 // Trigger bindings are fully reserved rather than leaking partial trigger input.
 for(int button:{15,16}){quickmenu::Selection wheel;Controller policy;auto config=binding;config.button=button;policy.sample(wheel,neutral,true,true,false,false,true,identity,2,1,config,true);auto input=neutral;input.lt=input.rt=70;auto out=policy.sample(wheel,input,true,true,false,false,true,identity,2,2,config,true);assert(out.gameplay);assert(button==15?!out.game.lt&&!out.game.buttons:!out.game.rt&&!out.game.buttons);}
 // Existing Session menu-toggle controls remain functional without also reaching game input.
 {quickmenu::Selection wheel;Controller policy;WheelBinding disabled{false,0,.28f,1,false,false};auto mask=menuToggleMask(ImGuiKey_GamepadStart)|menuToggleMask(ImGuiKey_GamepadR2);policy.sample(wheel,neutral,true,true,false,false,true,identity,2,1,disabled,true,mask);auto input=neutral;input.buttons=0x0210;input.lt=100;input.rt=200;input.rx=15000;auto out=policy.sample(wheel,input,true,true,false,false,true,identity,2,2,disabled,true,mask);assert(out.gameplay&&out.game.buttons==0x0200&&out.game.lt==100&&!out.game.rt&&out.game.rx==15000);assert(input.buttons==0x0210&&input.rt==200);}
 // When radial is disabled/unbound, full mode passes every controller byte.
 {quickmenu::Selection wheel;Controller policy;WheelBinding disabled{false,0,.28f,1,false,false};policy.sample(wheel,neutral,true,true,false,false,true,identity,2,1,disabled,true);quickmenu::XboxState every{111,65535,255,254,-32768,32767,-32768,32767};auto out=policy.sample(wheel,every,true,true,false,false,true,identity,2,2,disabled,true);assert(out.gameplay&&std::memcmp(&out.game,&every,sizeof every)==0);}
 for(bool blocked:{false,true}){auto decision=decide(true,true,true,blocked,true,true,false,true);assert(decision.keyboardMouseBlocked&&!decision.controllerBlocked&&!decision.pause);}
 assert(blocksHotkey(true,true,false,true,false,true));assert(!blocksHotkey(true,true,false,false,false,true));
 assert(!pauseMain(true,true,true,true,false)&&!pauseMain(true,true,false,false,true));
 std::cout<<"PASS: full controller bytes on/off; ordinary mouse-UI continuity; reserved button/trigger; exclusive radial right stick; one-shot and neutral/release; UI/focus/disconnect/death-title/timeout/close cancellation; keyboard/mouse block and no-pause policy.\n";
}
