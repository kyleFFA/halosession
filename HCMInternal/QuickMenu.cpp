#include "pch.h"
#include "QuickMenu.h"
#include "QuickMenuCore.h"
#include "QuickMenuView.h"
#include "SettingsStateAndEvents.h"
#include "IMCCStateHook.h"
#include "GUIElementStore.h"
#include "SharedRequestProvider.h"
#include "H2MovementCore.h"
#include "AutoCrouchIdentity.h"
#include "H2CV_Mem.h"
#include "QuickMenuHook.h"
#include "QuickMenuPause.h"
#include "safetyhook.hpp"
#include "pugixml.hpp"
#include <dinput.h>
#include <condition_variable>

namespace {
struct Reader {
 template<class T>T read(uintptr_t at) {T value{};if(!mem::copy(&value,at,sizeof value))throw std::runtime_error("Player unavailable");return value;}
};
bool foreground() {DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);return process==GetCurrentProcessId();}
bool image(uintptr_t base,uint32_t stamp,uint32_t size) {
 if(!base)return false;
 Reader r;auto nt=base+r.read<uint32_t>(base+0x3C);
 return r.read<uint16_t>(base)==0x5A4D && r.read<uint32_t>(nt)==0x4550 && r.read<uint16_t>(nt+4)==0x8664 && r.read<uint32_t>(nt+8)==stamp && r.read<uint32_t>(nt+0x50)==size;
}
bool range(uintptr_t device,int axis,quickmenu::Axis& out) {
 // Device/property query only, on the same thread immediately after MCC's GetDeviceState.
 __try {
  DIPROPRANGE property{};property.diph.dwSize=sizeof property;property.diph.dwHeaderSize=sizeof property.diph;
  property.diph.dwObj=DWORD(axis*4);property.diph.dwHow=DIPH_BYOFFSET;
  const HRESULT hr=reinterpret_cast<IDirectInputDevice8W*>(device)->GetProperty(DIPROP_RANGE,&property.diph);
  if(FAILED(hr)||property.lMin>=property.lMax)return false;
  out={property.lMin,property.lMax};return out.valid();
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool replaceAxes(uintptr_t at,int x,int y,const quickmenu::Axis& horizontal,const quickmenu::Axis& vertical) {
 __try {return quickmenu::suppressDirect(reinterpret_cast<int32_t*>(at),x,y,horizontal,vertical);}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
enum Action { Empty,Save,Back,Auto,Fly,Invulnerability,Tick30,Grid,HUD,Clip,KeepArmed,Arm,Physics,AI,Count };
const char* names[]={"Empty","Save spot","Back to spot","Auto-crouch","Fly","Invulnerability","30 tick","Map grid","Hide HUD","Bottomless clip","Keep armed","Arm now","Halo 2 physics","Disable AI"};
const char* xboxButtons[]={"Unbound","Start / Options","Back / Share / Create","LB / L1","RB / R1","LS / L3","RS / R3","A / Cross","B / Circle","X / Square","Y / Triangle","D-pad up","D-pad down","D-pad left","D-pad right","LT / L2","RT / R2"};
struct Config {
 bool enabled=false;int route=0,button=0,user=0,axisX=-1,axisY=-1;bool invertY=false,confirmed=false;
 float deadzone=.28f,sensitivity=1.f;
 std::array<int,5> slots{Save,Back,Auto,Fly,Invulnerability};
};
struct Observation {quickmenu::Identity identity{};unsigned game=255;bool valid=false;};
}

class QuickMenu::Impl {
public:
 std::shared_ptr<SettingsStateAndEvents> settings;
 std::shared_ptr<IMCCStateHook> state;
 std::shared_ptr<GUIElementStore> store;
 std::shared_ptr<TokenSharedRequestProvider> hotkeyDisabler;
 std::mutex mutex;
 Config config;
 quickmenu::Selection selection;
 std::atomic<bool> closing{false},blocked{true};
 std::atomic<uint64_t> frameTime{0};
 std::atomic<bool> installWanted{true};
 std::atomic<bool> controllerLive{false};
 std::atomic<uint64_t> controllerObject{0},controllerTime{0},cancelRevision{1};
 quickmenu::NativeHook hook;
 std::string path,hookStatus="Controller filter pending verification",lastDispatch;
 std::atomic<uint32_t> stateWord{0};
 std::atomic<uint64_t> stateEpoch{1};
 struct CallbackLife {std::mutex gate;bool closed=false;};
 std::shared_ptr<CallbackLife> callbackLife=std::make_shared<CallbackLife>();
 std::unique_ptr<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>> stateSubscription;
 std::unique_ptr<ScopedCallback<ToggleEvent>> menuSubscription;
 uint64_t nativeDevice=0,nativeSeen=0;
 std::array<int32_t,6> nativeAxes{};
 std::array<bool,32> nativeButtons{};
 quickmenu::Axis rangeX,rangeY;
 uintptr_t rangeDevice=0;int rangeAxisX=-1,rangeAxisY=-1;
 bool nativeRanges=false;
 std::condition_variable ready;
 struct Work {int action;quickmenu::Pick pick;uint64_t revision;};
 std::optional<Work> work;
 std::thread worker;
 inline static std::atomic<Impl*> instance=nullptr;

 Impl(std::shared_ptr<SettingsStateAndEvents> s,std::shared_ptr<IMCCStateHook> st,std::shared_ptr<GUIElementStore> gs,std::shared_ptr<TokenSharedRequestProvider> hd,const std::string& directory)
 :settings(s),state(st),store(gs),hotkeyDisabler(hd),path(directory+"SessionQuickMenu.xml") {
  auto encode=[](const MCCState& m){return uint32_t(m.currentGameState.operator GameState::Value()) | (uint32_t(m.currentPlayState)<<8) | (uint32_t(m.currentLevelID)<<16);};
  stateWord=encode(state->getCurrentMCCState());
  stateSubscription=std::make_unique<ScopedCallback<eventpp::CallbackList<void(const MCCState&)>>>(state->getMCCStateChangedEvent(),[this,encode,life=callbackLife](const MCCState& m){std::lock_guard lock(life->gate);if(life->closed)return;stateWord=encode(m);++stateEpoch;clear();});
  menuSubscription=std::make_unique<ScopedCallback<ToggleEvent>>(settings->GUIWindowOpen->valueChangedEvent,[this,life=callbackLife](bool& open){std::lock_guard lock(life->gate);if(life->closed)return;if(open){blocked=true;clear();}});
  load();instance=this;worker=std::thread([this]{run();});
 }
 ~Impl() {
  closing=true;{std::lock_guard lock(callbackLife->gate);callbackLife->closed=true;}
  menuSubscription.reset();stateSubscription.reset();while(!shutdown())Sleep(10);instance=nullptr;
 }
 void load() {
  pugi::xml_document doc;if(!doc.load_file(path.c_str()))return;auto root=doc.child("QuickMenu");
  auto& c=config;c.enabled=root.attribute("enabled").as_bool(false);c.route=std::clamp(root.attribute("route").as_int(0),0,1);
  c.button=std::clamp(root.attribute("button").as_int(0),0,c.route?36:16);c.user=std::clamp(root.attribute("user").as_int(0),0,3);
  c.axisX=std::clamp(root.attribute("axisX").as_int(-1),-1,5);c.axisY=std::clamp(root.attribute("axisY").as_int(-1),-1,5);
  c.confirmed=root.attribute("confirmed").as_bool(false);c.invertY=root.attribute("invertY").as_bool(false);
  float dz=root.attribute("deadzone").as_float(.28f),sens=root.attribute("sensitivity").as_float(1.f);
  c.deadzone=std::isfinite(dz)?std::clamp(dz,.12f,.75f):.28f;c.sensitivity=std::isfinite(sens)?std::clamp(sens,.5f,2.f):1.f;
  for(int i=0;i<5;++i)c.slots[i]=std::clamp(root.child(("Slot"+std::to_string(i+1)).c_str()).text().as_int(c.slots[i]),0,int(Count)-1);
 }
 void save() {
  pugi::xml_document doc;auto root=doc.append_child("QuickMenu");auto& c=config;
  root.append_attribute("enabled")=c.enabled;root.append_attribute("route")=c.route;root.append_attribute("button")=c.button;root.append_attribute("user")=c.user;
  root.append_attribute("axisX")=c.axisX;root.append_attribute("axisY")=c.axisY;root.append_attribute("confirmed")=c.confirmed;root.append_attribute("invertY")=c.invertY;
  root.append_attribute("deadzone")=c.deadzone;root.append_attribute("sensitivity")=c.sensitivity;
  for(int i=0;i<5;++i)root.append_child(("Slot"+std::to_string(i+1)).c_str()).text()=c.slots[i];
  if(!doc.save_file(path.c_str()))PLOG_ERROR<<"[QuickMenu] Could not save candidate-local settings";
 }
 Observation observe() {
  Observation o;
  if(closing || settings->h2NativeDisconnecting || !foreground())return o;
  try {
   const auto word=stateWord.load();const auto epoch=stateEpoch.load();o.game=word&255;
   if(((word>>8)&255)!=unsigned(PlayState::Ingame))return o;
   Reader r;auto base=uintptr_t(GetModuleHandleW(GameState(GameState::Value(o.game)).toModuleName().c_str()));
   if(o.game==1) {
    h2movement::verifyBuild(r,base);auto s=h2movement::session(r,base);auto b=h2movement::body(r,s);
    o.identity={base,s.globals,s.playersGlobals,b.object,b.player,b.unit,s.sessionNonce,s.simulation,uint64_t((word>>16)&255),s.control};
   } else {
    AutoCrouchLayout layout;uint32_t stamp=0,size=0;uintptr_t health=0;
    if(o.game==2){layout=autoCrouchHalo3;stamp=0x68A0EDFC;size=0x4768000;health=0xF4;}
    else if(o.game==5){layout=autoCrouchODST;stamp=0x68A0F232;size=0x4797000;health=0xE8;}
    else if(o.game==6){layout=autoCrouchReach;stamp=0x68A0EFE1;size=0x4EDA000;health=0xE4C;}
    else return o;
    if(!image(base,stamp,size))return o;
    auto b=readAutoCrouchIdentity(r,base,layout);
    const float hp=r.read<float>(b.object+health);if(!std::isfinite(hp)||hp<=0)return o;
    o.identity={base,b.globals,b.mapping,b.object,b.player,b.unit,b.pool,b.simulation,uint64_t((word>>16)&255),0};
   }
   o.identity[10]=word;o.identity[11]=epoch;
   o.valid=word==stateWord.load() && epoch==stateEpoch.load();
  }catch(...){ }
  return o;
 }
 bool eligible(uint64_t now) {return !closing && !blocked && now-frameTime.load()<200;}
 void cancelLocked() {selection.cancel();work.reset();++cancelRevision;controllerLive=false;settings->quickMenuCapturing=false;}
 void clear() {std::lock_guard lock(mutex);cancelLocked();}
 void close() {closing=true;blocked=true;installWanted=false;clear();ready.notify_all();}
 static void input(safetyhook::Context& context) {
  auto self=instance.load();if(!self||self->closing)return;
  try {self->poll(context);}catch(...){self->clear();}
 }
 void poll(safetyhook::Context& context) {
  const auto object=uintptr_t(context.rbx);Reader r;
  const auto index=r.read<int32_t>(object+0x808);const bool direct=index<0;
  const auto now=GetTickCount64();
  std::lock_guard lock(mutex);auto& c=config;
  if(direct!=(c.route==1))return;
  if(!direct && index!=c.user)return;
  const bool connected=int32_t(context.rax)>=0 && !r.read<uint8_t>(object+0xDAD) && r.read<uint8_t>(object+(direct?0xDAC:8))!=0;
  if(direct) {
   if(nativeDevice && nativeDevice!=object && now-nativeSeen<500)return;
   if(nativeDevice!=object){cancelLocked();nativeDevice=object;nativeRanges=false;}
   if(connected)nativeSeen=now;else nativeRanges=false;
  }
  auto observation=observe();bool down=false;float x=0,y=0;
  quickmenu::XboxState xbox{};
  const bool axesConfigured=c.axisX>=2 && c.axisY>=2 && c.axisX!=c.axisY && c.confirmed;
  if(connected && direct) {
   nativeAxes=r.read<std::array<int32_t,6>>(object+0x80C);
   auto buttons=r.read<std::array<uint8_t,32>>(object+0x80C+48);
   for(int i=0;i<32;++i)nativeButtons[i]=(buttons[i]&0x80)!=0;
   down=quickmenu::nativeDown(buttons,r.read<uint32_t>(object+0x80C+32),c.button);
   auto device=r.read<uintptr_t>(object+0x958);
   if(axesConfigured && (!nativeRanges || rangeDevice!=device || rangeAxisX!=c.axisX || rangeAxisY!=c.axisY)) {
    nativeRanges=range(device,c.axisX,rangeX) && range(device,c.axisY,rangeY);
    rangeDevice=device;rangeAxisX=c.axisX;rangeAxisY=c.axisY;
   }
   if(nativeRanges && axesConfigured) {
    nativeRanges=nativeAxes[c.axisX]>=rangeX.minimum && nativeAxes[c.axisX]<=rangeX.maximum && nativeAxes[c.axisY]>=rangeY.minimum && nativeAxes[c.axisY]<=rangeY.maximum;
    if(nativeRanges){x=rangeX.normalize(nativeAxes[c.axisX]);y=-rangeY.normalize(nativeAxes[c.axisY]);}
   }
  } else if(connected) {
   xbox=r.read<quickmenu::XboxState>(object+0x91C);down=quickmenu::xboxDown(xbox,c.button);
   x=float(xbox.rx)/32768.f;y=float(xbox.ry)/32768.f;
  }
  if(c.invertY)y=-y;
  const bool allow=c.enabled && c.button>0 && eligible(now) && observation.valid && (!direct || (axesConfigured && nativeRanges));
  controllerObject=object;controllerTime=now;controllerLive=connected && allow;
  if(!connected || !allow)cancelLocked();
  const bool filter=selection.sample(object,connected,down,x,y,allow,observation.identity,now,c.deadzone,c.sensitivity);
  settings->quickMenuCapturing=selection.active();
  if(filter) {
   bool ok=false;
   if(direct)ok=replaceAxes(object+0x80C,c.axisX,c.axisY,rangeX,rangeY);
   else ok=replaceXbox(object+0x91C);
   if(!ok)cancelLocked();
  }
 }
 static bool replaceXbox(uintptr_t at) {
  __try {auto state=reinterpret_cast<quickmenu::XboxState*>(at);quickmenu::suppress(*state);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 bool available(int action,unsigned game) {
  if(action==Empty||game==255)return false;
  const bool h2=game==1;
  GUIElementEnum id=GUIElementEnum::spotSaveGUI;
  switch(action) {
  case Save:id=h2?GUIElementEnum::h2NativeSaveSpotGUI:GUIElementEnum::spotSaveGUI;break;
  case Back:id=h2?GUIElementEnum::h2NativeBackToSpotGUI:GUIElementEnum::spotBackGUI;break;
  case Auto:id=h2?GUIElementEnum::h2NativeAutoCrouchGUI:GUIElementEnum::autoCrouchGUI;break;
  case Fly:id=GUIElementEnum::h2pFlyToggleGUI;if(settings->h2pFlyHoldToggle->GetValue())return false;break;
  case Invulnerability:id=GUIElementEnum::invulnGUI;break;
  case Tick30:if(h2 && !settings->h2NativeTickrateEnabled->GetValue())return false;id=h2?GUIElementEnum::h2NativeTickrateGUI:GUIElementEnum::h2pTick30ToggleGUI;break;
  case Grid:id=h2?GUIElementEnum::h2NativeGridGUI:GUIElementEnum::h2pWorldLinesToggleGUI;break;
  case HUD:id=GUIElementEnum::hideHUDToggle;break;
  case Clip:id=h2?GUIElementEnum::h2NativeClipGUI:GUIElementEnum::h2pBottomlessClipGUI;break;
  case KeepArmed:id=GUIElementEnum::h2pKeepArmedGUI;break;
  case Arm:id=GUIElementEnum::h2pArmGUI;break;
  case Physics:id=GUIElementEnum::h2pPhysicsToggleGUI;break;
  case AI:id=h2?GUIElementEnum::aiFreezeGUI:GUIElementEnum::h2pDisableAIGUI;break;
  default:return false;
  }
  auto key=GameState(GameState::Value(game));auto found=store->mapOfSuccessfullyConstructedGUIElements.find(key);
  return found!=store->mapOfSuccessfullyConstructedGUIElements.end() && found->second.contains(id);
 }
 std::shared_ptr<BinarySetting<bool>> toggle(int action,unsigned game) {
  const bool h2=game==1;
  switch(action) {
  case Fly:return settings->h2pFlyToggle;
  case Invulnerability:return h2?settings->h2NativeInvulnerability:settings->invulnerabilityToggle;
  case Tick30:return h2?settings->h2NativeTick30:settings->h2pTick30Toggle;
  case Grid:return h2?settings->h2NativeGrid:settings->h2pWorldLinesToggle;
  case HUD:return h2?settings->h2NativeHideHUD:settings->hideHUDToggle;
  case Clip:return h2?settings->h2NativeClip:settings->bottomlessClipToggle;
  case KeepArmed:return settings->h2pKeepArmedToggle;
  case Physics:return settings->h2pPhysicsToggle;
  case AI:return h2?settings->h2NativeAI:settings->h2pDisableAIToggle;
  default:return {};
  }
 }
 std::string label(int action,unsigned game,bool enabled) {
  std::string text=names[action];if(auto t=toggle(action,game)) {
   bool on=t->GetValue();if(action==Tick30 && game==1 && !settings->h2NativeTickrateEnabled->GetValue())text+=" [Stock]";
   else text+=on?" [On]":" [Off]";
  }
  if(!enabled && action!=Empty)text+=" (unavailable)";
  return text;
 }
 void dispatch(const Work& item) {
  auto o=observe();if(!o.valid || o.identity!=item.pick.identity || !eligible(GetTickCount64()) || GetTickCount64()-item.pick.time>100 || !available(item.action,o.game)
   || item.revision!=cancelRevision.load() || !controllerLive || controllerObject.load()!=item.pick.device || GetTickCount64()-controllerTime.load()>100)return;
  // Existing service callbacks retain their own title, authority and write guards.
  switch(item.action) {
  case Save:(*settings->spotSaveEvent)();break;
  case Back:(*settings->spotBackEvent)();break;
  case Auto:(*settings->autoCrouchEvent)();break;
  case Arm:(*settings->h2pArmEvent)();break;
  default:
   if(auto t=toggle(item.action,o.game))t->flipBoolSetting();break;
  }
 }
 void run() {
  for(;;) {
   std::unique_lock lock(mutex);ready.wait(lock,[&]{return closing || work.has_value();});if(closing)return;
   auto item=*work;work.reset();lock.unlock();
   try {dispatch(item);}catch(const std::exception& e){PLOG_ERROR<<"[QuickMenu] Action refused: "<<e.what();}
   catch(...){PLOG_ERROR<<"[QuickMenu] Action refused: unexpected service error";}
  }
 }
 void draw(int slot,const Observation& o,const SessionSketch::State& geometry) {
  quickmenu::WheelView v;v.selected=slot;
  for(int i=0;i<5;++i){v.enabled[i]=available(config.slots[i],o.game);v.labels[i]=label(config.slots[i],o.game,v.enabled[i]);}
  quickmenu::renderWheel(geometry,v);
 }
 void frame(bool conflict,const SessionSketch::State& geometry) {
  blocked=conflict || !foreground() || hotkeyDisabler->serviceIsRequested();frameTime=GetTickCount64();auto o=observe();
  std::lock_guard lock(mutex);const bool wasActive=selection.active();selection.expire(GetTickCount64());
  if(blocked || !o.valid || (wasActive && !selection.active()))cancelLocked();
  if(auto picked=selection.take(o.identity,GetTickCount64(),o.valid && eligible(GetTickCount64()))) {
   int action=config.slots[picked->slot];if(available(action,o.game)){work=Work{action,*picked,cancelRevision.load()};ready.notify_one();}
  }
  settings->quickMenuCapturing=selection.active();
  if(selection.active())draw(selection.slot,o,geometry);
 }
 void ui() {
  std::lock_guard lock(mutex);auto& c=config;bool changed=false;
  ImGui::TextWrapped("Hold your chosen button, select with the right stick, release once to use. Center cancels. The wheel does not pause the game.");
  changed|=ImGui::Checkbox("Enable quick menu",&c.enabled);
  changed|=ImGui::Combo("Controller route",&c.route,"Xbox / PlayStation via XInput\0PlayStation / native DirectInput\0");
  if(ImGui::IsItemEdited()){c.button=0;c.confirmed=false;nativeDevice=0;nativeRanges=false;}
  if(!c.route) {
   changed|=ImGui::SliderInt("XInput player slot",&c.user,0,3);
   changed|=ImGui::Combo("Activation",&c.button,xboxButtons,int(std::size(xboxButtons)));
   ImGui::TextWrapped("Xbox pads and PlayStation pads mapped to gamepad input by Steam Input or DS4Windows use this route. Mouse-mapped right sticks are not supported.");
  } else {
   auto buttonName=[](int b){if(!b)return std::string("Unbound");if(b<=32)return std::format("Button {}",b);const char* directions[]={"D-pad up","D-pad right","D-pad down","D-pad left"};return std::string(directions[b-33]);};
   if(ImGui::BeginCombo("Activation",buttonName(c.button).c_str())) {
    for(int b=0;b<=36;++b)if(ImGui::Selectable(buttonName(b).c_str(),b==c.button)){c.button=b;changed=true;}
    ImGui::EndCombo();
   }
   ImGui::TextWrapped("Use the live readings to identify your PlayStation button and right-stick axes. Native layouts differ. X/Y are reserved for left-stick movement.");
   const char* axes[]={"Unconfigured","Z","Rx","Ry","Rz"};
   int ax=c.axisX>=2?c.axisX-1:0,ay=c.axisY>=2?c.axisY-1:0;
   if(ImGui::Combo("Right horizontal axis",&ax,axes,5)){c.axisX=ax?ax+1:-1;c.confirmed=false;changed=true;}
   if(ImGui::Combo("Right vertical axis",&ay,axes,5)){c.axisY=ay?ay+1:-1;c.confirmed=false;changed=true;}
   changed|=ImGui::Checkbox("These axes are my right stick",&c.confirmed);
   if(GetTickCount64()-nativeSeen<500) {
    ImGui::Text("Axes: Z %d / Rx %d / Ry %d / Rz %d",nativeAxes[2],nativeAxes[3],nativeAxes[4],nativeAxes[5]);
    std::string pressed="Pressed:";for(int i=0;i<32;++i)if(nativeButtons[i])pressed+=" "+std::to_string(i+1);
    ImGui::TextUnformatted(pressed.c_str());
   }else ImGui::TextDisabled("No native controller seen by MCC yet");
   if(!nativeRanges)ImGui::TextWrapped("Native camera capture stays disabled until both selected axis ranges are confirmed by the device.");
  }
  changed|=ImGui::Checkbox("Invert wheel vertical selection",&c.invertY);
  changed|=ImGui::SliderFloat("Center dead zone",&c.deadzone,.12f,.75f,"%.2f");
  changed|=ImGui::SliderFloat("Stick sensitivity",&c.sensitivity,.5f,2.f,"%.2f");
  ImGui::Separator();auto o=observe();
  for(int i=0;i<5;++i) {
   ImGui::PushID(i);changed|=ImGui::Combo(std::format("Slot {}",i+1).c_str(),&c.slots[i],names,Count);
   ImGui::TextWrapped("%s",label(c.slots[i],o.game,o.valid && available(c.slots[i],o.game)).c_str());ImGui::PopID();
  }
  if(!o.valid)ImGui::TextWrapped("Actions require a verified living local player in a local/server session. Clients, unknown builds, loading and ambiguous player mappings are disabled.");
  ImGui::Separator();ImGui::TextWrapped("%s",hookStatus.c_str());
  ImGui::TextWrapped("Your activation button keeps its normal game action. Choose an overlap you accept. Existing keyboard and full Menu bindings remain available. Fly is unavailable here while Hold Fly is enabled. Original H2 30 tick requires enabling tickrate control in Practice first. The ring follows your saved menu position and size; its practical minimum is 620 x 600 pixels.");
  if(changed){cancelLocked();nativeRanges=false;save();}
 }
 void pump() {
  if(closing || hook || !installWanted)return;
  try {
   Reader r;auto base=uintptr_t(GetModuleHandleW(nullptr));
   if(!image(base,0x68A0E5D7,0x4333000))throw std::runtime_error("Controller filter unavailable: requires Steam MCC 1.3528");
   const auto bytes=r.read<std::array<uint8_t,10>>(base+0x87D38A);
   const std::array<uint8_t,10> expected{0x48,0x8B,0x5C,0x24,0x30,0x48,0x8B,0x6C,0x24,0x38};
   if(bytes!=expected || r.read<uint32_t>(base+0x87D365)!=0x00000110)throw std::runtime_error("Controller filter unavailable: polling code differs");
   hook.install(reinterpret_cast<void*>(base+0x87D38A),input);
   std::lock_guard lock(mutex);hookStatus=hook?"Xbox/XInput and native DirectInput filter ready; in-game controller validation pending":"Controller hook unavailable";
  }catch(const std::exception& e){std::lock_guard lock(mutex);hookStatus=e.what();}
  installWanted=false;
 }
 bool shutdown() {
  close();
  if(worker.joinable())worker.join(); // No service callback may outlive gameplay-service retirement.
  return hook.retire();
 }
};
QuickMenu::QuickMenu(std::shared_ptr<SettingsStateAndEvents> s,std::shared_ptr<IMCCStateHook> st,std::shared_ptr<GUIElementStore> gs,std::shared_ptr<TokenSharedRequestProvider> hd,const std::string& directory):p(std::make_unique<Impl>(s,st,gs,hd,directory)){}
QuickMenu::~QuickMenu()=default;
void QuickMenu::frame(bool conflict,const SessionSketch::State& geometry){p->frame(conflict,geometry);}
void QuickMenu::settings(){p->ui();}
void QuickMenu::cancel(){p->clear();}
void QuickMenu::pump(){p->pump();}
bool QuickMenu::shutdown(){return p->shutdown();}
bool QuickMenu::verifiedSoloCampaign() {
 Reader r;return quickmenu::soloMenuPause(r,uintptr_t(GetModuleHandleW(L"halo2.dll")),true,true);
}
