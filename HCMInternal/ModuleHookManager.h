#pragma once
#include "ModuleHook.h"
#include "H2TransitionState.h"
#include "RetirementBackoff.h"
#include "GameplayRoute.h"



// Singleton that works with ModuleCache to manage ModuleHooks,
// attaching and detaching hooks as necessary when modules load/unload)
class ModuleHookManager
{
private:
    static inline std::recursive_mutex registryMutex;
    static inline std::recursive_mutex stateMutex;
    static inline H2TransitionState h2State;
    static inline HMODULE h2Lease=nullptr;
    static inline uint64_t retiredRevision=0;
    static inline RetirementBackoff h2Retry;
    static inline std::atomic<uint64_t> retirementAttempts{0};
    static inline std::string retirementStatus;
    static inline GameplayRoute gameplayRoutes[] = {
        GameplayRoute(L"halo1.dll"), GameplayRoute(L"halo3.dll"),
        GameplayRoute(L"halo3odst.dll"), GameplayRoute(L"haloreach.dll"), GameplayRoute(L"halo4.dll")
    };
    static void pumpGameplayRoute(GameplayRoute& route);
	friend class ModuleInlineHook;
	friend class ModuleMidHook;
	friend class ModulePatch;


	// Where we keep all the module relative hooks (key = name of module, value = list of all hooks for that module).
	static inline std::unordered_map	<std::wstring, std::vector
		<
		ModuleHookBase*
		>
	> mModuleHooksMap;


	// Hooks on module load/unload functions.
	// These are attached on ModuleHookManager construction
	// These are automagically cleaned up (detached) on ModuleHookManager destruction
	static inline safetyhook::InlineHook mHook_LoadLibraryA;
	static inline safetyhook::InlineHook mHook_LoadLibraryW;
	static inline safetyhook::InlineHook mHook_LoadLibraryExA;
	static inline safetyhook::InlineHook mHook_LoadLibraryExW;
	static inline safetyhook::InlineHook mHook_FreeLibrary;

	// So we can process the hooks we need to attach/detach of the loading/unloading library
	static HMODULE newLoadLibraryA(LPCSTR lpLibFileName);
	static HMODULE newLoadLibraryW(LPCWSTR lpLibFileName);
	static HMODULE newLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
	static HMODULE newLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
	static BOOL newFreeLibrary(HMODULE hLibModule);


	static void preModuleUnload_UpdateHooks(std::wstring_view libFilename); // called by newFreeLibrary
	static void postModuleLoad_UpdateHooks(std::wstring_view libPath); // called by newLoadLibraries

	static void detachAllHooks();

	static void addHook(const std::wstring& moduleName, ModuleHookBase* newHook)
	{
        std::lock_guard lock(registryMutex);
		// Note: [] operator creates the moduleName key if it didn't already exist in the map
		mModuleHooksMap[moduleName].emplace_back(newHook);
	}


	static void removeHook(const std::wstring& moduleName, ModuleHookBase* hookToRemove)
	{
        std::lock_guard lock(registryMutex);
		if (mModuleHooksMap.contains(moduleName))
		{
			auto position = std::ranges::find(mModuleHooksMap[moduleName], hookToRemove);

			if (position == mModuleHooksMap[moduleName].end())
				return;

			mModuleHooksMap[moduleName].erase(position);
		}
	}

public:

    static void observeGameplay(std::wstring_view activeModule);
    static bool gameplayEffectsAllowed(std::wstring_view module);
    static bool gameplayRequested(std::wstring_view module);
    static bool sharedGameplayRequested() {
        for(auto& route:gameplayRoutes) if(route.state.wantsGameplay()) return true;
        return false;
    }
    static void pumpGameplayLifecycle();
    static bool retireGameplay();
    static std::string gameplayLifecycleStatus();

    static std::recursive_mutex& h2StateMutex() { return stateMutex; }
    static void observeH2Gameplay(bool active) { std::lock_guard lock(stateMutex); h2State.observe(active); }
    static bool h2EffectsAllowed() { return h2State.allowsEffects(); }
    static bool h2GameplayRequested() { return h2State.wantsGameplay(); }
    static void pumpH2Lifecycle(); // App owner thread only; never called by loader hooks.
    static bool retireH2();
    static uint64_t h2RetirementAttempts() { return retirementAttempts.load(); }
    static std::string h2LifecycleStatus() { std::lock_guard lock(stateMutex);return retirementStatus; }

	ModuleHookManager();

	~ModuleHookManager();





};


