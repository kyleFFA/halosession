#include "pch.h"
#include "SessionSketch.h"
#include "QuickMenuView.h"
#include "SessionSkinDefinition.h"
#include "imgui_impl_dx11.h"
#include <wrl/client.h>
#include <fstream>
#include <cassert>
using Microsoft::WRL::ComPtr;
int main(){
 namespace fs=std::filesystem;auto root=fs::absolute("out/skin-fixtures");fs::create_directories(root);
 auto write=[&](const char* id,const std::string& text){fs::create_directories(root/id);std::ofstream(root/id/"skin.xml")<<text;};
 auto manifest=[](const char* name,const char* textures="ring=\"ring.png\" clouds=\"clouds.png\"",const char* center=".51"){
  return std::string("<SessionSkin version=\"1\" name=\"")+name+"\"><Textures "+textures+"/><Opening centerX=\""+center+"\" centerY=\".49\" radius=\".28\" wheelInsetRatio=\".08\"/></SessionSkin>";
 };
 auto png=[&](const char* id,const char* destination,const char* source){fs::copy_file(fs::path("HCMInternal/Assets")/source,root/id/destination,fs::copy_options::overwrite_existing);};
 write("valid",manifest("Validation fixture","ring=\"ring.png\" clouds=\"clouds.png\" node=\"node.png\" plaque=\"plaque.png\""));
 png("valid","ring.png","session-ring-circular.png");png("valid","clouds.png","session-ring.png");png("valid","node.png","session-ring-circular.png");png("valid","plaque.png","session-ring-circular.png");
 write("missing",manifest("Missing asset fixture"));png("missing","ring.png","session-ringworld-round.png");
 write("broken",manifest("Broken PNG fixture"));png("broken","ring.png","session-ringworld-round.png");std::ofstream(root/"broken/clouds.png")<<"not a PNG";
 write("opaque",manifest("Covered opening fixture"));const unsigned char coveredOpening[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,5,0,0,0,5,8,6,0,0,0,141,111,38,229,0,0,0,18,73,68,65,84,120,156,99,96,32,25,104,68,53,253,39,93,23,0,104,176,2,4,135,77,59,151,0,0,0,0,73,69,78,68,174,66,96,130};std::ofstream(root/"opaque/ring.png",std::ios::binary).write(reinterpret_cast<const char*>(coveredOpening),sizeof(coveredOpening));png("opaque","clouds.png","session-clouds.png");
 write("nonsquare",manifest("Non-square ring fixture"));png("nonsquare","ring.png","session-clouds.png");png("nonsquare","clouds.png","session-clouds.png");
 write("oversized",manifest("File size cap fixture"));png("oversized","ring.png","session-ringworld-round.png");{std::ofstream file(root/"oversized/clouds.png",std::ios::binary);file.seekp(16*1024*1024);file.put('x');}
 write("badxml","<SessionSkin");write("badmeta",manifest("Invalid geometry fixture","ring=\"ring.png\" clouds=\"clouds.png\"","NaN"));
 write("traversal",manifest("Path escape fixture","ring=\"../valid/ring.png\" clouds=\"../valid/clouds.png\""));
 assert(!SessionSkin::read(root/"badxml")&&!SessionSkin::read(root/"badmeta")&&!SessionSkin::read(root/"traversal"));
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1920,1080};io.DeltaTime=1.f/60;io.Fonts->AddFontDefault();ImGui_ImplDX11_Init(device.Get(),context.Get());
 SessionSketch::setSkinDirectory(root);SessionSketch::prepare(device.Get());assert(SessionSketch::ready());
 D3D11_TEXTURE2D_DESC desc{};desc.Width=1920;desc.Height=1080;desc.ArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rtv;assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target)));assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
 SessionSketch::State state;state.initialized=true;state.size={1410,940};state.backgroundOpacity=.78f;state.wheel.background.w=.23f;
 auto frame=[&]{ImGui_ImplDX11_NewFrame();ImGui::NewFrame();SessionSketch::beginFrame();bool open=true;SessionSketch::render(state,io.DisplaySize,true,open,[](int){ImGui::TextUnformatted("Skin loader fixture");});SessionSketch::endFrame();ImGui::Render();assert(ImGui::GetDrawData()->TotalVtxCount>0);float clear[]={.1f,.15f,.2f,1};context->ClearRenderTargetView(rtv.Get(),clear);auto rt=rtv.Get();context->OMSetRenderTargets(1,&rt,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());};
 frame();assert(SessionSketch::activeSkin().id=="default");auto before=SessionSketch::layout(state);
 SessionSketch::requestSkin("valid");assert(SessionSketch::activeSkin().id=="default");frame();
 auto active=SessionSketch::activeSkin();assert(active.id=="valid"&&active.nodeTexture&&active.plaqueTexture);auto layout=SessionSketch::layout(state);auto opening=SessionSketch::skinOpening();auto wheel=quickmenu::geometry(state);
 assert(std::fabs(opening.center.x-.51f)<.00001f&&std::fabs(opening.radius-.28f)<.00001f);
 assert(std::fabs(wheel.center.x-(layout.artMin.x+layout.diameter*.51f))<.001f&&std::fabs(wheel.center.y-(layout.artMin.y+layout.diameter*.49f))<.001f);
 assert(std::fabs(wheel.radius.x-layout.diameter*.28f*.92f)<.001f&&wheel.radius.x==wheel.radius.y);assert(layout.center.x!=before.center.x);
 assert(state.backgroundOpacity==.78f&&state.wheel.background.w==.23f&&SessionSketch::hit(state,layout.center));
 for(auto id:{"missing","broken","opaque","nonsquare","oversized","badxml","badmeta","traversal","not-installed"}){
  SessionSketch::requestSkin("valid");frame();assert(SessionSketch::activeSkin().id=="valid");SessionSketch::requestSkin(id);assert(SessionSketch::activeSkin().id=="valid");frame();assert(SessionSketch::activeSkin().id=="default"&&SessionSketch::ready()&&!SessionSketch::skinMessage().empty());
  auto restored=SessionSketch::layout(state);assert(restored.center.x==before.center.x&&restored.center.y==before.center.y);assert(!SessionSketch::activeSkin().nodeTexture&&!SessionSketch::activeSkin().plaqueTexture);
 }
 // Refresh also reloads active texture files; deletion falls back safely.
 SessionSketch::requestSkin("valid");frame();fs::remove(root/"valid/clouds.png.saved");fs::rename(root/"valid/clouds.png",root/"valid/clouds.png.saved");SessionSketch::refreshSkins();frame();assert(SessionSketch::activeSkin().id=="default"&&SessionSketch::ready());fs::rename(root/"valid/clouds.png.saved",root/"valid/clouds.png");SessionSketch::refreshSkins();
 // A missing active manifest on refresh returns to V1 at the next frame boundary.
 SessionSketch::requestSkin("valid");frame();fs::remove(root/"valid/skin.xml.saved");fs::rename(root/"valid/skin.xml",root/"valid/skin.xml.saved");SessionSketch::refreshSkins();frame();assert(SessionSketch::activeSkin().id=="default");fs::rename(root/"valid/skin.xml.saved",root/"valid/skin.xml");SessionSketch::refreshSkins();
 SessionSketch::requestSkin("valid");frame();ComPtr<ID3D11Device> second;ComPtr<ID3D11DeviceContext> secondContext;assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&second,nullptr,&secondContext)));SessionSketch::prepare(second.Get());assert(SessionSketch::ready());
 SessionSketch::prepare(device.Get());frame();assert(SessionSketch::activeSkin().id=="valid");SessionSketch::reset();assert(!SessionSketch::ready());SessionSketch::prepare(device.Get());frame();assert(SessionSketch::activeSkin().id=="valid");
 SessionSketch::requestSkin("default");SessionSketch::reset();assert(!SessionSketch::ready());SessionSketch::prepare(device.Get());frame();assert(SessionSketch::ready()&&SessionSketch::activeSkin().id=="default");
 SessionSketch::reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
 std::cout<<"PASS: four replacement PNGs; calibrated shared layout; frame-boundary swaps; malformed/NaN/path escape/missing/corrupt/opaque/oversized fallback; palette preservation; refresh/removal; device reset. Fixtures are not production skins.\n";
}
