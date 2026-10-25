#pragma once
// Session D3D11 OBS Game Capture route. OBS ABI 1.8 is read-only; no fixed OBS RVA.
// ABI/order references: obsproject/obs-studio 32.2.2 graphics-hook-info.h and dxgi-capture.cpp.
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <Zydis/Zydis.h>
namespace sessionobs {
enum class Status { Off, Waiting, Unsupported, CaptureOverlays, Ready, Active, Restoring, GridTransition };
inline std::atomic<Status> status{Status::Off};
inline std::atomic_bool requested{false};
inline std::atomic_uint64_t gridPostUntil{0},earlyGridSequence{0};
inline void request(bool value){requested=value;if(!value)gridPostUntil=0;}
// A short lease comes only from a successfully observed post-capture callback.
inline bool lateGridLease(uint64_t now){return requested.load()&&now<gridPostUntil.load();}
inline const char* text(Status s){switch(s){
 case Status::Off:return "Off - normal Session capture routing.";
 case Status::Waiting:return "Waiting - no supported OBS Game Capture hook for this game window. Bypass is not active.";
 case Status::Unsupported:return "Unavailable - OBS hook/ABI or Present route is unsupported. Bypass is not active; overlays may be captured.";
 case Status::CaptureOverlays:return "Unavailable - turn OFF Capture third-party overlays in OBS Game Capture properties. Bypass is not active.";
 case Status::Ready:return "Waiting for the OBS post-capture callback. Exclusion is not verified.";
 case Status::GridTransition:return "Post-capture route observed; the Halo 3 grid is transitioning or unavailable. An early grid may still be captured.";
 case Status::Active:return "Post-capture route observed. Verify a recording before relying on exclusion.";
 default:return "Restoring normal capture routing.";
}}
inline bool copy(uintptr_t from,void* out,size_t size){SIZE_T done=0;return from&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(from),out,size,&done)&&done==size;}
template<class T> bool read(uintptr_t from,T& out){return copy(from,&out,sizeof out);}
inline bool executable(uintptr_t p){MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;return (m.Protect&0xff)==PAGE_EXECUTE_READ||(m.Protect&0xff)==PAGE_EXECUTE_READWRITE||(m.Protect&0xff)==PAGE_EXECUTE_WRITECOPY;}
struct Image {uintptr_t base=0;size_t size=0;IMAGE_NT_HEADERS64 nt{};};
inline bool image(HMODULE module,Image& out){out={};IMAGE_DOS_HEADER dos{};out.base=reinterpret_cast<uintptr_t>(module);if(!read(out.base,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<=0||dos.e_lfanew>0x100000)return false;
 if(!read(out.base+dos.e_lfanew,out.nt)||out.nt.Signature!=IMAGE_NT_SIGNATURE||out.nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||out.nt.FileHeader.NumberOfSections>32||out.nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return false;out.size=out.nt.OptionalHeader.SizeOfImage;return out.size>=4096&&out.size<128*1024*1024;
}
inline uintptr_t jump(uintptr_t at){std::array<unsigned char,16> b{};if(!copy(at,b.data(),b.size()))return 0;int32_t disp=0;uintptr_t value=0;
 if(b[0]==0xe9){memcpy(&disp,b.data()+1,4);return at+5+intptr_t(disp);}if(b[0]==0xeb)return at+2+intptr_t(int8_t(b[1]));
 if(b[0]==0xff&&b[1]==0x25){memcpy(&disp,b.data()+2,4);if(read(at+6+intptr_t(disp),value))return value;}return 0;
}
inline bool ownedEntry(uintptr_t entry,const Image& obs){auto p=entry;for(int i=0;i<4;++i){p=jump(p);if(!p||!executable(p))return false;if(p>=obs.base&&p-obs.base<obs.size)return true;}return false;}
inline bool jumpsBack(uintptr_t candidate,uintptr_t entry){if(!executable(candidate))return false;std::array<unsigned char,32> code{};if(!copy(candidate,code.data(),code.size()))return false;ZydisDecoder decoder;ZydisDecoderInit(&decoder,ZYDIS_MACHINE_MODE_LONG_64,ZYDIS_STACK_WIDTH_64);
 for(size_t n=0;n<code.size();){ZydisDecodedInstruction ins{};ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder,code.data()+n,code.size()-n,&ins,ops))||!ins.length)return false;
  if(ins.mnemonic==ZYDIS_MNEMONIC_JMP){
   const auto dest=jump(candidate+n);
   if(dest>=entry&&dest-entry<0x20)return true;
   // Detours can relocate an already-hooked prologue: JMP previous hook, followed by
   // its own canonical FF25 jump-back stub. That second stub still identifies the entry.
   // Only accept this exact two-branch layout at the start, never arbitrary later code.
   if(n==0&&(ins.length==5||ins.length==6)&&executable(dest)&&ins.length+6<=code.size()&&code[ins.length]==0xff&&code[ins.length+1]==0x25){
    const auto back=jump(candidate+ins.length);
    return back==entry+ins.length;
   }
   return false;
  }if(ins.mnemonic==ZYDIS_MNEMONIC_RET)return false;n+=ins.length;}return false;
}
struct Probe {Status result=Status::Waiting;uintptr_t slot=0,target=0;HMODULE module=nullptr;};
// Inspect a bounded image and reject ambiguous candidates. Also used by the synthetic module fixture.
inline Probe discover(HMODULE module,uintptr_t entry){Probe p{Status::Unsupported,0,0,module};Image obs{},dxgi{};if(!image(module,obs)||!executable(entry)||!ownedEntry(entry,obs))return p;
 image(GetModuleHandleW(L"dxgi.dll"),dxgi);IMAGE_DOS_HEADER dos{};read(obs.base,dos);const auto sectionTable=obs.base+dos.e_lfanew+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+obs.nt.FileHeader.SizeOfOptionalHeader;
 for(unsigned i=0;i<obs.nt.FileHeader.NumberOfSections;++i){IMAGE_SECTION_HEADER s{};if(!read(sectionTable+i*sizeof s,s))return p;
  if(!(s.Characteristics&IMAGE_SCN_MEM_WRITE)||!(s.Characteristics&IMAGE_SCN_CNT_INITIALIZED_DATA))continue;
  if(s.VirtualAddress>=obs.size||s.Misc.VirtualSize>obs.size-s.VirtualAddress||s.Misc.VirtualSize>1024*1024)return p;
  for(uintptr_t cursor=(obs.base+s.VirtualAddress+7)&~uintptr_t(7),end=obs.base+s.VirtualAddress+s.Misc.VirtualSize;cursor+8<=end;cursor+=8){uintptr_t value=0;if(!read(cursor,value))return p;if(value<0x10000||(value>=obs.base&&value-obs.base<obs.size)||(dxgi.base&&value>=dxgi.base&&value-dxgi.base<dxgi.size)||!jumpsBack(value,entry))continue;
   if(p.slot&&p.slot!=cursor)return Probe{Status::Unsupported,0,0,module};p.slot=cursor;p.target=value;
  }
 }
 if(p.slot)p.result=Status::Ready;return p;
}
struct Info {std::array<unsigned char,648> data{};template<class T>T get(size_t offset)const{T v{};memcpy(&v,data.data()+offset,sizeof v);return v;}};
inline Status infoPolicy(const Info& info,uintptr_t expectedEntry,uintptr_t window,const Image& dxgi){
 if(info.get<uint32_t>(0)!=1||info.get<uint32_t>(4)!=8)return Status::Unsupported;
 if(info.get<uint8_t>(66))return Status::CaptureOverlays;
 if(info.get<uint32_t>(12)!=uint32_t(window)||!info.get<uint32_t>(20)||!info.get<uint32_t>(24))return Status::Waiting;
 const auto rva=info.get<uint32_t>(92);if(!rva||rva>=dxgi.size||dxgi.base+rva!=expectedEntry)return Status::Unsupported;return Status::Ready;
}
inline Probe probe(uintptr_t entry,uintptr_t window,bool scan=true){HMODULE module=GetModuleHandleW(L"graphics-hook64.dll");if(!module)return {};
 wchar_t name[80];swprintf_s(name,L"CaptureHook_HookInfo%lu",GetCurrentProcessId());HANDLE mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,name);if(!mapping)return {};
 auto view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,648);Info info{};const bool valid=view&&copy(reinterpret_cast<uintptr_t>(view),info.data.data(),info.data.size());if(view)UnmapViewOfFile(view);CloseHandle(mapping);
 Image dxgi{};if(!valid||!image(GetModuleHandleW(L"dxgi.dll"),dxgi))return {Status::Unsupported};auto gate=infoPolicy(info,entry,window,dxgi);if(gate!=Status::Ready)return {gate};return scan?discover(module,entry):Probe{Status::Ready,0,0,module};
}
// One game Present owns the render callback. Nested/foreign swapchains never duplicate it.
struct Frame {void* swap=nullptr;bool post=false,drawn=false,earlyGrid=false;bool drawNormal()const{return !post&&!drawn;}bool drawPost(void* candidate,bool requested){if(!post||drawn||candidate!=swap||!requested)return false;drawn=true;return true;}};
inline thread_local Frame* frame=nullptr;
inline Frame beginFrame(void* swap,uint64_t& seen){const auto serial=earlyGridSequence.load();Frame next{swap,false,false,serial!=seen};seen=serial;return next;}
inline void finishFrame(const Frame& value,uint64_t now){gridPostUntil=value.post&&value.drawn&&requested.load()?now+250:0;}
inline bool postCapturePass(){return frame&&frame->post&&frame->drawn;}
}
