#pragma once
#include "QuickMenuCore.h"
namespace quickmenu {
template<class Chord,class Down,class Pressed>bool bindingNewPress(const Chord& chord,Down&& down,Pressed&& pressed){
 if(chord.empty())return false;bool fresh=false;for(auto key:chord){if(!down(key))return false;fresh|=pressed(key);}return fresh;
}
struct BoundSpotContext {
 bool open,conflict,wheelConflict,hookReady,xinput,frameReady,valid,hotkeysDisabled,wheelActive,capturing,controllerLive,busy,available;
 uint64_t wheelUntil,controllerTime;
};
inline bool allowBoundSpot(const BoundSpotContext& s,uint64_t now){
 return s.open&&!s.conflict&&!s.wheelConflict&&s.hookReady&&s.xinput&&s.frameReady&&s.valid&&!s.hotkeysDisabled&&!s.wheelActive&&!s.capturing&&now>=s.wheelUntil&&s.controllerLive&&now>=s.controllerTime&&now-s.controllerTime<=100&&!s.busy&&s.available;
}
struct DispatchContext {
 bool valid,frameReady,available,mainOpen,mainConflict,controllerLive,bindingConflict;
 Identity identity;uint64_t revision,device,controllerTime;
};
inline bool allowDispatch(const Pick& pick,uint64_t revision,bool fromMain,const DispatchContext& s,uint64_t now){
 return s.valid&&s.identity==pick.identity&&s.frameReady&&now>=pick.time&&now-pick.time<=100&&s.available&&(!fromMain||(s.mainOpen&&!s.mainConflict))&&s.revision==revision&&s.controllerLive&&s.device==pick.device&&now>=s.controllerTime&&now-s.controllerTime<=100&&!s.bindingConflict;
}
template<class Save,class Back>bool coordinateAction(int action,Save&& save,Back&& back){
 if(action==1){save();return true;}if(action==2){back();return true;}return false;
}
}
