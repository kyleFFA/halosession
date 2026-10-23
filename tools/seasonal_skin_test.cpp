#include "pch.h"
#include "SessionSketch.h"
#include "SessionSkinDefinition.h"
#include "QuickMenuView.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"
#include <wrl/client.h>
#include <fstream>
#include <cassert>
using Microsoft::WRL::ComPtr;
int main(){
 namespace fs=std::filesystem;const auto production=fs::absolute("skins"),temporary=fs::absolute("out/seasonal-fallbacks");
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1920,1080};io.DeltaTime=1.f/60;io.Fonts->AddFontDefault();SessionSketch::addFonts();ImGui_ImplDX11_Init(device.Get(),context.Get());SessionSketch::setSkinDirectory(production);SessionSketch::prepare(device.Get());
 D3D11_TEXTURE2D_DESC desc{};desc.Width=1920;desc.Height=1080;desc.ArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rtv;assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target)));assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
 SessionSketch::State state;state.initialized=true;state.size={1410,940};quickmenu::WheelView wheel{{"Save spot","Back to spot","Auto-crouch","Fly [Off]","Invulnerability [On]"},{true,true,true,true,true},2};
 auto frame=[&](bool radial=false){ImGui_ImplDX11_NewFrame();ImGui::NewFrame();SessionSketch::beginFrame();if(radial)quickmenu::renderWheel(state,wheel);else{bool open=true;SessionSketch::render(state,io.DisplaySize,true,open,[](int){ImGui::TextUnformatted("Seasonal production-asset fixture");ImGui::Button("Momentary control");});}SessionSketch::endFrame();ImGui::Render();float clear[]={.1f,.15f,.2f,1};context->ClearRenderTargetView(rtv.Get(),clear);auto rt=rtv.Get();context->OMSetRenderTargets(1,&rt,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());};
 frame();assert(SessionSketch::availableSkins().size()==4);
 for(const char* id:{"garden","aurora","autumn"}){
  auto definition=SessionSkin::read(production/id);assert(definition&&definition->cloudRimOpacity>.95f);
  SessionSketch::requestSkin(id);frame();frame();assert(SessionSketch::ready()&&SessionSketch::activeSkin().id==id);
  for(bool compact:{false,true}){
   state.size=compact?ImVec2{900,600}:ImVec2{1410,940};state.position=compact?ImVec2{1000,480}:ImVec2{12,12};SessionSketch::clamp(state,io.DisplaySize);frame();frame();
   const auto layout=SessionSketch::layout(state);const auto geometry=quickmenu::geometry(state);
   assert(std::fabs(geometry.center.x-(layout.artMin.x+layout.diameter*definition->centerX))<.001f&&std::fabs(geometry.center.y-(layout.artMin.y+layout.diameter*definition->centerY))<.001f);
   assert(std::fabs(geometry.radius.x-layout.diameter*definition->radius*(1-definition->insetRatio))<.001f&&geometry.radius.x==geometry.radius.y);
   auto window=ImGui::FindWindowByName("Session###PCM");assert(window&&std::fabs(window->Pos.y+window->Size.y*.5f-layout.center.y)<1.5f);assert(SessionSketch::hit(state,layout.center));
   for(int tab:{1,2,3,0}){auto node=SessionSketch::navigationNode(state,tab);auto point=ImVec2{(node.minimum.x+node.maximum.x)*.5f,node.center.y};io.AddMousePosEvent(point.x,point.y);io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();assert(state.tab==tab);}
   io.AddMousePosEvent(geometry.center.x,geometry.center.y);frame(true);frame(true);assert(!io.WantCaptureMouse&&!io.WantCaptureKeyboard);
   state.animateClouds=true;frame(true);state.animateClouds=false;
  }
  // Damage copies only; curated production assets are never modified by tests.
  auto bad=temporary/id;fs::create_directories(bad);fs::copy_file(production/id/"skin.xml",bad/"skin.xml",fs::copy_options::overwrite_existing);fs::copy_file(production/id/"ring.png",bad/"ring.png",fs::copy_options::overwrite_existing);fs::remove(bad/"clouds.png");
  SessionSketch::setSkinDirectory(temporary);SessionSketch::requestSkin(id);frame();assert(SessionSketch::activeSkin().id=="default"&&SessionSketch::ready()&&!SessionSketch::skinMessage().empty());
  fs::copy_file(production/id/"clouds.png",bad/"clouds.png",fs::copy_options::overwrite_existing);std::ofstream(bad/"ring.png")<<"corrupt PNG";SessionSketch::requestSkin(id);frame();assert(SessionSketch::activeSkin().id=="default"&&SessionSketch::ready());
  SessionSketch::setSkinDirectory(production);SessionSketch::requestSkin(id);frame();assert(SessionSketch::activeSkin().id==id);
 }
 SessionSketch::requestSkin("default");frame();assert(SessionSketch::activeSkin().id=="default"&&SessionSketch::ready());SessionSketch::reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
 std::cout<<"PASS: Garden/Aurora/Autumn production PNGs and manifests; measured shared geometry at two sizes/positions; 24 navigation clicks; circular wheel/no capture; gentle/static clouds; missing/corrupt fallback per skin; V1 return. No game connection.\n";
}
