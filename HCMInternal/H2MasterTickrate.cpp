#include "pch.h"
#include "ModuleHookManager.h"
#include "H2MasterTickrate.h"
#include "H2TickrateLifecycle.h"
#include "H2TimingPair.h"
#include "H2CV_Halo2.h"
#include "H2CV_Mem.h"
#include "SettingsStateAndEvents.h"
#include "IMCCStateHook.h"
#include "IMessagesGUI.h"
#include "PointerDataStore.h"
#include "MultilevelPointer.h"
#include "GameTickEventHook.h"
#include "IMakeOrGetCheat.h"
#include "ScopedThreadSuspender.h"

namespace
{
    std::mutex statusMutex;
    std::string timingStatus = "Stock timing. Enable control to select 30 or 60 tick.";
    void setTimingStatus(std::string s) { std::lock_guard lock(statusMutex); timingStatus = std::move(s); }

    // Native scalar mechanism from Harc's MasterTickrate. The collision builder
    // reads this float only when constructing a body; changing it also requires
    // the imported game-thread body rebuild below. Never overwrite another mod.
    class TickrateScalarPatcher
    {
        static constexpr uintptr_t siteRva = 0x70DBFA;
        static constexpr uint8_t stock[8] = {0xF3,0x0F,0x59,0x35,0x9E,0x4E,0x52,0x00};
        uintptr_t base = 0;
        float* scalar = nullptr;
        uint8_t owned[8]{};
        bool applied = false;

        static bool matches(uintptr_t p, const uint8_t* expected)
        {
            uint8_t bytes[8];
            return mem::copy(bytes,p,8) && memcmp(bytes,expected,8)==0;
        }
        static void* allocNear(uintptr_t target)
        {
            SYSTEM_INFO info; GetSystemInfo(&info);
            const uintptr_t gran=info.dwAllocationGranularity;
            const uintptr_t lo=target>0x70000000ULL ? target-0x70000000ULL : 0x10000ULL;
            const uintptr_t hi=target+0x70000000ULL;
            const uintptr_t start=target&~(gran-1);
            for (uintptr_t p=start;p>lo;p-=gran)
                if (auto m=VirtualAlloc((void*)p,0x1000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)) return m;
            for (uintptr_t p=start+gran;p<hi;p+=gran)
                if (auto m=VirtualAlloc((void*)p,0x1000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)) return m;
            return nullptr;
        }
        static bool replace(uintptr_t at, const uint8_t* expected, const uint8_t* replacement)
        {
            // No heap allocation/free while other threads are suspended.
            ScopedThreadSuspender suspend;
            if (!suspend.snapshotOk() || suspend.missedCount()!=0 || !matches(at,expected)) return false;
            DWORD protect=0, ignored=0;
            if (!VirtualProtect((void*)at,8,PAGE_EXECUTE_READWRITE,&protect)) return false;
            memcpy((void*)(at+4),replacement+4,4);
            VirtualProtect((void*)at,8,protect,&ignored);
            FlushInstructionCache(GetCurrentProcess(),(void*)at,8);
            return true;
        }
    public:
        bool isApplied() const { return applied; }
        bool stillOwned() const { return applied && h2::base()==base && matches(base+siteRva,owned); }
        uintptr_t moduleBase() const { return base; }
        void apply()
        {
            if (stillOwned()) return;
            if (applied) {
                // A reloaded module has its stock instruction again. The old
                // module no longer references our allocation.
                if (h2::base()==base && !matches(base+siteRva,stock))
                    throw HCMRuntimeException("H2 scalar patch changed ownership; refusing to replace it.");
                if (scalar) VirtualFree(scalar,0,MEM_RELEASE);
                scalar=nullptr; applied=false;
            }
            base=h2::base();
            if (!base || !h2::buildOk() || !matches(base+siteRva,stock))
                throw HCMRuntimeException("H2 tickrate requires the unmodified 1.3528 collision scalar instruction.");
            scalar=(float*)allocNear(base);
            if (!scalar) throw HCMRuntimeException("H2 tickrate: could not allocate the native collision scalar.");
            *scalar=30.f;
            const int64_t disp=(int64_t)(uintptr_t)scalar-(int64_t)(base+siteRva+8);
            if (disp<INT32_MIN || disp>INT32_MAX) {
                VirtualFree(scalar,0,MEM_RELEASE); scalar=nullptr;
                throw HCMRuntimeException("H2 tickrate: collision scalar is out of range.");
            }
            memcpy(owned,stock,8);
            const int32_t rel=(int32_t)disp; memcpy(owned+4,&rel,4);
            if (!replace(base+siteRva,stock,owned)) {
                VirtualFree(scalar,0,MEM_RELEASE); scalar=nullptr;
                throw HCMRuntimeException("H2 tickrate: scalar instruction changed before patching.");
            }
            applied=true;
        }
        void set(float value) { if (scalar) *scalar=value; }
        bool readEffective(float& value) const {
            const auto b=h2::base();
            if(!b)return false;
            uint8_t bytes[8]{};
            if(!mem::copy(bytes,b+siteRva,8))return false;
            if(memcmp(bytes,stock,8)!=0 && !(applied && b==base && memcmp(bytes,owned,8)==0))return false;
            int32_t displacement=0;memcpy(&displacement,bytes+4,4);
            return mem::rd(b+siteRva+8+displacement,value) && std::isfinite(value);
        }
        bool revert()
        {
            if (!applied) return true;
            if (h2::base()==base && !matches(base+siteRva,stock) && !replace(base+siteRva,owned,stock))
                return false; // Keep the allocation if executable code might still refer to it.
            applied=false;
            if (scalar) VirtualFree(scalar,0,MEM_RELEASE);
            scalar=nullptr;
            return true;
        }
        ~TickrateScalarPatcher() { if (!revert()) PLOG_ERROR << "H2 scalar ownership changed; allocation retained."; }
    };

    // POD-only SEH boundary. Verify that rate/dt are a real timing pair before
    // either write; all calls come from the H2 game tick, after level-state checks.
    bool readPair(uintptr_t r,uintptr_t d,h2timing::Pair& out) {
        __try {out={*(int16_t*)r,*(float*)d};return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool storePair(uintptr_t r,uintptr_t d,const h2timing::Pair& value) {
        __try {*(int16_t*)r=value.rate;*(float*)d=value.dt;return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    h2timing::WriteResult writeTiming(uintptr_t r,uintptr_t d,int16_t rate) {
        if(d!=r+2)return h2timing::WriteResult::Refused;
        return h2timing::writePair(rate,[&](h2timing::Pair& p){return readPair(r,d,p);},
            [&](const h2timing::Pair& p){return storePair(r,d,p);});
    }

}

class H2MasterTickrate::Impl
{
    using Mode=h2timing::Mode;
    std::weak_ptr<SettingsStateAndEvents> settingsWeak;
    std::weak_ptr<IMCCStateHook> stateWeak;
    std::weak_ptr<IMessagesGUI> messagesWeak;
    std::shared_ptr<MultilevelPointer> ratePointer, dtPointer;
    std::shared_ptr<ObservedEvent<GameTickEvent>> tickEvent;
    std::unique_ptr<ScopedCallback<GameTickEvent>> tickCallback;
    std::unique_ptr<ScopedCallback<ToggleEvent>> enabledCallback, rateCallback;
    std::unique_ptr<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>> stateCallback;
    struct Life { std::mutex mutex; bool closing=false; };
    std::shared_ptr<Life> life=std::make_shared<Life>();
    std::atomic<Mode> requested{Mode::Stock};
    std::atomic<uint64_t> revision{0};
    std::atomic<uint64_t> committed{0};
    TickrateScalarPatcher scalar;
    float bodyScalar=30.f;
    std::atomic<bool> touched{false};
    bool rebuildFault=false;
    bool inactiveSinceFault=false;
    bool shuttingDown=false;
    bool stockRestored=true;
    bool wasH2=false;
    uint64_t callbackCount=0;
    uint32_t lastTick=0;

#include "H2TickrateBody.inl"
    bool bodyTableEmpty() {
        const auto base=h2::base(); if(!base)return true;
        uintptr_t table=0;uint32_t count=0;
        if(!mem::rd(base+kObjectHeaderPtrRVA,table))return false;
        return !table || (mem::rd(table+kHeaderCountOffset,count) && count==0);
    }

    void queue()
    {
        auto s=settingsWeak.lock();
        if (!s || shuttingDown) return;
        const Mode mode=s->h2NativeTickrateEnabled->GetValue() ?
            (s->h2NativeTick30->GetValue() ? Mode::Native30 : Mode::Native60) : Mode::Stock;
        requested=mode;
        const auto rev=++revision;
        const auto target=h2timing::target(mode);
        PLOG_INFO << std::format("[H2Timing] requested mode={} request={} enabled={} select30={} target_rate={} target_scalar={} callbacks={} last_tick={}",
            h2timing::name(mode),rev,s->h2NativeTickrateEnabled->GetValue(),s->h2NativeTick30->GetValue(),target.rate,target.scalar,callbackCount,lastTick);
        setTimingStatus(std::format("Pending {}: close the menu and resume Halo 2 gameplay. No applied confirmation yet.",h2timing::name(mode)));
    }
    void onTick(uint32_t tick)
    {
        lastTick=tick;
        if(++callbackCount==1)PLOG_INFO << std::format("[H2Timing] tick callback active tick={} hook=halo2.dll+0x7067F0 (campaign/original H2 multiplayer)",tick);
        const uint64_t rev=revision.load();
        if (rev==committed.load() || rebuildFault) return;
        const auto mcc=stateWeak.lock();
        if (!ModuleHookManager::h2EffectsAllowed() || !mcc || !mcc->isGameCurrentlyPlaying(GameState::Value::Halo2)) return;
        const Mode mode=requested.load();
        if (!touched && mode==Mode::Stock) {
            committed=rev;
            PLOG_INFO << std::format("[H2Timing] control off request={} tick={}; no timing writes made by this service",rev,tick);
            setTimingStatus("Native control off; this service has made no timing writes.");return;
        }
        uintptr_t rateAt=0,dtAt=0;
        int rebuildCalls=0;
        try {
            const auto target=h2timing::target(mode);
            if (!h2::base() || !h2::buildOk() || !ratePointer->resolve(&rateAt) || !dtPointer->resolve(&dtAt))
                throw HCMRuntimeException("H2 timing data is unavailable for this build/level.");
            h2timing::Pair before{};
            const bool beforeReadable=readPair(rateAt,dtAt,before);
            PLOG_INFO << std::format("[H2Timing] applying mode={} request={} tick={} timer_readable={} previous_rate={} previous_dt={} body_scalar={}",
                h2timing::name(mode),rev,tick,beforeReadable,before.rate,before.dt,bodyScalar);
            // When a module is replaced, newly constructed bodies use stock 30.
            if (scalar.isApplied() && !scalar.stillOwned()) bodyScalar=30.f;
            const bool rebuild=h2timing::rebuildRequired(bodyScalar,mode);
            if (rebuild) {
                uint8_t a[6], b[6];
                if (!mem::copy(a,h2::base()+kRecreateCollisionRVA,6) || memcmp(a,kRecreateProbe,6) ||
                    !mem::copy(b,h2::base()+kGetObjectRVA,6) || memcmp(b,kGetObjectProbe,6))
                    throw HCMRuntimeException("H2 collision rebuild function bytes differ; timing change refused.");
            }
            scalar.apply();
            stockRestored=false;
            touched=true;
            const auto timingResult=writeTiming(rateAt,dtAt,target.rate);
            if(timingResult!=h2timing::WriteResult::Applied) {
                PLOG_ERROR << std::format("[H2Timing] timer transaction mode={} request={} result={}",h2timing::name(mode),rev,
                    timingResult==h2timing::WriteResult::Refused ? "Refused" : timingResult==h2timing::WriteResult::RolledBack ? "RolledBack (readback verified)" : "Uncertain");
                if(timingResult==h2timing::WriteResult::Uncertain) {
                    rebuildFault=true;inactiveSinceFault=false;
                    throw HCMRuntimeException("H2 timer write and rollback failed. Leave/restart the level or quit MCC; resuming alone cannot recover this state.");
                }
                throw HCMRuntimeException("H2 timing write refused or rolled back; stock timing is not yet confirmed.");
            }
            scalar.set(target.scalar);
            float verifiedScalar=0.f;
            if(!scalar.readEffective(verifiedScalar) || verifiedScalar!=target.scalar)
                throw HCMRuntimeException("H2 collision scalar readback failed; timing application is unconfirmed.");
            if (rebuild) {
                const auto base=scalar.moduleBase();
                const int count=rebuildAllCollisionSEH(base,(GetObjectFn)(base+kGetObjectRVA),(RecreateCollisionFn)(base+kRecreateCollisionRVA));
                if (count<0) { rebuildFault=true;inactiveSinceFault=false; bodyScalar=std::numeric_limits<float>::quiet_NaN(); throw HCMRuntimeException("H2 collision rebuild was interrupted. Restart the level before further tickrate changes."); }
                rebuildCalls=count;
            }
            bodyScalar=target.scalar;
            if (mode==Mode::Stock && !scalar.revert())
                throw HCMRuntimeException("H2 scalar ownership changed; could not restore its stock instruction.");
            h2timing::Pair verified{};
            if(!readPair(rateAt,dtAt,verified) || !h2timing::matches(verified,target.rate) ||
                !scalar.readEffective(verifiedScalar) || verifiedScalar!=target.scalar) {
                rebuildFault=true;inactiveSinceFault=false;
                throw HCMRuntimeException("H2 final timer/scalar readback failed. Leave/restart the level or quit MCC before further timing changes.");
            }
            stockRestored=mode==Mode::Stock;
            committed=rev;
            PLOG_INFO << std::format("[H2Timing] applied mode={} request={} tick={} rate={} dt={} scalar={} body_scalar={} rebuild_calls={} stock_restored={}",
                h2timing::name(mode),rev,tick,verified.rate,verified.dt,verifiedScalar,bodyScalar,rebuildCalls,stockRestored);
            const auto text=mode==Mode::Stock ? std::string("Stock restored: 60 Hz, original collision scaling.") :
                std::format("Native {} tick active; collision scaling matched.",target.rate);
            setTimingStatus(text);
            if (auto m=messagesWeak.lock()) m->addMessage(text);
        } catch (HCMRuntimeException& ex) {
            // No repeated calls into a failing body rebuild. A new level/request
            // is required; report the exact failure instead of claiming success.
            committed=rev;
            setTimingStatus(ex.what());
            h2timing::Pair observed{};float observedScalar=0.f;
            const bool timerReadable=rateAt && dtAt && readPair(rateAt,dtAt,observed);
            const bool scalarReadable=scalar.readEffective(observedScalar);
            PLOG_ERROR << std::format("[H2Timing] failed mode={} request={} tick={} reason={} timer_readable={} rate={} dt={} scalar_readable={} scalar={} body_scalar={} rebuild_calls={} stock_restored={} restart_required={}",
                h2timing::name(mode),rev,tick,ex.what(),timerReadable,observed.rate,observed.dt,scalarReadable,observedScalar,bodyScalar,rebuildCalls,stockRestored,rebuildFault);
            if (auto m=messagesWeak.lock()) m->addMessage(ex.what());
        }
    }
public:
    static inline Impl* current=nullptr;
    bool beginShutdown() {
        std::lock_guard lock(life->mutex);
        shuttingDown=true; requested=Mode::Stock;
        if(auto s=settingsWeak.lock())s->h2NativeDisconnecting=true;
        PLOG_INFO << std::format("[H2Timing] disconnect requested touched={} stock_restored={} body_scalar={} callbacks={} last_tick={}",
            touched.load(),stockRestored,bodyScalar,callbackCount,lastTick);
        if(!touched || stockRestored)return false;
        ++revision;
        setTimingStatus(rebuildFault ? "Disconnecting after a failed H2 rebuild/write: leave or restart the level, or quit MCC. Resume alone cannot restore this state." : "Disconnecting: resume Halo 2 gameplay or load a fresh H2 level to restore stock timing. Quit MCC if the level is unavailable.");
        return true;
    }
    bool shutdownReady() {
        std::lock_guard lock(life->mutex);
        if(stockRestored || !touched || !h2::base()) {
            if(!stockRestored)PLOG_INFO << "[H2Timing] disconnect ready: H2 module gone or no modified state";
            stockRestored=true;return true;
        }
        // The module may stay mapped after its level is destroyed. Confirm an
        // empty body table AND the stock timer; unreadable live data is unknown.
        if(!scalar.isApplied() && bodyTableEmpty()) {
            uintptr_t r=0,d=0;h2timing::Pair pair{};
            if(ratePointer->resolve(&r) && dtPointer->resolve(&d) && readPair(r,d,pair) &&
                pair.rate==60 && fabsf(pair.dt-1.f/60)<1e-6f) {
                PLOG_INFO << std::format("[H2Timing] disconnect ready: body table empty; stock instruction restored; rate={} dt={}",pair.rate,pair.dt);
                stockRestored=true;return true;
            }
        }
        return false;
    }
    Impl(GameState game, IDIContainer& dicon)
        : settingsWeak(dicon.Resolve<SettingsStateAndEvents>()), stateWeak(dicon.Resolve<IMCCStateHook>()), messagesWeak(dicon.Resolve<IMessagesGUI>())
    {
        if (game!=GameState(GameState::Value::Halo2)) throw HCMInitException("H2 native tickrate only supports original Halo 2.");
        const auto ptr=dicon.Resolve<PointerDataStore>().lock();
        ratePointer=ptr->getData<std::shared_ptr<MultilevelPointer>>("masterTickrate",game);
        dtPointer=ptr->getData<std::shared_ptr<MultilevelPointer>>("masterTickrateDt",game);
        tickEvent=resolveDependentCheat(GameTickEventHook)->getGameTickEvent();
        tickCallback=tickEvent->subscribe([this,guard=life](uint32_t tick) { std::lock_guard lock(guard->mutex); if(!guard->closing) onTick(tick); });
        PLOG_INFO << "[H2Timing] subscribed to original H2 simulation ticks; no timing writes until an enabled request";
        wasH2=stateWeak.lock()->getCurrentMCCState().currentGameState==GameState(GameState::Value::Halo2);
        current=this;
        auto s=settingsWeak.lock();
        enabledCallback=std::make_unique<ScopedCallback<ToggleEvent>>(s->h2NativeTickrateEnabled->valueChangedEvent,
            [this,guard=life](bool&) { std::lock_guard lock(guard->mutex); if(!guard->closing) queue(); });
        rateCallback=std::make_unique<ScopedCallback<ToggleEvent>>(s->h2NativeTick30->valueChangedEvent,
            [this,guard=life](bool&) { std::lock_guard lock(guard->mutex); if(!guard->closing) queue(); });
        stateCallback=std::make_unique<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>>(stateWeak.lock()->getMCCStateChangedEvent(),
            [this,guard=life](const MCCState& state) {
                std::lock_guard lock(guard->mutex);
                if(guard->closing) return;
                const bool active=state.currentGameState==GameState(GameState::Value::Halo2) && state.currentPlayState==PlayState::Ingame;
                const bool leavingTitle=state.currentGameState!=GameState(GameState::Value::Halo2) || state.currentPlayState==PlayState::MainMenu;
                if (leavingTitle && touched && wasH2) {
                    // The old title is no longer simulating. Restore its clock
                    // and scalar, and mark live bodies for repair on any return.
                    // Never call its collision rebuild from the state thread.
                    bool timerRestored=!h2::base();
                    if (h2::base() && h2::buildOk()) {
                        uintptr_t r=0,d=0;
                        if (ratePointer->resolve(&r) && dtPointer->resolve(&d)) {
                            ScopedThreadSuspender pause;
                            timerRestored=pause.snapshotOk() && pause.missedCount()==0 && writeTiming(r,d,60)==h2timing::WriteResult::Applied;
                        }
                    }
                    const bool scalarRestored=scalar.revert();
                    stockRestored=h2timing::canRelease(timerRestored,scalarRestored,bodyScalar==30.f || bodyTableEmpty(),!h2::base());
                    PLOG_INFO << std::format("[H2Timing] title exit restore timer_verified={} scalar_instruction_restored={} body_scalar={} stock_restored={} module_gone={}",
                        timerRestored,scalarRestored,bodyScalar,stockRestored,!h2::base());
                    if (!stockRestored) setTimingStatus("H2 stock restoration is unconfirmed. Load a fresh H2 level or quit MCC before disconnecting.");
                    if (bodyScalar!=30.f) bodyScalar=std::numeric_limits<float>::quiet_NaN();
                }
                if(!active && rebuildFault)inactiveSinceFault=true;
                if(active && (touched || requested.load()!=Mode::Stock)) {
                    if(!rebuildFault || inactiveSinceFault) {
                        rebuildFault=false; const auto rev=++revision;
                        PLOG_INFO << std::format("[H2Timing] level active: requeued mode={} request={} body_scalar={}",h2timing::name(requested.load()),rev,bodyScalar);
                    }
                }
                wasH2=!leavingTitle;
            });
    }
    ~Impl()
    {
        enabledCallback.reset(); rateCallback.reset(); stateCallback.reset();
        // App waits for stock restoration before destroying this service. This
        // avoids unloading a body-rebuild callback while H2 is paused.
        current=nullptr;
        if(touched && !stockRestored) PLOG_ERROR << "H2 timing service destroyed before restoration";
        { std::lock_guard lock(life->mutex); life->closing=true; }
        tickCallback.reset();
    }
};

H2MasterTickrate::H2MasterTickrate(GameState game, IDIContainer& dicon) : pimpl(std::make_unique<Impl>(game,dicon)) {}
H2MasterTickrate::~H2MasterTickrate()=default;
std::string H2MasterTickrate::status() { std::lock_guard lock(statusMutex); return timingStatus; }

bool H2MasterTickrate::beginShutdown() {return Impl::current && Impl::current->beginShutdown();}
bool H2MasterTickrate::shutdownReady() {return !Impl::current || Impl::current->shutdownReady();}
