#include <windows.h>
#include <dxgi.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <safetyhook.hpp>
#include "../HCMInternal/SessionOBSBypass.h"
using Fn=HRESULT(__stdcall*)(void*,UINT,UINT);
static uintptr_t slot=0;
static bool requested=true;
static int local=0,captured=0,draws=0;
static HRESULT __stdcall fakeOBS(void* swap,UINT interval,UINT flags){captured=local;return (*reinterpret_cast<Fn*>(slot))(swap,interval,flags);}
static safetyhook::InlineHook hook;
static HRESULT __stdcall afterCapture(void* swap,UINT interval,UINT flags){auto f=sessionobs::frame;if(f&&!(flags&DXGI_PRESENT_TEST)&&f->drawPost(swap,requested)){local=100;++draws;}return hook.stdcall<HRESULT>(swap,interval,flags);}
int main(){
 using namespace sessionobs;
 assert(probe(0,0).result==Status::Waiting);
 Info info{};auto put=[&](size_t at,uint32_t v){memcpy(info.data.data()+at,&v,4);};Image dxgi{};dxgi.base=0x100000;dxgi.size=0x10000;
 put(0,1);put(4,8);put(12,42);put(20,1920);put(24,1080);put(92,0x1234);
 assert(infoPolicy(info,0x101234,42,dxgi)==Status::Ready);
 info.data[66]=1;assert(infoPolicy(info,0x101234,42,dxgi)==Status::CaptureOverlays);info.data[66]=0;
 put(4,9);assert(infoPolicy(info,0x101234,42,dxgi)==Status::Unsupported);put(4,8);
 assert(infoPolicy(info,0x101235,42,dxgi)==Status::Unsupported);assert(infoPolicy(info,0x101234,43,dxgi)==Status::Waiting);
 auto mem=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(mem);auto base=reinterpret_cast<uintptr_t>(mem);
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(mem);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(mem+0x80);nt->Signature=IMAGE_NT_SIGNATURE;nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;nt->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);nt->FileHeader.NumberOfSections=1;nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt->OptionalHeader.SizeOfImage=0x4000;
 auto section=IMAGE_FIRST_SECTION(nt);section->VirtualAddress=0x2000;section->Misc.VirtualSize=0x100;section->Characteristics=IMAGE_SCN_MEM_WRITE|IMAGE_SCN_CNT_INITIALIZED_DATA;
 // Synthetic dxgi entry -> OBS hook -> relocated trampoline -> original return.
 auto entry=base+0x5000,target=base+0x6000,obsEntry=base+0x1000;slot=base+0x2000;
 mem[0x5000]=0xe9;int32_t disp=int32_t(obsEntry-(entry+5));memcpy(mem+0x5001,&disp,4);mem[0x5005]=0xc3;
 mem[0x1000]=0x48;mem[0x1001]=0xb8;auto handler=reinterpret_cast<uintptr_t>(&fakeOBS);memcpy(mem+0x1002,&handler,8);mem[0x100a]=0xff;mem[0x100b]=0xe0;
 mem[0x6000]=0xb8;uint32_t value=42;memcpy(mem+0x6001,&value,4);mem[0x6005]=0xe9;disp=int32_t((entry+5)-(target+10));memcpy(mem+0x6006,&disp,4);*reinterpret_cast<uintptr_t*>(slot)=target;
 FlushInstructionCache(GetCurrentProcess(),mem,0x10000);
 auto found=discover(reinterpret_cast<HMODULE>(base),entry);assert(found.result==Status::Ready&&found.target==target&&found.slot==slot);
 // Ambiguity and invalid candidates must fail closed.
 *reinterpret_cast<uintptr_t*>(slot+8)=target;assert(discover(reinterpret_cast<HMODULE>(base),entry).result==Status::Unsupported);*reinterpret_cast<uintptr_t*>(slot+8)=0;
 assert(!jumpsBack(1,entry));assert(!jumpsBack(obsEntry,entry));
 // Reproduce the live layout: OBS relocated a prior JMP, then appended FF25 -> dxgi+5.
 mem[0x7000]=0xb8;memcpy(mem+0x7001,&value,4);mem[0x7005]=0xc3;
 mem[0x6000]=0xe9;disp=int32_t((base+0x7000)-(target+5));memcpy(mem+0x6001,&disp,4);
 mem[0x6005]=0xff;mem[0x6006]=0x25;disp=0x3d;memcpy(mem+0x6007,&disp,4);
 *reinterpret_cast<uintptr_t*>(target+0x48)=entry+5;
 FlushInstructionCache(GetCurrentProcess(),mem,0x10000);
 assert(jumpsBack(target,entry));found=discover(reinterpret_cast<HMODULE>(base),entry);assert(found.result==Status::Ready&&found.slot==slot&&found.target==target);
 // Wrong jump-back, unavailable prior target, duplicate candidates and later arbitrary stubs fail closed.
 *reinterpret_cast<uintptr_t*>(target+0x48)=entry+0x100;assert(!jumpsBack(target,entry));*reinterpret_cast<uintptr_t*>(target+0x48)=entry+6;assert(!jumpsBack(target,entry));*reinterpret_cast<uintptr_t*>(target+0x48)=entry+5;
 DWORD oldProtect=0;VirtualProtect(mem+0x7000,0x1000,PAGE_NOACCESS,&oldProtect);assert(!jumpsBack(target,entry));VirtualProtect(mem+0x7000,0x1000,oldProtect,&oldProtect);
 *reinterpret_cast<uintptr_t*>(slot+8)=target;assert(discover(reinterpret_cast<HMODULE>(base),entry).result==Status::Unsupported);*reinterpret_cast<uintptr_t*>(slot+8)=0;
 auto created=safetyhook::InlineHook::create(reinterpret_cast<void*>(target),&afterCapture);assert(created);hook=std::move(*created);
 for(int i=0;i<200;++i){local=0;draws=0;captured=-1;Frame f{reinterpret_cast<void*>(42),true,false};frame=&f;auto call=reinterpret_cast<Fn>(entry);assert(call(f.swap,0,0)==42);assert(captured==0&&local==100&&draws==1&&f.drawn);assert(!f.drawPost(f.swap,true));assert(!f.drawPost(reinterpret_cast<void*>(43),true));frame=nullptr;}
 auto disabled=hook.disable();assert(disabled);hook.reset();
 // Off restores normal draw -> capture -> Present order with no duplicate callback.
 local=100;draws=0;captured=-1;Frame normal{reinterpret_cast<void*>(42),false,false};assert(normal.drawNormal());frame=&normal;assert(reinterpret_cast<Fn>(entry)(normal.swap,0,0)==42);assert(captured==100&&draws==0);frame=nullptr;
 // Validate removal and rediscovery without retaining stale addresses.
 assert(discover(reinterpret_cast<HMODULE>(base),entry).target==target);*reinterpret_cast<uintptr_t*>(slot)=0;assert(discover(reinterpret_cast<HMODULE>(base),entry).result==Status::Unsupported);
 DWORD previous=0;VirtualProtect(mem+0x6000,0x1000,PAGE_NOACCESS,&previous);assert(!jumpsBack(target,entry));VirtualProtect(mem+0x6000,0x1000,previous,&previous);
 VirtualFree(mem,0,MEM_RELEASE);std::cout<<"PASS: read-only OBS ABI/options/window/Present gates; bounded unambiguous discovery; direct and canonical prior-hook trampoline discovery; actual synthetic chained-Detours hook activation and 200 post-capture frames; off restoration; wrong/duplicate swap suppression; unsupported/no-OBS/guarded-memory cases; cleanup and rediscovery. No OBS/game accessed.\n";
}
