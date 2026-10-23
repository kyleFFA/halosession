#pragma once
#include "ScopedThreadSuspender.h"

// After entry jumps are disabled, a callback can still RETURN into a mid-hook
// stub. Counters in the C++ callback do not cover that machine-code epilogue.
// Inspect stopped instruction pointers AND active stack return-address slots.
// Unreadable/unknown threads are not quiescent; keep their hook/module owned.
namespace hook_lifetime {
struct Range { uintptr_t begin=0; size_t size=0; };
inline bool contains(Range r,uintptr_t address) { return address>=r.begin && address-r.begin<r.size; }
inline bool references(uintptr_t rsp,uintptr_t end,Range a,Range b,bool& found) {
    if(rsp>end || end-rsp>8*1024*1024) return false;
    __try {
        for(auto p=rsp;p+sizeof(uintptr_t)<=end;p+=sizeof(uintptr_t)) {
            const auto value=*reinterpret_cast<const uintptr_t*>(p);
            if(contains(a,value) || contains(b,value)) {found=true;return true;}
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool stackBase(void* teb,uintptr_t& end) {
    __try { end=reinterpret_cast<uintptr_t>(static_cast<NT_TIB*>(teb)->StackBase);return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
using Query=NTSTATUS(NTAPI*)(HANDLE,THREADINFOCLASS,void*,ULONG,ULONG*);
inline Query query=nullptr;
inline void initialize() { query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationThread")); }
enum class Result { Clear, Busy, Unreadable };
inline Result quiescent(Range a,Range b) {
    struct ThreadInfo { NTSTATUS status;void* teb;CLIENT_ID id;ULONG_PTR affinity;LONG priority;LONG basePriority; };
    if(!query) return Result::Unreadable;
    // Allocations happen in ScopedThreadSuspender before threads stop. Nothing
    // below logs, allocates or waits for a lock held by a suspended thread.
    ScopedThreadSuspender pause;
    if(!pause.snapshotOk() || pause.missedCount()) return Result::Unreadable;
    const auto handles=pause.suspendedHandles();const auto ids=pause.suspendedIds();
    for(size_t i=0;i<pause.suspendedCount();++i) {
        CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;
        if(!GetThreadContext(handles[i],&context)) return Result::Unreadable;
        if(contains(a,context.Rip) || contains(b,context.Rip)) return Result::Busy;
        HANDLE thread=OpenThread(THREAD_QUERY_INFORMATION,FALSE,ids[i]);
        if(!thread) return Result::Unreadable;
        ThreadInfo info{};const auto status=query(thread,static_cast<THREADINFOCLASS>(0),&info,sizeof(info),nullptr);
        CloseHandle(thread);
        uintptr_t end=0;bool found=false;
        if(status<0 || !stackBase(info.teb,end) || !references(context.Rsp,end,a,b,found)) return Result::Unreadable;
        if(found) return Result::Busy;
    }
    return Result::Clear;
}
}

