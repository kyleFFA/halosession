#include "../HCMInternal/pch.h"
#include "../HCMInternal/ModuleHookManager.h"
#include "../HCMInternal/ModuleCache.h"
#include "../HCMInternal/ScopedThreadSuspender.h"
#include "../HCMInternal/HookRetirement.h"
#include "../HCMInternal/HookQuiescence.h"
#include "../HCMInternal/H2TickrateLifecycle.h"
#include <cassert>

static std::atomic<bool> midEntered{false},midProceed{false};
static std::atomic<unsigned> midCppCounter{0};
static void midDetour(SafetyHookContext&) {
    ++midCppCounter;--midCppCounter; // Deliberately zero while a return into the stub is outstanding.
    midEntered=true;midEntered.notify_all();
    while(!midProceed.load()) midProceed.wait(false);
}
static ModuleInlineHook* grid=nullptr;
static std::atomic<unsigned> flights{0};
static std::atomic<bool> entered{false},proceed{false},draining{false};
static int gridDetour(int value) {
    ++flights;entered=true;entered.notify_all();
    while(!proceed.load()) proceed.wait(false);
    const int result=grid->getInlineHook().call<int>(value)+1;
    --flights;flights.notify_all();return result;
}
static auto pointer(HMODULE module,const char* symbol) {
    const auto offset=reinterpret_cast<uintptr_t>(GetProcAddress(module,symbol))-reinterpret_cast<uintptr_t>(module);
    return std::make_shared<MultilevelPointerSpecialisation::ModuleOffset>(L"halo2.dll",std::vector<int64_t>{static_cast<int64_t>(offset)});
}
int main() {
    // Production two-phase failure policy: never drain/release on a failed disable.
    struct Fake { bool canDisable=true;int prepares=0,drains=0;bool prepareDetach(){++prepares;return canDisable;}bool detach(){++drains;return true;} } a,b;
    b.canDisable=false;std::array<Fake*,2> fake{&a,&b};
    assert(!retireHooks(fake) && a.drains==0 && b.drains==0 && a.prepares==1 && b.prepares==1);
    b.canDisable=true;assert(retireHooks(fake) && a.drains==1 && b.drains==1);

    RetirementBackoff retry;auto clock=std::chrono::steady_clock::time_point{};
    for(unsigned expected:{50u,100u,200u,400u,800u,1600u,2000u,2000u}) {
        assert(retry.ready(clock));assert(retry.failed(clock)==expected);assert(!retry.ready(clock));clock+=std::chrono::milliseconds(expected);
    }
    retry.clear();assert(retry.ready(clock));
    ModuleCache::initialize();
    ModuleHookManager manager; // Hooks only this test process, never MCC.
    HMODULE module=LoadLibraryW(L"out\\transition-fixtures\\halo2.dll");assert(module);
    auto* site=reinterpret_cast<byte*>(GetProcAddress(module,"patchSite"));assert(site);
    const std::vector<byte> stock(site,site+6),patched(6,0x90);
    auto patch=ModulePatch::make(L"halo2.dll",pointer(module,"patchSite"),patched);
    patch->setFeatureName("fixture/Pause");
    using O=ModulePatch::Outcome;
    assert(!patch->isHookInstalled() && patch->detach() && patch->lastOutcome()==O::NeverApplied);
    patch->setWantsToBeAttached(true);
    ModuleHookManager::observeH2Gameplay(true);ModuleHookManager::pumpH2Lifecycle();
    assert(patch->isHookInstalled() && patch->lastOutcome()==O::Applied);
    assert(patch->detach() && !patch->isHookInstalled() && patch->lastOutcome()==O::Restored);
    assert(patch->detach() && patch->lastOutcome()==O::AlreadyRestored);
    ModuleHookManager::pumpH2Lifecycle();assert(patch->isHookInstalled());
    site[0]=0xEE;
    assert(!patch->isHookInstalled() && !patch->detach() && patch->lastOutcome()==O::Conflict && site[0]==0xEE);
    ModuleHookManager::observeH2Gameplay(false);ModuleHookManager::pumpH2Lifecycle();
    assert(FreeLibrary(module)); // The conflict keeps our ownership lease alive.
    assert(GetModuleHandleW(L"halo2.dll") && ModuleCache::isModuleInCache(L"halo2.dll"));
    memcpy(site,patched.data(),6);Sleep(60);ModuleHookManager::pumpH2Lifecycle();
    assert(!GetModuleHandleW(L"halo2.dll") && !ModuleCache::isModuleInCache(L"halo2.dll"));
    assert(!patch->isHookInstalled() && patch->detach() && patch->lastOutcome()==O::ModuleAbsent);

    // Fresh generation at the same or another base must never inherit originals.
    const auto oldGeneration=ModuleCache::generation(L"halo2.dll");
    module=LoadLibraryW(L"out\\transition-fixtures\\halo2.dll");assert(module);
    site=reinterpret_cast<byte*>(GetProcAddress(module,"patchSite"));
    assert(ModuleCache::generation(L"halo2.dll")!=oldGeneration);
    assert(patch->getWantsToBeAttached() && !patch->isHookInstalled() && memcmp(site,stock.data(),6)==0);
    // Loader reference churn alone cannot rearm H2 during a menu/title transition.
    auto ref=LoadLibraryW(L"out\\transition-fixtures\\halo2.dll");assert(ref==module);
    assert(FreeLibrary(ref));assert(!patch->isHookInstalled() && ModuleCache::isModuleInCache(L"halo2.dll"));
    auto* second=reinterpret_cast<byte*>(GetProcAddress(module,"secondSite"));
    const std::vector<byte> secondStock(second,second+6);memcpy(second,patched.data(),6);
    auto foreign=ModulePatch::make(L"halo2.dll",pointer(module,"secondSite"),patched);
    foreign->setFeatureName("fixture/unowned");foreign->setWantsToBeAttached(true);
    ModuleHookManager::observeH2Gameplay(true);ModuleHookManager::pumpH2Lifecycle();
    assert(patch->isHookInstalled());assert(foreign->lastOutcome()==O::UnownedPatch && !foreign->isHookInstalled());
    assert(foreign->detach() && foreign->lastOutcome()==O::UnownedPatch && memcmp(second,patched.data(),6)==0);
    foreign->setWantsToBeAttached(false);memcpy(second,secondStock.data(),6);Sleep(60);

    // Preserve H2 selections through another title; never alter that title's wish.
    HMODULE h3=LoadLibraryW(L"out\\transition-fixtures\\halo3.dll");assert(h3);
    auto h3Address=reinterpret_cast<uintptr_t>(GetProcAddress(h3,"patchSite"));
    auto h3Pointer=std::make_shared<MultilevelPointerSpecialisation::ModuleOffset>(L"halo3.dll",std::vector<int64_t>{static_cast<int64_t>(h3Address-reinterpret_cast<uintptr_t>(h3))});
    auto h3Patch=ModulePatch::make(L"halo3.dll",h3Pointer,patched);h3Patch->setWantsToBeAttached(true);
    ModuleHookManager::observeH2Gameplay(false);ModuleHookManager::pumpH2Lifecycle();
    assert(!patch->isHookInstalled() && patch->getWantsToBeAttached() && h3Patch->isHookInstalled());
    ModuleHookManager::observeH2Gameplay(true);ModuleHookManager::pumpH2Lifecycle();
    assert(patch->isHookInstalled() && h3Patch->isHookInstalled());
    h3Patch->setWantsToBeAttached(false);assert(!h3Patch->isHookInstalled());
    h3Patch.reset();assert(FreeLibrary(h3));

    // A zero C++ callback counter does not permit freeing the mid-hook stub.
    auto mid=ModuleMidHook::make(L"halo2.dll",pointer(module,"fixtureOriginal"),&midDetour);
    mid->setWantsToBeAttached(true);ModuleHookManager::pumpH2Lifecycle();assert(mid->isHookInstalled());
    auto midFn=reinterpret_cast<int(*)(int)>(GetProcAddress(module,"fixtureOriginal"));
    int midResult=0;std::thread midCall([&]{midResult=midFn(10);});
    while(!midEntered.load()) midEntered.wait(false);
    assert(midCppCounter==0);assert(FreeLibrary(module));
    ModuleHookManager::observeH2Gameplay(false);ModuleHookManager::pumpH2Lifecycle();
    assert(GetModuleHandleW(L"halo2.dll") && !mid->isHookInstalled());
    midProceed=true;midProceed.notify_all();midCall.join();assert(midResult==27);
    // Unreadable thread state keeps ownership and must not scan at pump rate.
    const auto realQuery=hook_lifetime::query;hook_lifetime::query=nullptr;
    Sleep(60);ModuleHookManager::pumpH2Lifecycle();
    const auto attempts=ModuleHookManager::h2RetirementAttempts();
    for(int i=0;i<100;++i) ModuleHookManager::pumpH2Lifecycle();
    assert(ModuleHookManager::h2RetirementAttempts()==attempts && GetModuleHandleW(L"halo2.dll"));
    assert(ModuleHookManager::h2LifecycleStatus().find("unreadable")!=std::string::npos);
    hook_lifetime::query=realQuery;Sleep(120);ModuleHookManager::pumpH2Lifecycle();assert(!GetModuleHandleW(L"halo2.dll"));mid.reset();
    module=LoadLibraryW(L"out\\transition-fixtures\\halo2.dll");assert(module);
    ModuleHookManager::observeH2Gameplay(true);ModuleHookManager::pumpH2Lifecycle();

    // A grid callback paused BEFORE its original call must retain both the
    // trampoline and owning module until the original executes and returns.
    auto inlineHook=ModuleInlineHook::make(L"halo2.dll",pointer(module,"fixtureOriginal"),reinterpret_cast<void*>(&gridDetour));
    grid=inlineHook.get();
    inlineHook->setCallbackDrain([] {
        if(flights.load()) {draining=true;draining.notify_all();}
        return flights.load()==0;
    });
    inlineHook->setWantsToBeAttached(true);ModuleHookManager::pumpH2Lifecycle();assert(inlineHook->isHookInstalled());
    auto fn=reinterpret_cast<int(*)(int)>(GetProcAddress(module,"fixtureOriginal"));
    int result=0;std::thread call([&]{result=fn(10);});
    while(!entered.load()) entered.wait(false);
    assert(FreeLibrary(module)); // Active lease keeps the function valid.
    ModuleHookManager::observeH2Gameplay(false);
    std::thread release([] {
        while(!draining.load()) draining.wait(false);
        assert(GetModuleHandleW(L"halo2.dll"));
        assert(ModuleHookManager::h2StateMutex().try_lock());ModuleHookManager::h2StateMutex().unlock();
        assert(hcmThreadSuspensionMutex().try_lock());hcmThreadSuspensionMutex().unlock();
        proceed=true;proceed.notify_all();
    });
    ModuleHookManager::pumpH2Lifecycle();call.join();release.join();Sleep(60);ModuleHookManager::pumpH2Lifecycle();
    assert(result==28 && flights==0 && !inlineHook->isHookInstalled() && !GetModuleHandleW(L"halo2.dll"));
    inlineHook.reset();
    // An original call holds SafetyHook's call mutex. Retirement must report
    // pending, not wait for that mutex while owning the shared freeze lock.
    module=LoadLibraryW(L"out\\transition-fixtures\\halo2.dll");assert(module);
    auto* originalEntered=reinterpret_cast<volatile LONG*>(GetProcAddress(module,"originalEntered"));
    auto* originalProceed=reinterpret_cast<volatile LONG*>(GetProcAddress(module,"originalProceed"));
    auto blocking=ModuleInlineHook::make(L"halo2.dll",pointer(module,"fixtureBlockingOriginal"),reinterpret_cast<void*>(&gridDetour));
    grid=blocking.get();blocking->setCallbackDrain([]{return flights.load()==0;});
    blocking->setWantsToBeAttached(true);ModuleHookManager::observeH2Gameplay(true);ModuleHookManager::pumpH2Lifecycle();
    auto blockingFn=reinterpret_cast<int(*)(int)>(GetProcAddress(module,"fixtureBlockingOriginal"));
    int blockedResult=0;std::thread originalCall([&]{blockedResult=blockingFn(10);});
    while(!InterlockedCompareExchange(originalEntered,0,0)) Sleep(1);
    assert(FreeLibrary(module));blocking->setWantsToBeAttached(false);
    const auto start=std::chrono::steady_clock::now();ModuleHookManager::pumpH2Lifecycle();
    assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(1));
    assert(GetModuleHandleW(L"halo2.dll") && ModuleHookManager::h2LifecycleStatus().find("original call")!=std::string::npos);
    const auto pendingReason=ModuleHookManager::h2LifecycleStatus();
    for(int i=0;i<100;++i) ModuleHookManager::pumpH2Lifecycle();
    assert(ModuleHookManager::h2LifecycleStatus()==pendingReason && GetModuleHandleW(L"halo2.dll"));
    InterlockedExchange(originalProceed,1);originalCall.join();assert(blockedResult==28);
    ModuleHookManager::observeH2Gameplay(false);
    Sleep(60);ModuleHookManager::pumpH2Lifecycle();assert(!GetModuleHandleW(L"halo2.dll"));
    blocking.reset();foreign.reset();patch.reset();

    // Production scoped hooks: absence and mere loading never arm another title.
    struct Errors : plog::IAppender { unsigned count=0;void write(const plog::Record& rec) override {
        if(rec.getSeverity()<=plog::error) ++count;
    }};
    static Errors errors;
    plog::init(plog::verbose,&errors);
    const wchar_t* names[]={L"halo3.dll",L"halo3odst.dll",L"haloreach.dll"};
    std::array<HMODULE,3> modules{};
    std::array<std::unique_ptr<ModuleMidHook>,3> scoped;
    HMODULE probe=LoadLibraryW(L"out\\transition-fixtures\\halo3.dll");assert(probe);
    const auto rva=reinterpret_cast<uintptr_t>(GetProcAddress(probe,"fixtureOriginal"))-reinterpret_cast<uintptr_t>(probe);
    assert(FreeLibrary(probe));
    for(int i=0;i<3;++i) {
        auto ptr=std::make_shared<MultilevelPointerSpecialisation::ModuleOffset>(names[i],std::vector<int64_t>{static_cast<int64_t>(rva)});
        scoped[i]=ModuleMidHook::make(names[i],ptr,&midDetour,false,true);
        scoped[i]->setWantsToBeAttached(i!=2);
    }
    ModuleHookManager::observeGameplay(L"halo3.dll");ModuleHookManager::pumpGameplayLifecycle();
    for(auto& hook:scoped) assert(!hook->isHookInstalled());
    assert(errors.count==0 && ModuleHookManager::gameplayLifecycleStatus().empty());
    ModuleHookManager::observeGameplay(L"");
    for(int i=0;i<3;++i) {
        modules[i]=LoadLibraryW((std::wstring(L"out\\transition-fixtures\\")+names[i]).c_str());assert(modules[i]);
        assert(!scoped[i]->isHookInstalled());
    }
    // All three are mapped; only actual gameplay activates the selected route.
    for(int active:{0,1,2,0}) {
        ModuleHookManager::observeGameplay(names[active]);ModuleHookManager::pumpGameplayLifecycle();
        for(int i=0;i<3;++i) {
            assert(scoped[i]->getWantsToBeAttached()==(i!=2));
            assert(scoped[i]->isHookInstalled()==(i==active && i!=2));
        }
    }
    // Loader reference churn must not detach an active scoped hook.
    auto extra=LoadLibraryW(L"out\\transition-fixtures\\halo3.dll");assert(extra);
    assert(FreeLibrary(extra) && scoped[0]->isHookInstalled());
    // Toggle while inactive is retained; activation and active disable are owned by pump.
    scoped[2]->setWantsToBeAttached(true);assert(!scoped[2]->isHookInstalled());
    ModuleHookManager::observeGameplay(names[2]);ModuleHookManager::pumpGameplayLifecycle();assert(scoped[2]->isHookInstalled());
    scoped[2]->setWantsToBeAttached(false);ModuleHookManager::pumpGameplayLifecycle();assert(!scoped[2]->isHookInstalled());
    ModuleHookManager::observeGameplay(names[0]);ModuleHookManager::pumpGameplayLifecycle();
    midEntered=false;midProceed=false;
    auto routeFn=reinterpret_cast<int(*)(int)>(GetProcAddress(modules[0],"fixtureOriginal"));
    int routeResult=0;std::thread routeCall([&]{routeResult=routeFn(10);});
    while(!midEntered.load()) midEntered.wait(false);
    ModuleHookManager::observeGameplay(L"");assert(!ModuleHookManager::gameplayEffectsAllowed(names[0]));
    assert(FreeLibrary(modules[0]));modules[0]=nullptr;
    ModuleHookManager::pumpGameplayLifecycle();
    assert(GetModuleHandleW(names[0]) && !scoped[0]->isHookInstalled());
    assert(!ModuleHookManager::gameplayLifecycleStatus().empty());
    midProceed=true;midProceed.notify_all();routeCall.join();assert(routeResult==27);
    const auto routeQuery=hook_lifetime::query;hook_lifetime::query=nullptr;
    Sleep(60);ModuleHookManager::pumpGameplayLifecycle();
    for(int n=0;n<100;++n) ModuleHookManager::pumpGameplayLifecycle();
    assert(GetModuleHandleW(names[0]) && ModuleHookManager::gameplayLifecycleStatus().find("unreadable")!=std::string::npos);
    hook_lifetime::query=routeQuery;Sleep(120);ModuleHookManager::pumpGameplayLifecycle();
    assert(!GetModuleHandleW(names[0]) && ModuleHookManager::gameplayLifecycleStatus().empty());
    assert(scoped[0]->getWantsToBeAttached());
    modules[0]=LoadLibraryW(L"out\\transition-fixtures\\halo3.dll");assert(modules[0] && !scoped[0]->isHookInstalled());
    ModuleHookManager::observeGameplay(names[0]);ModuleHookManager::pumpGameplayLifecycle();assert(scoped[0]->isHookInstalled());
    assert(ModuleHookManager::retireGameplay());
    for(int i=0;i<3;++i) {scoped[i].reset();assert(FreeLibrary(modules[i]));}
    assert(errors.count==0);
    std::cout<<"PASS: scoped H3/ODST/Reach absent and loaded-inactive gating, selected-state retention/reentry, loader churn, toggles, blocked callback retirement, unreadable recovery, and shutdown; zero routing errors\n";
    // Modes/selections survive suspension; target/scalar semantics stay intact.
    using namespace h2timing;
    for(auto mode:{Mode::Native30,Mode::Native60,Mode::Stock}) {
        H2TransitionState state;state.observe(true);assert(state.activate(0));
        const auto selected=mode;state.observe(false);assert(!state.allowsEffects());
        state.observe(true);assert(!state.activate(0));assert(state.activate(state.retirement()));
        assert(mode==selected && target(mode).rate==(mode==Mode::Native30?30:60));
        assert(target(mode).scalar==(mode==Mode::Native60?60:30));
    }
    std::cout<<"PASS: real ModulePatch outcomes/readback, module generations/reference churn, H2-H3-H2 wishes, failed-disable retention, blocked mid-hook stub return, bounded unreadable-thread retries/recovery, and active grid callback/original-call drain\n";
}
