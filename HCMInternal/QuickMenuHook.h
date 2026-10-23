#pragma once
#include "HookQuiescence.h"
#include "safetyhook.hpp"
namespace quickmenu {
// Main-executable input site: owned by the application thread, independent of title leases.
class NativeHook {
 safetyhook::MidHook hook;
public:
 explicit operator bool()const{return bool(hook);}
 void install(void* at,safetyhook::MidHookFn callback) {
  std::lock_guard freeze(hcmThreadSuspensionMutex());
  if(!hook)hook=safetyhook::create_mid(at,callback);
 }
 bool retire() {
  std::lock_guard freeze(hcmThreadSuspensionMutex());
  if(!hook)return true;if(!hook.disable())return false;
  const auto& stub=hook.stub_allocation();const auto& trampoline=hook.trampoline_allocation();
  if(hook_lifetime::quiescent({stub.address(),stub.size()},{trampoline.address(),trampoline.size()})!=hook_lifetime::Result::Clear)return false;
  hook={};return true;
 }
 ~NativeHook(){while(!retire())Sleep(10);}
};
}
