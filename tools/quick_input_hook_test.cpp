#include "pch.h"
#include "QuickMenuHook.h"
#include "QuickMenuCore.h"
#include "MenuMovementPolicy.h"
#include "MainMenuController.h"
#include <cassert>
#include <dinput.h>
extern "C" unsigned fixturePoll(void*);
extern "C" void fixtureSite();
static std::atomic<bool> capture=false,hold=false,entered=false,proceed=false;
static bool direct=false,menuMovement=false,menuCamera=false,menuFull=false;static uint32_t menuUiMask=0;
static quickmenu::Selection menuSelection;static menumovement::Controller menuController;
static void filter(safetyhook::Context& context) {
 if(hold){entered=true;entered.notify_all();while(!proceed.load())proceed.wait(false);}
 if(!capture)return;
 if(menuMovement){auto input=reinterpret_cast<quickmenu::XboxState*>(context.rbx);*input=menuController.sample(menuSelection,*input,true,true,true,menuCamera,true,{},1,GetTickCount64(),{true,7,.28f,1,false},menuFull,menuUiMask).game;return;}
 if(direct)assert(quickmenu::suppressDirect(reinterpret_cast<int32_t*>(context.rbx),2,5,{0,65535},{0,65535}));
 else quickmenu::suppress(*reinterpret_cast<quickmenu::XboxState*>(context.rbx));
}
int main() {
 hook_lifetime::initialize();quickmenu::NativeHook hook;hook.install(reinterpret_cast<void*>(fixtureSite),filter);assert(bool(hook));
 quickmenu::XboxState original{91,0xFFFF,13,79,23000,17000,-32000,28000};
 capture=true;int movement=0,ticks=0;
 for(int i=0;i<200;++i){auto game=original;assert(fixturePoll(&game)==0);++ticks;movement+=game.ly;assert(game.rx==0 && game.ry==0 && std::memcmp(&game,&original,12)==0);}
 assert(ticks==200 && movement==3400000);
 capture=false;auto game=original;fixturePoll(&game);assert(std::memcmp(&game,&original,sizeof game)==0);
 menuMovement=true;capture=true;game={};fixturePoll(&game);game=original;game.buttons=0x0200;assert(fixturePoll(&game)==0);assert(game.lx==original.lx&&game.ly==original.ly&&!game.buttons&&!game.lt&&!game.rt&&!game.rx&&!game.ry);
 menuCamera=true;game={};fixturePoll(&game);game=original;game.buttons=0x0200;fixturePoll(&game);assert(game.rx==original.rx&&game.ry==original.ry&&!game.buttons&&!game.lt&&!game.rt);game=original;game.buttons=0x1000;fixturePoll(&game);assert(!game.rx&&!game.ry&&menuSelection.active());game=original;game.buttons=0;fixturePoll(&game);assert(!game.rx&&!game.ry&&!menuSelection.active());menuFull=true;game={};fixturePoll(&game);game=original;game.buttons=0x0200;auto fullExpected=game;fixturePoll(&game);assert(std::memcmp(&game,&fullExpected,sizeof game)==0);menuUiMask=menumovement::menuToggleMask(ImGuiKey_GamepadStart)|menumovement::menuToggleMask(ImGuiKey_GamepadR2);game=fullExpected;game.buttons|=0x10;fixturePoll(&game);assert(game.buttons==fullExpected.buttons&&!game.rt&&game.rx==fullExpected.rx);menuUiMask=0;game=fullExpected;game.buttons=0x1000;fixturePoll(&game);assert(menuSelection.active()&&!game.rx&&!game.ry&&!game.buttons&&!game.lt&&!game.rt);menuFull=false;menuMovement=false;capture=false;
 direct=true;DIJOYSTATE2 native{};native.lX=20000;native.lY=40000;native.lZ=65000;native.lRz=6000;native.rgbButtons[8]=0x80;native.rgdwPOV[0]=27000;
 auto saved=native;capture=true;fixturePoll(&native);assert(native.lZ==32767 && native.lRz==32767 && native.lX==20000 && native.lY==40000);
 assert(native.rgbButtons[8]==0x80 && native.rgdwPOV[0]==27000);capture=false;native=saved;fixturePoll(&native);assert(std::memcmp(&native,&saved,sizeof native)==0);
 // Disable entry while a callback still has a machine-code return into the stub.
 hold=true;std::thread polling([&]{auto state=saved;fixturePoll(&state);});
 while(!entered.load())entered.wait(false);
 assert(!hook.retire());assert(bool(hook));proceed=true;proceed.notify_all();polling.join();
 assert(hook.retire());assert(!bool(hook));
 capture=true;game=original;fixturePoll(&game);assert(std::memcmp(&game,&original,sizeof game)==0);
 std::cout<<"PASS: production native-hook owner on an assembly poll fixture; RBX/XInput and native PlayStation buffers; preserved left movement and advancing consumer; immediate restoration; in-flight callback retirement retains stub until quiescent\n";
}
