#include "pch.h"
#include "ModuleHookManager.h"
#include "WindowsUtilities.h"

#include "ModuleCache.h"
#include "HookRetirement.h"
#include "HookQuiescence.h"




ModuleHookManager::ModuleHookManager()
{
    hook_lifetime::initialize();

	// We'll make global hooks here that track dll loading/unloading
	PLOG_INFO << "Hooking module load/unload";

	mHook_LoadLibraryA = safetyhook::create_inline(&LoadLibraryA, &newLoadLibraryA);
	mHook_LoadLibraryW = safetyhook::create_inline(&LoadLibraryW, &newLoadLibraryW);
	mHook_LoadLibraryExA = safetyhook::create_inline(&LoadLibraryExA, &newLoadLibraryExA);
	mHook_LoadLibraryExW = safetyhook::create_inline(&LoadLibraryExW, &newLoadLibraryExW); // Crashes on safetyhook 0.5.3 due to shared mem page with VirtualProtect.
	mHook_FreeLibrary = safetyhook::create_inline(&FreeLibrary, &newFreeLibrary);

	//mModuleHooksMap.reserve(6); // probably only the 6 game dll's that we might care about ever
	PLOG_VERBOSE << "Hooking module load/unload done";
}

ModuleHookManager::~ModuleHookManager()
{
	PLOG_VERBOSE << "ModuleHookManager destructor called";

	// We must detach the module-relative hooks before unhooking the library load/unload functions
	// otherwise we could have a stale reference issue
	detachAllHooks();

	mHook_LoadLibraryA.reset();
	mHook_LoadLibraryW.reset();
	mHook_LoadLibraryExA.reset();
	mHook_LoadLibraryExW.reset();
	mHook_FreeLibrary.reset();

}


// This only called on global_kill
void ModuleHookManager::detachAllHooks() {
	// Get all hook elements and detach them
	for (auto& [moduleName, hookVector] : mModuleHooksMap)
	{
		for (auto& hook : hookVector)
		{
			hook->detach();
		}
	}

}





// Called by newFreeLibrary, unhooks any hooks tied to the unloading module
void ModuleHookManager::preModuleUnload_UpdateHooks(std::wstring_view libFilename) 
{
    // H2 is leased until its owner-thread retirement. FreeLibrary reference
    // changes do not define a title exit, and never wait for render callbacks.
    if (libFilename==L"halo2.dll") return;
    std::lock_guard lock(registryMutex);
	// Get a ref to the module-hooks map
	const auto& moduleHooksMap = mModuleHooksMap;

	// Is the currently unloading library in our map?
	auto it = moduleHooksMap.find(libFilename.data());

	if (it == moduleHooksMap.end()) return; // Module isn't in our map, we're done here

	PLOG_DEBUG << "pre_lib_unload_hooks_patches detaching hooks for: " << libFilename;
	// Iterate thru the vector of hooks associated with this module and detach them all
	for (auto& hook : it->second)
	{
			if(!hook->isGameplayScoped()) hook->detach();
	}
}

// Called by newLoadLibraries, attaches any hooks (that want to be enabled) associated with the loading module
void ModuleHookManager::postModuleLoad_UpdateHooks(std::wstring_view libPath) 
{
	// Need to get the name of the module from the libPath arguement
	std::filesystem::path path = libPath;
	auto libFilename = path.filename().generic_wstring();
    if (libFilename==L"halo2.dll") return; // MCC gameplay is the only re-entry gate.
    std::lock_guard lock(registryMutex);

	// Get a ref to the module-hooks map
	const auto& moduleHooksMap = mModuleHooksMap;

	// Is the currently loading module in our map?
	auto it = moduleHooksMap.find(libFilename.data());

	if (it == moduleHooksMap.end()) return; // Module isn't in our map, we're done here

	PLOG_DEBUG << "post_lib_load_hooks_patches attaching hooks for: " << libFilename;
	// Iterate thru the vector of hooks associated with this module and attach all the wants that have their wantToBeAttached flag set
	for (auto& hook : it->second)
	{
        if(hook->isGameplayScoped()) continue; // Gameplay owner, never loader reference churn.
		if (hook->getWantsToBeAttached())
		{
			hook->attach();
		}
		else // It wants to be disabled
		{
			hook->detach();
		}
	}
}



// For debugging/logging the below functions, logs only if the affected module is one of the ones we care about
#ifdef HCM_DEBUG

std::set<std::wstring> gameDLLNames{ L"halo1.dll", L"halo2.dll", L"halo3.dll", L"halo3odst.dll", L"haloreach.dll", L"halo4.dll", L"graphics-hook64.dll" };

#define printGameDLL(dllname) if (gameDLLNames.contains(dllname)) \
						{	\
							PLOG_INFO << wstr_to_str(dllname);	\
						}	\

#endif // HCM_DEBUG



std::wstring filenameFromFilepath(std::wstring in)
{
	if (in.contains('\\') == false) return in;

	std::wstring out = in.substr(in.find_last_of(L"/\\") + 1);
	return out;
}



// the hook-redirected functions
HMODULE ModuleHookManager::newLoadLibraryA(LPCSTR lpLibFileName) {
	auto result =mHook_LoadLibraryA.call< HMODULE, LPCSTR > (lpLibFileName);

	auto newLib = filenameFromFilepath(str_to_wstr(lpLibFileName));

#ifdef HCM_DEBUG
	printGameDLL(newLib)
#endif

	ModuleCache::addModuleToCache(newLib, result);
	postModuleLoad_UpdateHooks(newLib);

	return result;
}

HMODULE ModuleHookManager::newLoadLibraryW(LPCWSTR lpLibFileName) {
	auto result = mHook_LoadLibraryW.call<HMODULE, LPCWSTR>(lpLibFileName);

	auto newLib = filenameFromFilepath(lpLibFileName);

#ifdef HCM_DEBUG
	printGameDLL(newLib)
#endif

	ModuleCache::addModuleToCache(newLib, result);
	postModuleLoad_UpdateHooks(newLib);

	return result;
}

HMODULE ModuleHookManager::newLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
	auto result = mHook_LoadLibraryExA.call<HMODULE, LPCSTR, HANDLE, DWORD>(lpLibFileName, hFile, dwFlags);

	auto newLib = filenameFromFilepath(str_to_wstr(lpLibFileName));



#ifdef HCM_DEBUG
	printGameDLL(newLib)
#endif
;
	ModuleCache::addModuleToCache(newLib, result);
	postModuleLoad_UpdateHooks(newLib);

	return result;
}

HMODULE ModuleHookManager::newLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
	auto result = mHook_LoadLibraryExW.call<HMODULE, LPCWSTR, HANDLE, DWORD>(lpLibFileName, hFile, dwFlags);

	auto newLib = filenameFromFilepath(lpLibFileName);

#ifdef HCM_DEBUG
	printGameDLL(newLib)
#endif
	ModuleCache::addModuleToCache(newLib, result);
	postModuleLoad_UpdateHooks(newLib);

	return result;
}

BOOL ModuleHookManager::newFreeLibrary(HMODULE hLibModule) {

	wchar_t moduleFilePath[MAX_PATH];
	GetModuleFileName(hLibModule, moduleFilePath, sizeof(moduleFilePath) / sizeof(TCHAR));



	std::filesystem::path path = moduleFilePath;
	auto filename = path.filename().generic_wstring();

#ifdef HCM_DEBUG
	PLOGV << "newFreeLibrary";
	printGameDLL(filename)
#endif

	preModuleUnload_UpdateHooks(filename);
    const BOOL result=mHook_FreeLibrary.call<BOOL, HMODULE>(hLibModule);
    // FreeLibrary can merely decrement a reference. Refresh after the call.
    if(auto present=GetModuleHandleW(filename.c_str())) ModuleCache::addModuleToCache(filename,present);
    else ModuleCache::removeModuleFromCache(filename);
    return result;
}


// H2 services are created/destroyed by the same App thread that pumps this.
// Snapshot only under the registry lock. No registry/state/freeze/viewer lock
// is held while an inline hook drains an original call or a grid callback.
void ModuleHookManager::pumpH2Lifecycle() {
    std::vector<ModuleHookBase*> hooks;
    { std::lock_guard lock(registryMutex); auto it=mModuleHooksMap.find(L"halo2.dll");
      if(it!=mModuleHooksMap.end()) hooks=it->second; }
    uint64_t revision;
    bool retire;
    { std::lock_guard lock(stateMutex);
      revision=h2State.retirement();
      retire=revision!=retiredRevision || (!h2State.wantsGameplay() && h2Lease);
      if(retire) h2State.suspend(); }
    if(retire) {
        const auto now=std::chrono::steady_clock::now();
        if(!h2Retry.ready(now)) return;
        ++retirementAttempts;
        if(!retireHooks(hooks)) {
            const auto delay=h2Retry.failed(now);
            std::string reason="hook cleanup incomplete";
            for(auto* hook:hooks) if(*hook->getCleanupFailure()) {reason=hook->getCleanupFailure();break;}
            std::lock_guard lock(stateMutex);
            if(retirementStatus!=reason) PLOG_WARNING<<"[H2Transition] retirement pending: "<<reason<<"; module retained; retries back off to 2000 ms (next "<<delay<<" ms)";
            retirementStatus=reason;return;
        }
        h2Retry.clear();
        std::lock_guard lock(stateMutex);
        retirementStatus.clear();
        if(h2Lease) {
            auto lease=std::exchange(h2Lease,nullptr);
            // Bypass our FreeLibrary observer; this is our reference, not MCC's.
            const BOOL released=mHook_FreeLibrary ? mHook_FreeLibrary.call<BOOL,HMODULE>(lease) : FreeLibrary(lease);
            if(!released) { h2Lease=lease; PLOG_ERROR<<"[H2Transition] lease release failed"; return; }
            if(auto present=GetModuleHandleW(L"halo2.dll")) ModuleCache::addModuleToCache(L"halo2.dll",present);
            else ModuleCache::removeModuleFromCache(L"halo2.dll");
            PLOG_INFO<<"[H2Transition] hooks retired, callbacks drained; module_gone="<<(!GetModuleHandleW(L"halo2.dll"));
        }
        retiredRevision=revision;
    }
    { std::lock_guard lock(stateMutex);
      if(!h2State.wantsGameplay() || retiredRevision!=h2State.retirement()) return;
      if(!h2Lease) {
          if(!GetModuleHandleExW(0,L"halo2.dll",&h2Lease)) return;
          ModuleCache::addModuleToCache(L"halo2.dll",h2Lease);
          PLOG_INFO<<"[H2Transition] gameplay module lease acquired";
      }
      if(!h2State.activate(retiredRevision)) return; }
    const auto now=std::chrono::steady_clock::now();
    if(!h2Retry.ready(now)) return;
    bool pending=false;
    for(auto* hook:hooks) {
        if(!h2State.allowsEffects()) break;
        if(hook->getWantsToBeAttached()) {
            if(!hook->isHookInstalled()) hook->attach();
            if(!hook->isHookInstalled()) pending=true;
        } else if(!hook->detach()) pending=true;
    }
    if(pending) {
        h2Retry.failed(now);
        std::string reason="requested H2 hook could not attach";
        for(auto* hook:hooks) if(*hook->getCleanupFailure()) {reason=hook->getCleanupFailure();break;}
        std::lock_guard lock(stateMutex);
        if(retirementStatus!=reason) PLOG_WARNING<<"[H2Transition] hook update pending: "<<reason<<"; ownership retained; retries back off to 2000 ms";
        retirementStatus=reason;
    } else {
        h2Retry.clear();std::lock_guard lock(stateMutex);retirementStatus.clear();
    }
}
bool ModuleHookManager::retireH2() {
    if(h2State.wantsGameplay()) observeH2Gameplay(false);
    pumpH2Lifecycle();
    return h2Lease==nullptr;
}

// Scoped routes use the same disable -> quiescence -> release protocol as H2,
// but own only the explicitly scoped hooks, leaving other services unchanged.
void ModuleHookManager::observeGameplay(std::wstring_view activeModule) {
    std::lock_guard lock(stateMutex);
    for(auto& route:gameplayRoutes) route.state.observe(activeModule==route.module);
}
bool ModuleHookManager::gameplayEffectsAllowed(std::wstring_view module) {
    if(module==L"halo2.dll") return h2EffectsAllowed();
    for(auto& route:gameplayRoutes) if(module==route.module) return route.state.allowsEffects();
    return false;
}
bool ModuleHookManager::gameplayRequested(std::wstring_view module) {
    if(module==L"halo2.dll") return h2GameplayRequested();
    for(auto& route:gameplayRoutes) if(module==route.module) return route.state.wantsGameplay();
    return false;
}
void ModuleHookManager::pumpGameplayRoute(GameplayRoute& route) {
    std::vector<ModuleHookBase*> hooks;
    { std::lock_guard lock(registryMutex);
      auto it=mModuleHooksMap.find(route.module);
      if(it!=mModuleHooksMap.end()) for(auto* hook:it->second)
          if(hook->isGameplayScoped()) hooks.push_back(hook); }
    if(hooks.empty() && !route.lease) return;
    uint64_t revision;
    bool retire;
    { std::lock_guard lock(stateMutex);
      revision=route.state.retirement();
      retire=revision!=route.retiredRevision || (!route.state.wantsGameplay() && route.lease);
      if(retire) route.state.suspend(); }
    const auto now=std::chrono::steady_clock::now();
    if(!route.retry.ready(now)) return;
    auto pending=[&](const char* fallback) {
        route.retry.failed(now);
        std::string reason=fallback;
        for(auto* hook:hooks) if(*hook->getCleanupFailure()) {reason=hook->getCleanupFailure();break;}
        std::lock_guard lock(stateMutex);
        if(route.status!=reason) PLOG_WARNING<<"[GameplayRouting] module="<<route.module<<" pending: "<<reason<<"; ownership retained; retries back off to 2000 ms";
        route.status=reason;
    };
    if(retire) {
        if(!retireHooks(hooks)) {pending("hook retirement incomplete");return;}
        std::lock_guard lock(stateMutex);
        if(route.lease) {
            auto lease=route.lease;
            const BOOL released=mHook_FreeLibrary ? mHook_FreeLibrary.call<BOOL,HMODULE>(lease) : FreeLibrary(lease);
            if(!released) {pending("module lease release failed");return;}
            route.lease=nullptr;
            if(auto present=GetModuleHandleW(route.module)) ModuleCache::addModuleToCache(route.module,present);
            else ModuleCache::removeModuleFromCache(route.module);
            PLOG_INFO<<"[GameplayRouting] module="<<route.module<<" hooks retired, callbacks drained; module_gone="<<(!GetModuleHandleW(route.module));
        }
        route.retiredRevision=revision;route.retry.clear();route.status.clear();
    }
    { std::lock_guard lock(stateMutex);
      if(!route.state.wantsGameplay() || route.retiredRevision!=route.state.retirement()) return;
      if(!route.lease) {
          // An eligible title may still be loading. Absence is deferred, not an attach error.
          if(!GetModuleHandleExW(0,route.module,&route.lease)) return;
          ModuleCache::addModuleToCache(route.module,route.lease);
          PLOG_INFO<<"[GameplayRouting] module="<<route.module<<" gameplay lease acquired";
      }
      if(!route.state.activate(route.retiredRevision)) return; }
    bool incomplete=false;
    for(auto* hook:hooks) {
        if(!route.state.allowsEffects()) break;
        if(hook->getWantsToBeAttached()) {
            if(!hook->isHookInstalled()) hook->attach();
            if(!hook->isHookInstalled()) incomplete=true;
        } else if(!hook->detach()) incomplete=true;
    }
    if(incomplete) pending("requested hook could not attach");
    else { route.retry.clear();std::lock_guard lock(stateMutex);route.status.clear(); }
}
void ModuleHookManager::pumpGameplayLifecycle() {
    for(auto& route:gameplayRoutes) pumpGameplayRoute(route);
}
bool ModuleHookManager::retireGameplay() {
    { std::lock_guard lock(stateMutex);
      for(auto& route:gameplayRoutes) if(route.state.wantsGameplay()) route.state.observe(false); }
    pumpGameplayLifecycle();
    for(auto& route:gameplayRoutes) if(route.lease) return false;
    return true;
}
std::string ModuleHookManager::gameplayLifecycleStatus() {
    std::lock_guard lock(stateMutex);
    std::string status;
    for(auto& route:gameplayRoutes) if(!route.status.empty()) {
        if(!status.empty()) status+="; ";
        status+=wstr_to_str(route.module)+": "+route.status;
    }
    return status;
}
