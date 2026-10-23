#include "pch.h"
#include "H2NativeMovement.h"
#include "H2MovementCore.h"
#include "H2CV_Mem.h"
#include "SettingsStateAndEvents.h"
#include "IMCCStateHook.h"
#include "IMessagesGUI.h"
#include "IGetMCCVersion.h"
#include "GameTickEventHook.h"
#include "ModuleHookManager.h"
#include "IMakeOrGetCheat.h"

namespace {
struct Memory {
 template<class T> T read(uintptr_t at) {T v{};h2movement::require(readBytes(at,&v,sizeof v),"H2 movement memory is unavailable");return v;}
 bool readBytes(uintptr_t at,void* out,size_t size) {return mem::copy(out,at,size);}
 bool writable(uintptr_t at,size_t size) {
  MEMORY_BASIC_INFORMATION m{};
  if(VirtualQuery((void*)at,&m,sizeof m)!=sizeof m||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
  return (m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY))&&at+size>=at&&at+size<=(uintptr_t)m.BaseAddress+m.RegionSize;
 }
 bool writeBytes(uintptr_t at,const void* value,size_t size) {
  // Only ordinary writable data; no VirtualProtect or game function calls.
  if(!writable(at,size))return false;
  __try {memcpy((void*)at,value,size);return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
};
struct Lifetime {std::mutex mutex;bool closing=false;};
}

class H2NativeMovement::Impl {
 using Action=h2movement::Action;
 std::shared_ptr<Lifetime> life=std::make_shared<Lifetime>();
 std::weak_ptr<IMCCStateHook> stateWeak;
 std::weak_ptr<IMessagesGUI> messagesWeak;
 std::weak_ptr<IGetMCCVersion> versionWeak;
 std::shared_ptr<ObservedEvent<GameTickEvent>> ticks;
 std::unique_ptr<ScopedCallback<GameTickEvent>> tickCallback;
 std::unique_ptr<ScopedCallback<ActionEvent>> saveCallback,backCallback,crouchCallback;
 std::unique_ptr<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>> stateCallback;
 std::optional<h2movement::Bookmark> bookmark;
 std::optional<h2movement::Session> context;
 struct Request {Action action;h2movement::Body body;uint64_t generation;std::chrono::steady_clock::time_point expires;};
 std::optional<Request> request;
 uint64_t generation=1;
 uint32_t lastTick=0;
 bool haveTick=false,fault=false;
 std::string lastUnavailable;
 LevelID level=LevelID::no_map_loaded;
 void say(const std::string& text) {
  PLOG_INFO << "[H2Movement] " << text;
  if(auto m=messagesWeak.lock())m->addMessage(text);
 }
 bool active() {
  auto s=stateWeak.lock();return s&&s->isGameCurrentlyPlaying(GameState::Value::Halo2)&&ModuleHookManager::h2EffectsAllowed();
 }
 void invalidate(const char* why) {
  if(bookmark||request)PLOG_INFO << "[H2Movement] bookmark/request cleared: " << why;
  bookmark.reset();request.reset();context.reset();++generation;haveTick=false;
 }
 h2movement::Session observe(Memory& r) {
  try {
  h2movement::require(active(),"Original H2 gameplay is required");
  auto version=versionWeak.lock();
  h2movement::require(version&&version->getMCCVersionAsString()=="1.3528.0.0"&&version->getMCCProcessType()==MCCProcessType::Steam,"H2 movement is verified only for Steam MCC 1.3528.0.0");
  auto base=(uintptr_t)GetModuleHandleW(L"halo2.dll");h2movement::verifyBuild(r,base);
  auto s=h2movement::session(r,base);
  const auto currentLevel=stateWeak.lock()->getCurrentMCCState().currentLevelID;
  if(!context||*context!=s||currentLevel!=level) {invalidate("map/session identity changed");context=s;level=currentLevel;}
  return s;
  }catch(...){invalidate("session/build verification unavailable");throw;}
 }
 void queue(Action action) {
  // Other titles have their own subscribers to these existing bindable actions.
  auto mcc=stateWeak.lock();if(!mcc||mcc->getCurrentMCCState().currentGameState!=GameState(GameState::Value::Halo2))return;
  try {
   h2movement::require(!fault,"H2 movement is locked after an uncertain write; restart MCC");
   Memory r;auto s=observe(r);auto b=h2movement::body(r,s);auto p=h2movement::pose(r,b);h2movement::validateLook(r,b,p.view);
   if(request&&std::chrono::steady_clock::now()>request->expires)request.reset();
   h2movement::require(!request,"H2 movement already has an action pending");
   if(action==Action::Back)h2movement::require(bookmark&&bookmark->generation==generation,"No H2 spot saved in this map/session");
   // Bind the queued press to this current body. A respawn never executes a stale request.
   request=Request{action,b,generation,std::chrono::steady_clock::now()+std::chrono::seconds(2)};
   say("H2 action queued; resume gameplay within 2 seconds.");
  }catch(const std::exception& e){say(e.what());}
 }
 void onTick(uint32_t tick) {
  if(fault||!active())return;
  const bool hadRequest=request.has_value();
  Memory io;
  try {
   auto s=observe(io);
   lastUnavailable.clear();
   if(haveTick&&tick<lastTick) {invalidate("simulation clock restarted");context=s;}
   lastTick=tick;haveTick=true;
   if(!request)return;
   auto pending=*request;request.reset(); // consume before any work; never repeat or retry a press
   h2movement::require(pending.generation==generation&&std::chrono::steady_clock::now()<=pending.expires,"H2 action expired or its session changed; press again");
   auto b=h2movement::body(io,s);
   h2movement::require(b==pending.body,"H2 player body changed after the button press; press again");
   auto p=h2movement::pose(io,b);h2movement::validateLook(io,b,p.view);
   auto verify=[&] {return active()&&stateWeak.lock()->getCurrentMCCState().currentLevelID==level&&context&&generation==pending.generation&&h2movement::session(io,s.module)==s&&h2movement::body(io,s)==b;};
   if(pending.action==Action::Save) {
    h2movement::require(verify(),"H2 player changed while saving spot");
    bookmark=h2movement::Bookmark{generation,p.feet,p.velocity,p.view};
    say("H2 spot saved: position, view and velocity. Teleport bookmark only.");return;
   }
   auto plan=h2movement::plan(pending.action,b,p,bookmark,generation);
   auto result=h2movement::commit(io,plan,verify);
   if(result==h2movement::Result::Applied) {
    say(pending.action==Action::Back?"H2 back to spot: current player teleported.":"H2 auto-crouch: moved 0.2 units in look direction.");
   }else if(result==h2movement::Result::Uncertain) {
    fault=true;invalidate("uncertain write");say("H2 movement write is unconfirmed. Restart MCC before further movement actions.");
   }else say(result==h2movement::Result::RolledBack?"H2 movement failed; original values restored.":"H2 movement refused; no writes applied.");
  }catch(const std::exception& e){
   if(request)request.reset();
   // Losing only the body (death/respawn/vehicle) keeps the pointer-free bookmark.
   // Session/build failure clears it; an unavailable context cannot be reused.
   try {if(!context||h2movement::session(io,context->module)!=*context)invalidate("session unavailable");}catch(...){invalidate("session unavailable");}
   if(hadRequest)say(e.what());
   else if(lastUnavailable!=e.what()){lastUnavailable=e.what();PLOG_INFO << "[H2Movement] unavailable: " << lastUnavailable;}
  }
 }
public:
 Impl(GameState game,IDIContainer& dicon):stateWeak(dicon.Resolve<IMCCStateHook>()),messagesWeak(dicon.Resolve<IMessagesGUI>()),versionWeak(dicon.Resolve<IGetMCCVersion>()) {
  if(game!=GameState(GameState::Value::Halo2))throw HCMInitException("Native H2 movement supports original Halo 2 only");
  auto settings=dicon.Resolve<SettingsStateAndEvents>().lock();
  auto bind=[&](auto event,Action action){return std::make_unique<ScopedCallback<ActionEvent>>(event,[this,guard=life,action]{std::lock_guard lock(guard->mutex);if(!guard->closing)queue(action);});};
  saveCallback=bind(settings->spotSaveEvent,Action::Save);backCallback=bind(settings->spotBackEvent,Action::Back);crouchCallback=bind(settings->autoCrouchEvent,Action::AutoCrouch);
  ticks=resolveDependentCheat(GameTickEventHook)->getGameTickEvent();
  tickCallback=ticks->subscribe([this,guard=life](uint32_t tick){std::lock_guard lock(guard->mutex);if(!guard->closing)onTick(tick);});
  stateCallback=std::make_unique<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>>(stateWeak.lock()->getMCCStateChangedEvent(),[this,guard=life](const MCCState& state){
   std::lock_guard lock(guard->mutex);if(guard->closing)return;
   if(state.currentGameState!=GameState(GameState::Value::Halo2)||state.currentPlayState!=PlayState::Ingame||state.currentLevelID!=level)invalidate("title/map/loading transition");
  });
 }
 ~Impl() {
  {std::lock_guard lock(life->mutex);life->closing=true;request.reset();bookmark.reset();}
  saveCallback.reset();backCallback.reset();crouchCallback.reset();stateCallback.reset();tickCallback.reset();
 }
};
H2NativeMovement::H2NativeMovement(GameState game,IDIContainer& dicon):pimpl(std::make_unique<Impl>(game,dicon)){}
H2NativeMovement::~H2NativeMovement()=default;
