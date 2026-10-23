#include "../HCMInternal/QuickMenuPause.h"
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
int main(){
 auto r=fixture();r.put(globals+8,int32_t(1));r.put(globals+0xC,uint8_t(1));r.put(globals+0x2C8,uint8_t(0));r.put(pg,int16_t(1));
 assert(quickmenu::soloMenuPause(r,base,true,true));
 assert(!quickmenu::soloMenuPause(r,base,false,true));assert(!quickmenu::soloMenuPause(r,base,true,false));
 for(uint8_t sim:{uint8_t(0),uint8_t(2),uint8_t(3),uint8_t(4),uint8_t(5),uint8_t(255)}){auto q=r;q.put(globals+0xC,sim);assert(!quickmenu::soloMenuPause(q,base,true,true));}
 for(int32_t mode:{0,2,3}){auto q=r;q.put(globals+8,mode);assert(!quickmenu::soloMenuPause(q,base,true,true));}
 for(uintptr_t at:{globals,globals+0x2C8}){auto q=r;q.put(at,uint8_t(1));assert(!quickmenu::soloMenuPause(q,base,true,true));}
 for(uintptr_t at:{globals+1,globals+0x1808}){auto q=r;q.put(at,uint8_t(0));assert(!quickmenu::soloMenuPause(q,base,true,true));}
 for(uintptr_t at:{pg,pg+8,pg+10})for(int16_t count:{int16_t(0),int16_t(2)}){auto q=r;q.put(at,count);assert(!quickmenu::soloMenuPause(q,base,true,true));}
 auto q=r;q.data.erase(globals+0x2C8);assert(!quickmenu::soloMenuPause(q,base,true,true));q=r;q.data[base+0x108]^=1;assert(!quickmenu::soloMenuPause(q,base,true,true));
 assert(r.calls==0 && q.calls==0);
 std::cout<<"PASS: production menu-pause predicate permits only verified solo H2 campaign; rejects multiplayer, co-op, clients/servers, loading, other titles, multiple users and unknown/unreadable builds; no memory writes\n";
}
