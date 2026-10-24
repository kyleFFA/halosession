#pragma once
#include "MenuMovementPolicy.h"
namespace menumovement {
struct WheelBinding {bool enabled;int button;float deadzone,sensitivity;bool invertY;bool reserveActivation=true;};
struct MenuSample {quickmenu::XboxState game;bool wheel=false,capture=false,move=false,look=false,gameplay=false;};
inline void consumeActivation(quickmenu::XboxState& state,int binding){
 constexpr uint16_t masks[]={0,0x0010,0x0020,0x0100,0x0200,0x0040,0x0080,0x1000,0x2000,0x4000,0x8000,1,2,4,8};
 if(binding==15)state.lt=0;else if(binding==16)state.rt=0;else if(binding>0&&binding<15)state.buttons&=uint16_t(~masks[binding]);
}
// Shares the existing selection state with the normal radial adapter. No game calls.
class Controller {
 Gate movement;
 bool cameraNeedsNeutral=true,wasWheel=false,cameraWasEnabled=false,fullWasEnabled=false,actionsNeedRelease=true;
 uint64_t owner=0;quickmenu::Identity identity{};
public:
 MenuSample sample(quickmenu::Selection& selection,const quickmenu::XboxState& input,bool connected,bool menu,bool movementEnabled,bool cameraEnabled,bool eligible,const quickmenu::Identity& current,uint64_t device,uint64_t now,const WheelBinding& binding,bool full=false,uint32_t uiMask=0){
  MenuSample out{input};
  const bool rightNeutral=std::abs(int(input.rx))<5898&&std::abs(int(input.ry))<5898;
  const bool centered=std::abs(int(input.lx))<5898&&std::abs(int(input.ly))<5898;
  const bool released=!input.buttons&&input.lt<16&&input.rt<16&&rightNeutral;
  if(owner!=device||identity!=current){selection.cancel();movement.disarm();cameraNeedsNeutral=true;wasWheel=false;owner=device;identity=current;}
  if(!menu){selection.cancel();movement.sample(false,false,device,current,centered,released);cameraNeedsNeutral=true;wasWheel=false;suppress(out.game,false,false);return out;}
  if(cameraEnabled!=cameraWasEnabled)cameraNeedsNeutral=true;cameraWasEnabled=cameraEnabled;
  if(full!=fullWasEnabled){selection.cancel();movement.disarm();actionsNeedRelease=true;cameraNeedsNeutral=true;}fullWasEnabled=full;
  const bool valid=connected&&eligible;
  out.move=movement.sample(true,valid&&(movementEnabled||full),device,current,centered,released);
  float x=float(input.rx)/32768.f,y=float(input.ry)/32768.f;if(binding.invertY)y=-y;
  out.capture=selection.sample(device,connected,quickmenu::xboxDown(input,binding.button),x,y,valid&&binding.enabled&&binding.button>0,current,now,binding.deadzone,binding.sensitivity);
  out.wheel=selection.active();
  if(!valid||out.capture||out.wheel||wasWheel){cameraNeedsNeutral=true;actionsNeedRelease=true;}
  if(valid&&!out.capture&&!out.wheel&&rightNeutral)cameraNeedsNeutral=false;
  if(valid&&!out.capture&&!out.wheel&&released)actionsNeedRelease=false;
  out.look=(cameraEnabled||full)&&out.move&&!out.capture&&!out.wheel&&!cameraNeedsNeutral;
  out.gameplay=full&&out.move&&valid&&!out.capture&&!out.wheel&&!actionsNeedRelease;
  if(!out.gameplay)suppress(out.game,out.move,out.look);
  else{if(!out.look)out.game.rx=out.game.ry=0;if(binding.reserveActivation)consumeActivation(out.game,binding.button);consumeMenuToggle(out.game,uiMask);}
  wasWheel=out.wheel;return out;
 }
 bool releaseGuard()const{return movement.releaseGuard();}
 void cancel(){movement.disarm();cameraNeedsNeutral=true;actionsNeedRelease=true;wasWheel=false;}
 void cancelWheelOnly(bool active){if(active){cameraNeedsNeutral=true;actionsNeedRelease=true;}wasWheel=false;}
};
inline bool pauseMain(bool setting,bool open,bool solo,bool movement,bool wheel){return setting&&open&&solo&&!movement&&!wheel;}
struct Presentation {bool showMain,inputs;};
class PresentationState {
 bool lastWheel=false,release=false;
public:
 Presentation sample(bool open,bool wheel,bool inputs,bool mouseHeld){
  if(lastWheel&&!wheel&&mouseHeld)release=true;if(!mouseHeld)release=false;if(!open)release=false;
  lastWheel=wheel;return {open&&!wheel,inputs&&!release};
 }
};
}
