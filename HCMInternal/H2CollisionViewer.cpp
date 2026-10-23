#include "pch.h"
#include "H2CollisionViewer.h"
#include "SettingsStateAndEvents.h"
#include "DirectXRenderEvent.h"
#include "IMessagesGUI.h"
#include "IMCCStateHook.h"
#include "ModuleHook.h"
#include "ModuleHookManager.h"
#include "MultilevelPointer.h"
#include "H2CV_Viewer.h"
#include "H2CV_Mem.h"
#include "ScopedThreadSuspender.h"

namespace {
    constexpr int64_t firstPersonRva=0x7E0C60, resolveRva=0x951EC0;
    std::mutex statusMutex;
    std::string gridStatus="Grid is off";
    void setStatus(std::string s) { std::lock_guard lock(statusMutex); gridStatus=std::move(s); }
    bool entryMatches(uintptr_t at,const uint8_t* bytes,size_t n) { uint8_t got[32]; return n<=32 && mem::copy(got,at,n) && memcmp(got,bytes,n)==0; }
    bool sehCall(void (*fn)(void*),void* arg) { __try {fn(arg);return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;} }
}

class H2CollisionViewer::Impl {
    std::weak_ptr<SettingsStateAndEvents> settingsWeak;
    std::weak_ptr<IMCCStateHook> stateWeak;
    std::weak_ptr<IMessagesGUI> messagesWeak;
    struct Life { std::mutex mutex; bool closing=false; };
    std::shared_ptr<Life> life=std::make_shared<Life>();
    std::unique_ptr<ScopedCallback<DirectXRenderEvent>> renderCallback;
    std::unique_ptr<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>> stateCallback;
    std::unique_ptr<ModuleInlineHook> fpHook, resolveHook;
    std::atomic<bool> enabled{false};
    std::atomic<bool> resetScene{false};
    std::mutex viewerMutex;
    h2cv::Viewer mViewer;
    uintptr_t refusedBase=0;
    bool wasActive=false;
    static inline std::atomic<Impl*> self{nullptr};
    static inline std::atomic<unsigned> inFlight{0};
    struct Flight { Flight(){++inFlight;} ~Flight(){--inFlight;inFlight.notify_all();} };
    bool active() {auto s=stateWeak.lock();return ModuleHookManager::h2EffectsAllowed() && s && s->isGameCurrentlyPlaying(GameState::Value::Halo2);}
    static int64_t __fastcall fpDetour(int64_t a,int64_t b,int64_t c,char d,unsigned e,int f) {
        Flight flight; Impl* p=self.load();
        if (!p) return 0;
        if (p->enabled.load()) p->drawHook(false);
        return p->fpHook->getInlineHook().call<int64_t>(a,b,c,d,e,f);
    }
    static int64_t __fastcall resolveDetour(int64_t a,int64_t b,int64_t c) {
        Flight flight; Impl* p=self.load();
        if (!p) return 0;
        auto result=p->resolveHook->getInlineHook().call<int64_t>(a,b,c);
        if (p->enabled.load()) p->drawHook(true);
        return result;
    }
    void drawHook(bool resolve) {
        std::unique_lock lock(viewerMutex,std::try_to_lock);
        if(!lock.owns_lock() || !enabled || !active()) return;
        h2cv::setGame(h2cv::Game::Halo2);
        if(!sehCall(resolve ? [](void* p){static_cast<h2cv::Viewer*>(p)->onResolveDone();} :
                              [](void* p){static_cast<h2cv::Viewer*>(p)->onFirstPersonPass();}, &mViewer)) {
            mViewer.recoverRenderState();
            enabled=false; setStatus("Grid render hook faulted; use collision depth for this session.");
            refusedBase=h2::base();
        }
    }
    void armHooks() {
        const auto b=h2::base();
        if(!b || b==refusedBase || !h2::buildOk()) return;
        static constexpr uint8_t fpBytes[]={0x48,0x8B,0xC4,0x57,0x48,0x81,0xEC,0xB0,0x01,0x00,0x00};
        static constexpr uint8_t rsBytes[]={0x48,0x8B,0xC4,0x55,0x41,0x56,0x48,0x8D,0x68,0xA1,0x48,0x81,0xEC,0xE8,0x00,0x00,0x00};
        if ((!fpHook->isHookInstalled() && !entryMatches(b+firstPersonRva,fpBytes,sizeof(fpBytes))) ||
            (!resolveHook->isHookInstalled() && !entryMatches(b+resolveRva,rsBytes,sizeof(rsBytes)))) {
            refusedBase=b;
            if(auto m=messagesWeak.lock())m->addMessage("H2 grid: game-depth hooks unavailable; using collision depth.");
            return;
        }
        fpHook->setWantsToBeAttached(true);
        resolveHook->setWantsToBeAttached(true);
    }
	static uint32_t packColour(const SimpleMath::Vector4& c)
	{
		auto ch = [](float f) { return (uint32_t)(std::clamp(f, 0.f, 1.f) * 255.f + 0.5f); };
		return ch(c.x) | (ch(c.y) << 8) | (ch(c.z) << 16) | (255u << 24);
	}

	void syncSettings(SettingsStateAndEvents& s)
    {
        auto& v = mViewer.settings;
        using h2cv::Cat;
        const bool only = s.h2NativeGridOnly->GetValue();
        for (auto& show : v.show) show = false;
        const bool map = (only ? s.wireViewShowStructure : s.h2pGridShowStructure)->GetValue();
        const bool inst = (only ? s.wireViewShowInstances : s.h2pGridShowInstances)->GetValue();
        v.show[(int)Cat::Bsp] = v.show[(int)Cat::BspInvisible] = v.show[(int)Cat::Breakable] = map;
        v.show[(int)Cat::Instanced] = v.show[(int)Cat::InstancedInvisible] = inst;
        v.gridOnly = only;
        const auto bg = s.wireViewBackgroundColor->GetValue();
        v.background[0]=bg.x; v.background[1]=bg.y; v.background[2]=bg.z; v.background[3]=1.f;
        v.mapColor = packColour((only ? s.wireViewStructureColor : s.h2pGridStructureColor)->GetValue());
        v.instanceColor = packColour((only ? s.wireViewInstanceColor : s.h2pGridInstanceColor)->GetValue());
        v.lineWidth = (only ? s.wireViewWidth : s.h2pGridWidth)->GetValue();
        v.lineOpacity = (only ? s.wireViewOpacity : s.h2pGridOpacity)->GetValue();
        v.nearRadius = s.h2GridRadius->GetValue();
        v.hiddenAlpha = s.h2GridHiddenAlpha->GetValue();
        v.fills = v.tim = v.timLabels = v.pills = v.hud = v.ghost = v.ghostLog = v.creases = false;
        v.bipedOccluders = !only;
        const bool xray = (only ? s.wireViewXray : s.h2pGridXray)->GetValue();
        v.occlusion = xray ? (int)h2cv::Occlusion::XRay : only ? (int)h2cv::Occlusion::Own :
            s.h2GridOcclusion->GetValue() == 0 ? (int)h2cv::Occlusion::InFrame :
            s.h2GridOcclusion->GetValue() == 1 ? (int)h2cv::Occlusion::PostBloom : (int)h2cv::Occlusion::Own;
    }

    void onRender(ID3D11Device* dev,ID3D11DeviceContext* ctx,SimpleMath::Vector2 screen,ID3D11RenderTargetView* rtv) {
        auto s=settingsWeak.lock(); if(!s)return;
        std::unique_lock lock(viewerMutex,std::try_to_lock); if(!lock.owns_lock())return;
        const bool playing=active();
        if(resetScene.exchange(false) || (wasActive && !playing)) mViewer.resetScene();
        wasActive=playing;
        const bool wanted=playing && !s->h2NativeDisconnecting.load() && (s->h2NativeGrid->GetValue() || s->h2NativeGridOnly->GetValue());
        if(!wanted) {enabled=false;mViewer.restoreBloom();return;}
        h2cv::setGame(h2cv::Game::Halo2);
        if(!h2::base() || !h2::buildOk()) {enabled=false;setStatus("Grid unavailable: expected halo2.dll build 1.3528.");return;}
        syncSettings(*s);
        const int occ=mViewer.settings.occlusion.load();
        const bool engine=occ==(int)h2cv::Occlusion::InFrame || occ==(int)h2cv::Occlusion::PostBloom;
        if(engine)armHooks();
        enabled=engine && h2::base()!=refusedBase;
        mViewer.onRender(dev,ctx,rtv,(uint32_t)screen.x,(uint32_t)screen.y);
        setStatus(mViewer.presentationStatus());
    }
public:
    Impl(GameState game,IDIContainer& dicon) : settingsWeak(dicon.Resolve<SettingsStateAndEvents>()),
        stateWeak(dicon.Resolve<IMCCStateHook>()),messagesWeak(dicon.Resolve<IMessagesGUI>()) {
        if(game!=GameState(GameState::Value::Halo2))throw HCMInitException("H2 viewer requires original Halo 2.");
        auto fp=std::make_shared<MultilevelPointerSpecialisation::ModuleOffset>(L"halo2.dll",std::vector<int64_t>{firstPersonRva});
        auto rs=std::make_shared<MultilevelPointerSpecialisation::ModuleOffset>(L"halo2.dll",std::vector<int64_t>{resolveRva});
        fpHook=ModuleInlineHook::make(L"halo2.dll",fp,(void*)&fpDetour);
        resolveHook=ModuleInlineHook::make(L"halo2.dll",rs,(void*)&resolveDetour);
        const auto drain=[] { return inFlight.load()==0; };
        fpHook->setCallbackDrain(drain);resolveHook->setCallbackDrain(drain);
        self=this;
        renderCallback=std::make_unique<ScopedCallback<DirectXRenderEvent>>(dicon.Resolve<DirectXRenderEvent>().lock(),
            [this,guard=life](ID3D11Device* d,ID3D11DeviceContext* c,SimpleMath::Vector2 z,ID3D11RenderTargetView* r) {
                std::lock_guard lock(guard->mutex);if(!guard->closing)onRender(d,c,z,r);
            });
        stateCallback=std::make_unique<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>>(stateWeak.lock()->getMCCStateChangedEvent(),
            [this,guard=life](const MCCState&) {
                std::lock_guard lock(guard->mutex); if(guard->closing)return;
                enabled=false;resetScene=true;
                std::lock_guard viewLock(viewerMutex);mViewer.restoreBloom();
            });
    }
    ~Impl() {
        {std::lock_guard lock(life->mutex);life->closing=true;enabled=false;}
        renderCallback.reset();stateCallback.reset();
        // Same two-phase path as title retirement. Never hold the viewer,
        // callback-life, or freeze mutex while an original call drains.
        for(;;) {
            const bool first=fpHook->prepareDetach(), second=resolveHook->prepareDetach();
            if(first && second) break;
            Sleep(10);
        }
        while(!fpHook->detach() || !resolveHook->detach()) Sleep(10);
        fpHook->setWantsToBeAttached(false);resolveHook->setWantsToBeAttached(false);
        fpHook.reset();resolveHook.reset();self=nullptr;
        std::lock_guard lock(viewerMutex);mViewer.shutdown();
    }
};
H2CollisionViewer::H2CollisionViewer(GameState game,IDIContainer& dicon):pimpl(std::make_unique<Impl>(game,dicon)){}
H2CollisionViewer::~H2CollisionViewer()=default;
std::string H2CollisionViewer::status(){std::lock_guard lock(statusMutex);return gridStatus;}
