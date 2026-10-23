#pragma once
#include "H2MovementCore.h"
namespace quickmenu {
template<class Reader>bool soloMenuPause(Reader& r,uintptr_t base,bool originalH2,bool ingame) {
 if(!originalH2 || !ingame)return false;
 try {
  h2movement::verifyBuild(r,base);
  auto g=h2movement::pointer(r,base+0xE80A78),players=h2movement::pointer(r,base+0xE80A20);
  return !r.template read<uint8_t>(g) && r.template read<uint8_t>(g+1)==1 && r.template read<int32_t>(g+8)==1
   && r.template read<uint8_t>(g+0xC)==1 && r.template read<uint8_t>(g+0x2C8)==0 && r.template read<uint8_t>(g+0x1808)==1
   && r.template read<int16_t>(players)==1 && r.template read<int16_t>(players+8)==1 && r.template read<int16_t>(players+10)==1;
 }catch(...){return false;}
}
}
