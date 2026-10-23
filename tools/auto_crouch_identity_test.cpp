#include "../HCMInternal/AutoCrouchIdentity.h"
#include <windows.h>
#include <tlhelp32.h>
#include <map>
#include <cstring>
#include <iostream>
#include <cassert>
struct Fake {
    std::map<uintptr_t,uint8_t> bytes;
    template<class T> void put(uintptr_t a,T v){for(size_t i=0;i<sizeof(T);++i)bytes[a+i]=((uint8_t*)&v)[i];}
    template<class T> T read(uintptr_t a){T v{};for(size_t i=0;i<sizeof(T);++i)((uint8_t*)&v)[i]=bytes.at(a+i);return v;}
};
constexpr uintptr_t halo=0x10000000,pool=0x20000000,object=0x30000000, testMapping=pool+0x4837E4;
Fake fixture(){Fake f;f.put(halo+0x204FD18,pool);f.put<uint8_t>(pool+0x47025C+0x11,5);f.put<uint8_t>(pool+0x47025C+0x160,0);f.put<int16_t>(testMapping,1);
for(int i=0;i<4;++i){f.put<uint32_t>(testMapping+4+4*i,0xFFFFFFFF);f.put<uint32_t>(testMapping+0x14+4*i,0xFFFFFFFF);}
f.put<uint32_t>(testMapping+4,0xABCD0007);f.put<uint32_t>(testMapping+0x14,0x12340002);f.put<int32_t>(testMapping+0x74+4*7,0);
f.put<uint16_t>(pool+0x50749C+2*0x18,0x1234);f.put<uint8_t>(pool+0x50749C+2*0x18+3,0);f.put(pool+0x50749C+2*0x18+0x10,object);f.put<uint32_t>(object+0x178,0xABCD0007);return f;}
void rejects(Fake f){try{readAutoCrouchIdentity(f,halo);assert(false);}catch(const std::exception&) {}}
struct Live {HANDLE h;template<class T>T read(uintptr_t a){T v{};SIZE_T n=0;if(!ReadProcessMemory(h,(void*)a,&v,sizeof(v),&n)||n!=sizeof(v))throw std::runtime_error("read failed");return v;}};
int main(int argc,char** argv){
const auto& layout = argc>1 && std::string(argv[1])=="--reach" ? autoCrouchReach : argc>1 && std::string(argv[1])=="--odst" ? autoCrouchODST : autoCrouchHalo3;
const wchar_t* moduleName = layout.reach ? L"haloreach.dll" : layout.ownerOffset==0x170 ? L"halo3odst.dll" : L"halo3.dll";
if(argc>1){
 DWORD pid=0;HANDLE ps=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W e{};e.dwSize=sizeof(e);if(Process32FirstW(ps,&e))do{if(!_wcsicmp(e.szExeFile,L"MCC-Win64-Shipping.exe"))pid=e.th32ProcessID;}while(Process32NextW(ps,&e));CloseHandle(ps);if(!pid)return 2;
 HANDLE ms=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);MODULEENTRY32W m{};m.dwSize=sizeof(m);uintptr_t module=0;if(Module32FirstW(ms,&m))do{if(!_wcsicmp(m.szModule,moduleName))module=(uintptr_t)m.modBaseAddr;}while(Module32NextW(ms,&m));CloseHandle(ms);
 Live r{OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)};if(!r.h||!module)return 3;
 try{auto s=readAutoCrouchIdentity(r,module,layout);std::cout<<"READ ONLY: simulation="<<(int)s.simulation<<" local player="<<std::hex<<s.player<<" unit="<<s.unit<<" owner verified\n";}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';CloseHandle(r.h);return 4;}CloseHandle(r.h);return 0;
}
for (const auto& cfg : {autoCrouchODST,autoCrouchReach}) {
 Fake data; const uintptr_t mapOffset=0x24000+cfg.allocationBase,globalOffset=0x258+cfg.allocationBase;
 data.put(halo+cfg.poolPointer,pool);
 data.put<int32_t>(halo+cfg.mappingIndex,9); data.put<int32_t>(halo+cfg.globalsIndex,3);
 data.put<uint32_t>(halo+cfg.allocationOffsets+9*0x30,0x24000); data.put<uint32_t>(halo+cfg.allocationOffsets+3*0x30,0x258);
 data.put<uint8_t>(pool+globalOffset+0x11,5);data.put<uint8_t>(pool+globalOffset+cfg.playbackOffset,0);
 data.put<int16_t>(pool+mapOffset,1);
 for(int i=0;i<4;++i){data.put<uint32_t>(pool+mapOffset+4+4*i,0xFFFFFFFF);data.put<uint32_t>(pool+mapOffset+0x14+4*i,0xFFFFFFFF);}
 data.put<uint32_t>(pool+mapOffset+4,0xABCD0007);data.put<uint32_t>(pool+mapOffset+0x14,0x12340002);data.put<int32_t>(pool+mapOffset+0x74+7*4,0);
 const auto header=pool+cfg.objectTable+2*24;
 data.put<uint16_t>(header,0x1234);data.put<uint8_t>(header+cfg.typeOffset,0);data.put(header+16,object);data.put<uint32_t>(object+cfg.ownerOffset,0xABCD0007);
 assert(readAutoCrouchIdentity(data,halo,cfg).unit==0x12340002);
 if (cfg.allocationBase) { // Regression: reading pool-relative rather than bank-relative globals must fail.
  auto oldLayout=cfg; oldLayout.allocationBase=0;
  try{readAutoCrouchIdentity(data,halo,oldLayout);assert(false);}catch(const std::exception&){}
 }
 auto bad=[&](Fake copy){try {readAutoCrouchIdentity(copy,halo,cfg);assert(false);}catch(const std::exception&) {}};
 for(int mode : {0,2,4,6}){auto copy=data;copy.put<uint8_t>(pool+globalOffset+0x11,static_cast<uint8_t>(mode));bad(copy);}
 auto copy=data;copy.put<uint32_t>(object+cfg.ownerOffset,0xABCD0000);bad(copy);
 copy=data;copy.put<int16_t>(pool+mapOffset,2);bad(copy);
 copy=data;copy.put<uint8_t>(pool+globalOffset+cfg.playbackOffset,1);bad(copy);
 copy=data;copy.put<int32_t>(halo+cfg.mappingIndex,-1);bad(copy);
 copy=data;copy.put<uint32_t>(pool+mapOffset+4,0xFFFFFFFF);bad(copy);
 copy=data;copy.put<uint16_t>(header,0x4321);bad(copy);
}
auto f=fixture();auto s=readAutoCrouchIdentity(f,halo);assert(s.player==0xABCD0007&&s.unit==0x12340002);
for(int mode:{1,3,5}){f=fixture();f.put<uint8_t>(pool+0x47025C+0x11,static_cast<uint8_t>(mode));assert(readAutoCrouchIdentity(f,halo).unit==s.unit);}
for(int mode:{0,2,4,6,255}){f=fixture();f.put<uint8_t>(pool+0x47025C+0x11,static_cast<uint8_t>(mode));rejects(f);}
f=fixture();f.put<uint32_t>(object+0x178,0xABCD0000);rejects(f);
f=fixture();f.put<int32_t>(testMapping+0x74+4*7,1);rejects(f);
f=fixture();f.put<uint32_t>(testMapping+4,0xFFFFFFFF);rejects(f);
f=fixture();f.put<uint32_t>(testMapping+8,0xABCD0003);rejects(f);
f=fixture();f.put<int16_t>(testMapping,2);rejects(f);
f=fixture();f.put<uint16_t>(pool+0x50749C+2*0x18,0x9999);rejects(f);
f=fixture();f.put<uint8_t>(pool+0x47025C+0x160,1);rejects(f);
std::cout<<"Local network player 7 selected; clients/unknown authority, remote owner, stale and ambiguous testMappings rejected\n";
}

