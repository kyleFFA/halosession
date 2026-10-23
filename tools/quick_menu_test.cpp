#include "../HCMInternal/QuickMenuCore.h"
#include <windows.h>
#include <Xinput.h>
#include <dinput.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
using namespace quickmenu;
int main() {
 static_assert(sizeof(XINPUT_STATE)==sizeof(XboxState));
 static_assert(offsetof(XINPUT_STATE,Gamepad.sThumbRX)==offsetof(XboxState,rx));
 static_assert(offsetof(XINPUT_STATE,Gamepad.sThumbRY)==offsetof(XboxState,ry));
 static_assert(sizeof(DIJOYSTATE2)==272 && offsetof(DIJOYSTATE2,rgbButtons)==48);
 static_assert(DIJOFS_Z==8 && DIJOFS_RX==12 && DIJOFS_RY==16 && DIJOFS_RZ==20);
 Identity id{1,2,3,4,5,6,7,8,9,10,11,12};Selection s;uint64_t now=1000;
 auto poll=[&](bool down,float x,float y,bool eligible=true,bool connected=true,uint64_t device=1) {
  ++now;return s.sample(device,connected,down,x,y,eligible,id,now,.28f,1.f);
 };
 // Starting connected while held, reconnecting held and returning focus held do not activate.
 assert(!poll(true,0,1));assert(!s.active());assert(!poll(false,0,0));
 assert(poll(true,0,1));assert(s.active() && s.slot==0);assert(!s.take(id,now,true));
 // The actual production XInput filter preserves every byte outside the right stick.
 XboxState before{72,0xFFFF,86,196,-18000,20000,30000,-26000},after=before;
 for(int button=1;button<=14;++button)assert(xboxDown(before,button));
 assert(!xboxDown(before,0) && !xboxDown(before,17));assert(!xboxDown(before,15) && xboxDown(before,16));
 std::array<uint8_t,32> nativeKeys{};nativeKeys[12]=0x80;assert(nativeDown(nativeKeys,0xFFFFFFFF,13));assert(!nativeDown(nativeKeys,0xFFFFFFFF,12));
 assert(nativeDown(nativeKeys,4500,33) && nativeDown(nativeKeys,4500,34));assert(!nativeDown(nativeKeys,4500,35));
 assert(nativeDown(nativeKeys,31500,33) && nativeDown(nativeKeys,31500,36));assert(!nativeDown(nativeKeys,0xFFFFFFFF,33));
 suppress(after);assert(after.rx==0 && after.ry==0);assert(std::memcmp(&before,&after,12)==0);
 // A fake native game consumer continues moving and advancing simulation during capture.
 int simulation=0,movement=0,camera=0;
 for(int i=0;i<120;++i) {auto game=before;if(poll(true,0,1))suppress(game);++simulation;movement+=game.ly;camera+=game.rx+game.ry;}
 assert(simulation==120 && movement==2400000 && camera==0);
 assert(!poll(false,0,1));assert(!s.active());auto pick=s.take(id,now,true);assert(pick && pick->slot==0);assert(!s.take(id,now,true));
 for(int i=0;i<20;++i)poll(false,0,1);assert(!s.take(id,now,true));
 after=before;if(poll(false,0,1))suppress(after);assert(std::memcmp(&before,&after,sizeof before)==0);
 // All five fixed sectors, center cancellation, angular hysteresis and neutral noise.
 constexpr float pi=3.14159265359f;
 for(int sector=0;sector<5;++sector){float a=sector*2*pi/5;assert(poll(true,std::sin(a),std::cos(a)));assert(s.slot==sector);poll(false,0,0);assert(s.take(id,now,true)->slot==sector);}
 poll(true,0,.31f);assert(s.slot==-1);poll(true,0,.5f);assert(s.slot==0);
 poll(true,std::sin(pi/5+.02f),std::cos(pi/5+.02f));assert(s.slot==0);
 poll(true,std::sin(pi/5+.12f),std::cos(pi/5+.12f));assert(s.slot==1);
 poll(true,.01f,.01f);assert(s.slot==-1);poll(false,0,0);assert(!s.take(id,now,true));
 // Disconnect, blocked UI/focus/death, identity/epoch changes and expiry consume no action.
 poll(true,1,0);poll(true,1,0,true,false);assert(!s.active());poll(false,0,0);assert(!s.take(id,now,true));
 poll(true,1,0);poll(true,1,0,false);assert(!s.active());assert(!poll(true,1,0));poll(false,0,0);assert(!s.take(id,now,true));
 poll(true,1,0);++id[11];assert(!poll(true,1,0));assert(!s.active());poll(false,0,0);assert(!s.take(id,now,true));
 poll(true,1,0);s.expire(now+201);assert(!s.active());poll(false,0,0);assert(!s.take(id,now,true));
 poll(true,1,0);poll(false,0,0);assert(!s.take(id,now+101,true));
 poll(true,1,0);poll(false,0,0);++id[3];assert(!s.take(id,now,true));
 poll(true,1,0);assert(!poll(true,0,1,true,true,2));assert(s.active());poll(false,0,0);assert(s.take(id,now,true));
 poll(true,std::numeric_limits<float>::quiet_NaN(),1);assert(s.slot==-1);poll(false,0,0);assert(!s.take(id,now,true));
 // Native PlayStation layouts can put right X/Y in Z/Rz or Rx/Ry. Only those bytes change.
 Axis unsignedAxis{0,65535},signedAxis{-1000,1000};assert(unsignedAxis.center()==32767 && signedAxis.center()==0);
 for(auto axes:{std::array<int,2>{2,5},std::array<int,2>{3,4}}) {
  DIJOYSTATE2 original{};auto bytes=reinterpret_cast<unsigned char*>(&original);for(size_t i=0;i<sizeof original;++i)bytes[i]=static_cast<unsigned char>(i*37);
  auto filtered=original;
  assert(suppressDirect(reinterpret_cast<int32_t*>(&filtered),axes[0],axes[1],unsignedAxis,signedAxis));
  auto b=reinterpret_cast<unsigned char*>(&filtered);
  for(size_t i=0;i<sizeof original;++i)if(i<size_t(axes[0])*4 || i>=size_t(axes[0]+1)*4)if(i<size_t(axes[1])*4 || i>=size_t(axes[1]+1)*4)assert(bytes[i]==b[i]);
  assert(reinterpret_cast<int32_t*>(&filtered)[axes[0]]==32767 && reinterpret_cast<int32_t*>(&filtered)[axes[1]]==0);
  auto unchanged=original;assert(!suppressDirect(reinterpret_cast<int32_t*>(&unchanged),0,5,unsignedAxis,signedAxis));
  assert(!suppressDirect(reinterpret_cast<int32_t*>(&unchanged),3,3,unsignedAxis,signedAxis));
  assert(!suppressDirect(reinterpret_cast<int32_t*>(&unchanged),3,4,{},signedAxis));assert(std::memcmp(&original,&unchanged,sizeof original)==0);
 }
 std::cout<<"PASS: SDK layouts; Xbox/XInput and native PlayStation axis filters preserve all other bytes; moving simulation fixture; five sectors; one-shot release; dead zone/hysteresis; disconnect/death/UI/focus/session/expiry cancellation; no stale/repeated action\n";
}
