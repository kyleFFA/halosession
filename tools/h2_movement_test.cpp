#include "../HCMInternal/H2MovementCore.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <limits>
#include <windows.h>
using namespace h2movement;
struct Fake {
 std::map<uintptr_t,uint8_t> data;
 int calls=0,failCall=0;bool canWrite=true,partial=false;
 template<class T> void put(uintptr_t a,T v){auto b=(uint8_t*)&v;for(size_t i=0;i<sizeof(T);++i)data[a+i]=b[i];}
 template<class T> T read(uintptr_t a){T v{};if(!readBytes(a,&v,sizeof v))throw std::runtime_error("unreadable");return v;}
 bool readBytes(uintptr_t a,void* out,size_t n){auto b=(uint8_t*)out;for(size_t i=0;i<n;++i){auto it=data.find(a+i);if(it==data.end())return false;b[i]=it->second;}return true;}
 bool writable(uintptr_t,size_t){return canWrite;}
 bool writeBytes(uintptr_t a,const void* in,size_t n){++calls;auto b=(const uint8_t*)in;if(calls==failCall){if(partial)data[a]=b[0];return false;}for(size_t i=0;i<n;++i)data[a+i]=b[i];return true;}
};
constexpr uintptr_t base=0x180000000,globals=0x200000000,pg=0x200010000,ph=0x200020000,oh=0x200030000,pool=0x200040000,hc=0x200050000,control=0x200060000,bsp=0x200070000,timer=0x200080000,records=0x200090000,testEntity=0x2000A0000,motion=0x2000B0000;
constexpr uint32_t pd=0x12340000,ud=0x23450001,hd=0x34560002;
constexpr uintptr_t testPlayer=ph+0x100,testHeader=oh+0x100+12,object=pool+0x50+0x400,testComponent=hc+0x100+2*0xC0;
void array(Fake& r,uintptr_t h,int stride,int count){r.put(h+0x24,stride);r.put(h+0x3C,count);r.put(h+0x48,int64_t(0x100));}
Fake fixture(){
 Fake r;
 r.put(base,uint16_t(0x5A4D));r.put(base+0x3C,uint32_t(0x100));r.put(base+0x100,uint32_t(0x4550));r.put(base+0x104,uint16_t(0x8664));r.put(base+0x108,uint32_t(0x68A0F0F2));r.put(base+0x150,uint32_t(0x2A38000));
 r.put(base+0x69D040,std::array<uint8_t,12>{0x83,0xF9,0xFF,0x74,0x0F,0x48,0x8B,0x05,0xD4,0x39,0x7E,0x00});
 r.put(base+0x6A6420,std::array<uint8_t,13>{0x48,0x8B,0x05,0x51,0xA6,0x7D,0x00,0xB2,0x01,0x0F,0xB6,0x48,0x0C});
 r.put(base+0x6C0E10,std::array<uint8_t,11>{0x4C,0x69,0xC3,0xB8,0x00,0x00,0x00,0x48,0x83,0xC2,0x38});
 for(auto [at,p]:{std::pair<uintptr_t,uintptr_t>{0xE80A78,globals},{0xE80A20,pg},{0xE80A28,ph},{0x18B7398,oh},{0x18B7360,pool},{0x1648940,hc},{0x15E40D0,control},{0xE6F770,bsp},{0x15FE008,timer}})r.put(base+at,p);
 r.put(globals,uint8_t(0));r.put(globals+1,uint8_t(1));r.put(globals+2,int16_t(0));r.put(globals+8,int32_t(2));r.put(globals+0xC,uint8_t(5));r.put(globals+0x18,uint64_t(123));r.put(globals+0x1808,uint8_t(1));
 r.put(pg+6,uint8_t(0));r.put(pg+7,uint8_t(0));r.put(pg+8,int16_t(1));r.put(pg+10,int16_t(1));
 for(int i=0;i<4;++i){r.put(pg+12+4*i,i==0?pd:none);r.put(pg+28+4*i,i==0?pd:none);}
 array(r,ph,0x224,16);array(r,oh,12,2048);array(r,hc,0xC0,32);
 r.put(testPlayer,uint16_t(pd>>16));r.put(testPlayer+0x28,int16_t(0));r.put(testPlayer+0x24,int32_t(0));r.put(testPlayer+0x2C,ud);
 r.put(testHeader,uint16_t(ud>>16));r.put(testHeader+2,uint8_t(0));r.put(testHeader+3,uint8_t(0));r.put(testHeader+8,uint32_t(0x400));
 r.put(object+0x140,pd);r.put(object+0x14,none);r.put(object+0xEC,1.f);r.put(object+0xB4,hd);r.put(object+0x64,Vec3{10,20,30});
 r.put(control,uint8_t(1));r.put(control+0x10,uint8_t(0));r.put(control+0x18,ud);r.put(control+0x38,Angles{0,0});
 r.put(testComponent,uint16_t(hd>>16));r.put(testComponent+0x70,records);r.put(testComponent+0x78,int32_t(1));r.put(records+0x40,testEntity);r.put(testEntity,base+0xB71058);r.put(testEntity+0x70,uint8_t(0));r.put(testEntity+0x68,motion);r.put(motion,base+0xB73080);
 r.put(motion+0x50,Vec3{1,2,3});r.put(motion+0x70,Vec3{9,19,30.3625f});r.put(motion+0x80,Vec3{10,20,30.3625f});r.put(motion+0xC0,Vec3{10,20,30});r.put(base+0x15F299C,Vec3{1,0,0});return r;
}
template<class F> void refuses(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}assert(rejected);}
template<class T> void rejects(uintptr_t at,T value){auto r=fixture();r.put(at,value);refuses([&]{auto s=session(r,base);auto b=body(r,s);auto p=pose(r,b);validateLook(r,b,p.view);});assert(r.calls==0);}
struct ReadOnly {
 HANDLE handle;
 explicit ReadOnly(DWORD pid):handle(OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)){if(!handle)throw std::runtime_error("Cannot open process read-only");}
 ~ReadOnly(){CloseHandle(handle);}
 template<class T> T read(uintptr_t at){T v{};SIZE_T got=0;if(!ReadProcessMemory(handle,(void*)at,&v,sizeof v,&got)||got!=sizeof v)throw std::runtime_error("Live read unavailable");return v;}
};
int main(int argc,char** argv){
 if(argc==4&&std::string(argv[1])=="--read-only") {
  try {
   ReadOnly live(static_cast<DWORD>(std::stoul(argv[2])));auto module=std::stoull(argv[3],nullptr,0);verifyBuild(live,module);
   for(int attempt=0;attempt<20;++attempt){auto currentSession=session(live,module);auto tick=live.read<uint32_t>(currentSession.timer+8);auto currentBody=body(live,currentSession);auto currentPose=pose(live,currentBody);validateLook(live,currentBody,currentPose.view);if(tick!=live.read<uint32_t>(currentSession.timer+8)||body(live,currentSession)!=currentBody)continue;
    std::cout<<"READ-ONLY PASS tick="<<tick<<" authority="<<int(currentSession.simulation)<<" player="<<std::hex<<currentBody.player<<" unit="<<currentBody.unit<<std::dec<<" feet="<<currentPose.feet.x<<","<<currentPose.feet.y<<","<<currentPose.feet.z<<" center_offset="<<currentPose.current.z-currentPose.feet.z<<" view="<<currentPose.view.yaw<<","<<currentPose.view.pitch<<"\n";return 0;}
   throw std::runtime_error("No coherent tick sample; retry while remaining on foot");
  }catch(const std::exception& e){std::cerr<<"READ-ONLY REFUSED: "<<e.what()<<"\n";return 2;}
 }
 auto r=fixture();verifyBuild(r,base);
 for(uintptr_t at:{base,base+0x104,base+0x108,base+0x150,base+0x69D040,base+0x6A6420,base+0x6C0E10}){auto wrong=fixture();wrong.data[at]^=1;refuses([&]{verifyBuild(wrong,base);});}
 auto s=session(r,base);auto b=body(r,s);auto p=pose(r,b);validateLook(r,b,p.view);
 for(uint8_t mode:{uint8_t(1),uint8_t(3),uint8_t(5)}){auto q=fixture();q.put(globals+0xC,mode);assert(session(q,base).simulation==mode);}
 for(uint8_t mode:{uint8_t(0),uint8_t(2),uint8_t(4),uint8_t(6),uint8_t(255)})rejects(globals+0xC,mode);
 rejects(globals,uint8_t(1));rejects(globals+1,uint8_t(0));rejects(globals+2,int16_t(-1));rejects(globals+8,int32_t(3));rejects(globals+0x1808,uint8_t(0));
 rejects(pg+8,int16_t(2));rejects(pg+10,int16_t(0));rejects(pg+6,uint8_t(1));rejects(pg+7,uint8_t(1));rejects(pg+28,none);rejects(pg+16,pd);rejects(pg+32,pd);
 rejects(testPlayer+0x28,int16_t(1));rejects(testPlayer+0x24,int32_t(-1));rejects(testPlayer+0x2C,none);rejects(testPlayer,uint16_t(99));rejects(ph+0x24,int32_t(123));rejects(oh+0x3C,int32_t(1));rejects(hc+0x48,int64_t(-1));
 rejects(testHeader,uint16_t(99));rejects(testHeader+2,uint8_t(0x10));rejects(testHeader+3,uint8_t(1));rejects(testHeader+8,none);rejects(object+0x140,pd+1);rejects(object+0x14,ud);rejects(object+0xEC,0.f);rejects(object+0xEC,std::numeric_limits<float>::quiet_NaN());
 rejects(control,uint8_t(0));rejects(control+0x10,uint8_t(1));rejects(control+0x18,none);rejects(testComponent,uint16_t(99));rejects(testComponent+0x78,int32_t(2));rejects(testEntity,uintptr_t(0x123456));rejects(testEntity+0x70,uint8_t(1));rejects(motion,base+0xB73180);
 rejects(control+0x38,Angles{0,2});rejects(base+0x15F299C,Vec3{-1,0,0});rejects(motion+0x80,Vec3{100,100,100});rejects(motion+0x50,Vec3{NAN,0,0});
 // Direction includes pitch, exactly 0.2; all position fields move together, preserving COM.
 p.view={1.2f,0.7f};auto ac=plan(Action::AutoCrouch,b,p,{},1);assert(ac.count==3);
 Vec3 dest{};memcpy(&dest,ac.writes[1].after.data(),sizeof dest);assert(std::fabs(std::sqrt(length2(dest-p.current))-0.2f)<0.00001f);assert(dest.z>p.current.z);
 auto f=forward(p.view);assert(std::fabs(dest.z-p.current.z-f.z*0.2f)<0.00001f);
 for(Angles aim:{Angles{0,0},Angles{1.5707963f,0},Angles{0,-1.5707963f},Angles{0,1.5707963f},Angles{2.1f,-0.8f}}) {
  auto sample=p;sample.view=aim;auto target=plan(Action::AutoCrouch,b,sample,{},1);auto expected=forward(aim);
  for(size_t i=0;i<3;++i){Vec3 before{},after{};memcpy(&before,target.writes[i].before.data(),sizeof before);memcpy(&after,target.writes[i].after.data(),sizeof after);auto delta=after-before;
   assert(std::fabs(std::sqrt(length2(delta))-0.2f)<0.00001f);assert(std::fabs(delta.x-expected.x*0.2f)<0.00001f&&std::fabs(delta.y-expected.y*0.2f)<0.00001f&&std::fabs(delta.z-expected.z*0.2f)<0.00001f);
  }
 }
 p=pose(r,b);std::optional<Bookmark> mark=Bookmark{1,Vec3{50,60,70},Vec3{4,5,6},Angles{2,0.3f}};
 auto back=plan(Action::Back,b,p,mark,1);assert(back.count==5);assert(commit(r,back,[&]{return body(r,session(r,base))==b;})==Result::Applied);
 assert(r.read<Vec3>(motion+0xC0)==mark->feet);assert(std::fabs(r.read<Vec3>(motion+0x80).z-70.3625f)<0.00001f);assert(r.read<Vec3>(motion+0x50)==mark->velocity);assert(r.read<Angles>(control+0x38)==mark->view);
 refuses([&]{plan(Action::Back,b,p,mark,2);});refuses([&]{plan(Action::Back,b,p,{},1);});auto bad=mark;bad->feet.x=NAN;refuses([&]{plan(Action::Back,b,p,bad,1);});
 // A bookmark contains values, not body pointers; using a fresh body computes its own COM offset.
 auto newBody=b;newBody.motion=motion+0x1000;newBody.unit=0x45670003;auto newPose=p;newPose.current.z+=0.1f;
 auto respawn=plan(Action::Back,newBody,newPose,mark,1);assert(respawn.writes[1].address==newBody.motion+0x80);memcpy(&dest,respawn.writes[1].after.data(),sizeof dest);assert(std::fabs(dest.z-70.4625f)<0.0001f);
 for(int fail=1;fail<=5;++fail){auto q=fixture();auto original=q.data;q.failCall=fail;q.partial=true;assert(commit(q,back,[]{return true;})==Result::RolledBack);assert(q.data==original);}
 {auto q=fixture();q.canWrite=false;assert(commit(q,back,[]{return true;})==Result::Refused);assert(q.calls==0);}
 {auto q=fixture();assert(commit(q,back,[]{return false;})==Result::Refused);assert(q.calls==0);}
 {auto q=fixture();int validations=0;assert(commit(q,back,[&]{return ++validations<3;})==Result::Uncertain);assert(q.calls==1);}
 {auto q=fixture();q.put(motion+0x80,Vec3{1,1,1});assert(commit(q,back,[]{return true;})==Result::Refused);assert(q.calls==0);}
 // Auto-crouch cannot alter either aim or velocity.
 {auto q=fixture();auto before=pose(q,b);auto target=plan(Action::AutoCrouch,b,before,{},1);assert(commit(q,target,[]{return true;})==Result::Applied);auto after=pose(q,b);assert(after.view==before.view&&after.velocity==before.velocity);}
 std::cout<<"PASS: H2 ownership/authority, stale/vehicle/dead/nonfinite refusals, pitched 0.2 movement, COM/respawn bookmarks, readback/rollback and no-aim/no-velocity auto-crouch\n";
}
