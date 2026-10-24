#pragma once
#include "QuickMenuCore.h"
#include "imgui.h"
namespace menumovement {
inline uint32_t menuToggleMask(ImGuiKey key){
 switch(key){
 case ImGuiKey_GamepadStart:return 0x10;case ImGuiKey_GamepadBack:return 0x20;
 case ImGuiKey_GamepadL1:return 0x100;case ImGuiKey_GamepadR1:return 0x200;
 case ImGuiKey_GamepadL3:return 0x40;case ImGuiKey_GamepadR3:return 0x80;
 case ImGuiKey_GamepadFaceDown:return 0x1000;case ImGuiKey_GamepadFaceRight:return 0x2000;case ImGuiKey_GamepadFaceLeft:return 0x4000;case ImGuiKey_GamepadFaceUp:return 0x8000;
 case ImGuiKey_GamepadDpadUp:return 1;case ImGuiKey_GamepadDpadDown:return 2;case ImGuiKey_GamepadDpadLeft:return 4;case ImGuiKey_GamepadDpadRight:return 8;
 case ImGuiKey_GamepadL2:return 1u<<16;case ImGuiKey_GamepadR2:return 1u<<17;
 case ImGuiKey_GamepadLStickLeft:case ImGuiKey_GamepadLStickRight:return 1u<<18;
 case ImGuiKey_GamepadLStickUp:case ImGuiKey_GamepadLStickDown:return 1u<<19;
 case ImGuiKey_GamepadRStickLeft:case ImGuiKey_GamepadRStickRight:return 1u<<20;
 case ImGuiKey_GamepadRStickUp:case ImGuiKey_GamepadRStickDown:return 1u<<21;
 default:return 0;
 }
}
inline void consumeMenuToggle(quickmenu::XboxState& state,uint32_t mask){
 state.buttons&=uint16_t(~mask);if(mask&(1u<<16))state.lt=0;if(mask&(1u<<17))state.rt=0;
 if(mask&(1u<<18))state.lx=0;if(mask&(1u<<19))state.ly=0;if(mask&(1u<<20))state.rx=0;if(mask&(1u<<21))state.ry=0;
}
inline bool retainReleaseGuard(bool guard,bool open,bool held){return !open&&guard&&held;}
inline bool blocksHotkey(bool open,bool toggle,bool text,bool gamepad,bool mouse,bool wheelOwns=false){return (gamepad&&wheelOwns)||(open&&!toggle&&(text||gamepad||mouse));}
// Only coordinate-bookmark presses may enter the guarded menu action queue.
inline bool menuSpotBinding(bool open,bool spot,bool gamepad,bool mouse,bool text,bool popup,bool active,bool wheelOwns,bool toggleOverlap){
 return open&&spot&&gamepad&&!mouse&&!text&&!popup&&!active&&!wheelOwns&&!toggleOverlap;
}
struct Decision {bool keyboardMouseBlocked,controllerBlocked,pause;};
inline Decision decide(bool open,bool option,bool ready,bool blockSetting,bool pauseSetting,bool verifiedSolo,bool conflict,bool wheelReady=false){
 bool pass=open&&(option||wheelReady)&&ready&&!conflict;
 return {open&&(blockSetting||option||wheelReady),open&&((option||wheelReady)?!pass:blockSetting),open&&pauseSetting&&verifiedSolo&&!option};
}
inline void suppress(quickmenu::XboxState& state,bool move,bool look=false){state.buttons=0;state.lt=state.rt=0;if(!look)state.rx=state.ry=0;if(!move)state.lx=state.ly=0;}
// Native record offsets follow MCC's already-verified DIJOYSTATE2 buffer.
inline bool suppressNative(int32_t* axes,const std::array<quickmenu::Axis,8>& ranges,bool move){
 for(auto r:ranges)if(!r.valid())return false;
 for(int i=move?2:0;i<8;++i)axes[i]=ranges[i].center();
 auto bytes=reinterpret_cast<uint8_t*>(axes);for(int i=0;i<4;++i)reinterpret_cast<uint32_t*>(bytes+32)[i]=0xffffffff;
 for(int i=0;i<128;++i)bytes[48+i]=0;
 // Velocity/acceleration/force axes are not movement controls; leave their verified record tail unchanged.
 return true;
}
class Gate {
 uint64_t device=0;quickmenu::Identity identity{};bool neutral=false,held=false;
public:
 bool sample(bool menu,bool eligible,uint64_t owner,const quickmenu::Identity& current,bool centered,bool actionsReleased){
  if(!menu){if(held&&actionsReleased){held=false;device=0;neutral=false;}return false;}
  held=true;if(device!=owner||identity!=current||!eligible){device=owner;identity=current;neutral=false;}
  if(eligible&&centered&&actionsReleased)neutral=true;return eligible&&neutral;
 }
 bool releaseGuard()const{return held;}
 void disarm(){neutral=false;}
 void reset(){device=0;neutral=false;held=false;}
};
}
