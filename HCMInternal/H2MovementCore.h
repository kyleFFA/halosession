#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>

// Original H2, Steam MCC 1.3528 only. Evidence: H2-MOVEMENT-AUDIT.txt.
// Reader throws on any failed read. No renderer fallback, game calls, or cached body pointers.
namespace h2movement {
inline constexpr uint32_t none=0xFFFFFFFF;
struct Vec3 { float x{},y{},z{}; bool operator==(const Vec3&) const=default; };
struct Angles { float yaw{},pitch{}; bool operator==(const Angles&) const=default; };
inline Vec3 operator+(Vec3 a,Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 operator-(Vec3 a,Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline float length2(Vec3 v) { return v.x*v.x+v.y*v.y+v.z*v.z; }
inline bool finite(Vec3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
inline bool valid(Angles a) { return std::isfinite(a.yaw)&&std::isfinite(a.pitch)&&std::fabs(a.pitch)<=1.571f&&std::fabs(a.yaw)<=100.f; }
inline void require(bool b,const char* why) { if(!b) throw std::runtime_error(why); }
template<class R> void verifyBuild(R& r,uintptr_t base) {
 require(base&&r.template read<uint16_t>(base)==0x5A4D,"Original H2 module is unavailable");
 auto nt=base+r.template read<uint32_t>(base+0x3C);
 require(r.template read<uint32_t>(nt)==0x4550&&r.template read<uint16_t>(nt+4)==0x8664&&r.template read<uint32_t>(nt+8)==0x68A0F0F2&&r.template read<uint32_t>(nt+0x50)==0x2A38000,"H2 movement requires the verified 1.3528 image");
 // Native user mapping, server predicate and facing reader, outside hook sites.
 require(r.template read<std::array<uint8_t,12>>(base+0x69D040)==std::array<uint8_t,12>{0x83,0xF9,0xFF,0x74,0x0F,0x48,0x8B,0x05,0xD4,0x39,0x7E,0x00}&&
  r.template read<std::array<uint8_t,13>>(base+0x6A6420)==std::array<uint8_t,13>{0x48,0x8B,0x05,0x51,0xA6,0x7D,0x00,0xB2,0x01,0x0F,0xB6,0x48,0x0C}&&
  r.template read<std::array<uint8_t,11>>(base+0x6C0E10)==std::array<uint8_t,11>{0x4C,0x69,0xC3,0xB8,0x00,0x00,0x00,0x48,0x83,0xC2,0x38},"H2 movement verification bytes differ; no action applied");
}
inline Vec3 forward(Angles a) { require(valid(a),"H2 view angles are unavailable");return {std::cos(a.pitch)*std::cos(a.yaw),std::cos(a.pitch)*std::sin(a.yaw),std::sin(a.pitch)}; }
template<class R> uintptr_t pointer(R& r,uintptr_t at) {
 auto p=r.template read<uintptr_t>(at);require(p>=0x10000&&p<0x00007FFFFFFEFFFFULL,"H2 pointer unavailable");return p;
}
template<class R> uintptr_t datum(R& r,uintptr_t h,uint32_t d,int stride,int capacity) {
 require(d!=none&&(d>>16)!=0&&(d>>16)!=0xFFFF,"H2 datum unavailable");
 auto size=r.template read<int32_t>(h+0x24), count=r.template read<int32_t>(h+0x3C);
 auto offset=r.template read<int64_t>(h+0x48);
 require(size==stride&&count>0&&count<=capacity&&(d&0xFFFF)<uint32_t(count)&&offset>0&&offset<0x10000000,"H2 data-array bounds unavailable");
 auto p=h+uintptr_t(offset)+uintptr_t(d&0xFFFF)*stride;
 require(r.template read<uint16_t>(p)==(d>>16),"H2 stale datum salt");return p;
}
struct Session {
 uintptr_t module{},globals{},playersGlobals{},players{},objects{},pool{},components{},control{},bsp{},timer{};
 uint8_t simulation{};
 uint64_t sessionNonce{};
 bool operator==(const Session&) const=default;
};
template<class R> Session session(R& r,uintptr_t base) {
 Session s; s.module=base;
 s.globals=pointer(r,base+0xE80A78);
 require(!r.template read<uint8_t>(s.globals)&&r.template read<uint8_t>(s.globals+1)==1&&r.template read<int16_t>(s.globals+2)>=0,"H2 map is loading or unavailable");
 auto mode=r.template read<int32_t>(s.globals+8);
 require(mode==1||mode==2,"H2 gameplay is required");
 s.simulation=r.template read<uint8_t>(s.globals+0xC);
 s.sessionNonce=r.template read<uint64_t>(s.globals+0x18);
 require(s.simulation==1||s.simulation==3||s.simulation==5,"H2 movement requires local/server authority; clients and unknown modes are blocked");
 require(r.template read<uint8_t>(s.globals+0x1808)==1,"H2 game is not in progress");
 s.playersGlobals=pointer(r,base+0xE80A20);s.players=pointer(r,base+0xE80A28);
 s.objects=pointer(r,base+0x18B7398);s.pool=pointer(r,base+0x18B7360);
 s.components=pointer(r,base+0x1648940);s.control=pointer(r,base+0x15E40D0);
 s.bsp=pointer(r,base+0xE6F770);s.timer=pointer(r,base+0x15FE008);
 return s;
}
struct Body {
 Session context;
 uintptr_t object{},motion{},view{};
 uint32_t player{},unit{},havok{};
 bool operator==(const Body&) const=default;
};
template<class R> Body body(R& r,const Session& s) {
 Body b; b.context=s;
 require(r.template read<int16_t>(s.playersGlobals+8)==1&&r.template read<int16_t>(s.playersGlobals+10)==1,"H2 movement requires exactly one local input user");
 require(!r.template read<uint8_t>(s.playersGlobals+6)&&!r.template read<uint8_t>(s.playersGlobals+7),"H2 player input is disabled");
 b.player=r.template read<uint32_t>(s.playersGlobals+0xC);
 require(b.player==r.template read<uint32_t>(s.playersGlobals+0x1C),"H2 user/controller mappings disagree");
 for(int i=1;i<4;++i)require(r.template read<uint32_t>(s.playersGlobals+0xC+4*i)==none&&r.template read<uint32_t>(s.playersGlobals+0x1C+4*i)==none,"H2 local user mapping is ambiguous");
 auto player=datum(r,s.players,b.player,0x224,16);
 require(r.template read<int16_t>(player+0x28)==0&&r.template read<int32_t>(player+0x24)==0,"H2 player does not round-trip to user/controller zero");
 b.unit=r.template read<uint32_t>(player+0x2C);
 auto header=datum(r,s.objects,b.unit,12,4096);
 require(r.template read<uint8_t>(header+3)==0&&!(r.template read<uint8_t>(header+2)&0x10),"H2 local biped is deleted or unavailable");
 auto offset=r.template read<uint32_t>(header+8);
 require(offset<0x10000000,"H2 object allocation unavailable");
 b.object=((s.pool+0x57)&~uintptr_t(15))+offset;
 require(r.template read<uint32_t>(b.object+0x140)==b.player,"H2 biped is not owned by the local player");
 require(r.template read<uint32_t>(b.object+0x14)==none,"H2 movement is unavailable while attached or in a vehicle");
 float health=r.template read<float>(b.object+0xEC);
 require(std::isfinite(health)&&health>0,"H2 living biped is required");
 require(r.template read<uint8_t>(s.control)==1&&!r.template read<uint8_t>(s.control+0x10)&&r.template read<uint32_t>(s.control+0x18)==b.unit,"H2 live player control/view mapping is unavailable");
 b.view=s.control+0x38;
 b.havok=r.template read<uint32_t>(b.object+0xB4);
 auto component=datum(r,s.components,b.havok,0xC0,4096);
 require(r.template read<int32_t>(component+0x78)==1,"H2 biped physics body is ambiguous");
 auto record=pointer(r,component+0x70),entity=pointer(r,record+0x40);
 require(pointer(r,entity)==s.module+0xB71058&&!r.template read<uint8_t>(entity+0x70),"H2 dynamic rigid body is unavailable");
 b.motion=pointer(r,entity+0x68);auto vt=pointer(r,b.motion)-s.module;
 require(vt==0xB71088||vt==0xB71188||vt==0xB72F80||vt==0xB73080,"H2 rigid motion type is unsupported");
 require(session(r,s.module)==s,"H2 session changed during player resolution");return b;
}
struct Pose { Vec3 feet,previous,current,origin,velocity; Angles view; };
template<class R> Pose pose(R& r,const Body& b) {
 Pose p{r.template read<Vec3>(b.object+0x64),r.template read<Vec3>(b.motion+0x70),r.template read<Vec3>(b.motion+0x80),r.template read<Vec3>(b.motion+0xC0),r.template read<Vec3>(b.motion+0x50),r.template read<Angles>(b.view)};
 require(finite(p.feet)&&finite(p.previous)&&finite(p.current)&&finite(p.origin)&&finite(p.velocity)&&valid(p.view),"H2 movement contains non-finite data");
 require(length2(p.current-p.origin)<9.f&&length2(p.origin-p.feet)<9.f,"H2 body/position mapping is inconsistent");
 return p;
}
template<class R> void validateLook(R& r,const Body& b,Angles view) {
 auto f=forward(view),camera=r.template read<Vec3>(b.context.module+0x15F299C);
 require(finite(camera)&&std::fabs(length2(camera)-1.f)<0.05f&&f.x*camera.x+f.y*camera.y+f.z*camera.z>0.98f,"H2 live view does not agree with the local player camera");
}
enum class Action { Save, Back, AutoCrouch };
struct Bookmark { uint64_t generation{};Vec3 feet{},velocity{};Angles view{}; };
struct Write { uintptr_t address{};std::array<uint8_t,12> before{},after{};size_t bytes{}; };
struct Plan { std::array<Write,5> writes{};size_t count{}; };
template<class T> void add(Plan& p,uintptr_t at,T before,T after) {
 auto& w=p.writes.at(p.count++);w.address=at;w.bytes=sizeof(T);static_assert(sizeof(T)<=12);
 std::memcpy(w.before.data(),&before,sizeof(T));std::memcpy(w.after.data(),&after,sizeof(T));
}
inline Plan plan(Action action,const Body& b,const Pose& p,const std::optional<Bookmark>& saved,uint64_t generation) {
 require(action!=Action::Save,"Save is a read-only bookmark operation");
 Vec3 delta;
 if(action==Action::Back) {
  require(saved&&saved->generation==generation,"No H2 spot saved in this map/session");
  require(finite(saved->feet)&&finite(saved->velocity)&&valid(saved->view),"H2 bookmark is invalid");delta=saved->feet-p.feet;
 } else {auto f=forward(p.view);delta={f.x*0.2f,f.y*0.2f,f.z*0.2f};}
 require(finite(delta)&&finite(p.previous+delta)&&finite(p.current+delta)&&finite(p.origin+delta),"H2 destination is not finite");
 Plan out;add(out,b.motion+0x70,p.previous,p.previous+delta);add(out,b.motion+0x80,p.current,p.current+delta);add(out,b.motion+0xC0,p.origin,p.origin+delta);
 if(action==Action::Back) {add(out,b.motion+0x50,p.velocity,saved->velocity);add(out,b.view,p.view,saved->view);}
 return out;
}
enum class Result { Applied, Refused, RolledBack, Uncertain };
// Executed once on the simulation thread. Failed partial writes are restored only
// while the exact same owned body/session remains valid. Never roll back into a new body.
template<class IO,class Verify> Result commit(IO& io,const Plan& p,Verify verify) {
 auto matches=[&](const Write& w,const auto& expected) {std::array<uint8_t,12> actual{};return io.readBytes(w.address,actual.data(),w.bytes)&&std::memcmp(actual.data(),expected.data(),w.bytes)==0;};
 try {if(!verify())return Result::Refused;for(size_t i=0;i<p.count;++i)if(!io.writable(p.writes[i].address,p.writes[i].bytes)||!matches(p.writes[i],p.writes[i].before))return Result::Refused;}catch(...){return Result::Refused;}
 size_t attempted=0;
 try {
  for(size_t i=0;i<p.count;++i) {if(!verify())throw std::runtime_error("identity changed");attempted=i+1;const auto& w=p.writes[i];if(!io.writeBytes(w.address,w.after.data(),w.bytes)||!matches(w,w.after))throw std::runtime_error("write/readback failed");}
  if(!verify())throw std::runtime_error("identity changed");return Result::Applied;
 }catch(...) {
  if(!attempted)return Result::Refused;
  try {if(!verify())return Result::Uncertain;for(size_t i=attempted;i>0;--i){if(!verify())return Result::Uncertain;const auto& w=p.writes[i-1];if(!io.writeBytes(w.address,w.before.data(),w.bytes)||!matches(w,w.before))return Result::Uncertain;}return Result::RolledBack;}catch(...){return Result::Uncertain;}
 }
}
}
