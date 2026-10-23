#include "pch.h"
#include "SessionSketch.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"
#include <wrl/client.h>
#include <cassert>
#include <iomanip>
using Microsoft::WRL::ComPtr;
int main(){
 SessionSketch::State defaults;SessionSketch::clamp(defaults,{3840,2160});
 assert(std::fabs(defaults.position.x-1220)<.002f&&std::fabs(defaults.position.y-617)<.002f);
 assert(defaults.size.x==1350&&defaults.size.y==900);
 for(auto screen:{ImVec2{1920,1080},ImVec2{1280,720}}){SessionSketch::State responsive;SessionSketch::clamp(responsive,screen);assert(responsive.position.x>=0&&responsive.position.y>=0&&responsive.position.x+responsive.size.x<=screen.x&&responsive.position.y+responsive.size.y<=screen.y);}
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1024,768};io.DeltaTime=1.f/60;io.Fonts->AddFontDefault();ImGui_ImplDX11_Init(device.Get(),context.Get());
 D3D11_TEXTURE2D_DESC desc{};desc.Width=1024;desc.Height=768;desc.ArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rtv;assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target)));assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
 SessionSketch::State state;state.initialized=true;state.position={0,0};state.size={900,600};
 auto frame=[&](double seconds,float clearValue){GImGui->Time=seconds-io.DeltaTime;ImGui_ImplDX11_NewFrame();ImGui::NewFrame();SessionSketch::beginFrame();SessionSketch::backdrop(state);SessionSketch::endFrame();ImGui::Render();float clear[]={clearValue,clearValue,clearValue,1};context->ClearRenderTargetView(rtv.Get(),clear);auto rt=rtv.Get();context->OMSetRenderTargets(1,&rt,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());};
 auto pixel=[&]{auto stagingDesc=desc;stagingDesc.Usage=D3D11_USAGE_STAGING;stagingDesc.BindFlags=0;stagingDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;assert(SUCCEEDED(device->CreateTexture2D(&stagingDesc,nullptr,&staging)));context->CopyResource(staging.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)));auto c=SessionSketch::layout(state).center;auto p=(BYTE*)mapped.pData+UINT(c.y)*mapped.RowPitch+UINT(c.x)*4;std::array<int,3> value{p[0],p[1],p[2]};context->Unmap(staging.Get(),0);return value;};
 // No artwork prepared: isolate the actual center backing blend from cloud alpha.
 frame(1,0);auto dark=pixel();frame(2,1);auto bright=pixel();for(int i=0;i<3;++i)assert(bright[i]-dark[i]>=126&&bright[i]-dark[i]<=129);
 SessionSketch::prepare(device.Get());assert(SessionSketch::ready());
 auto primaryPosition=[&]{auto draw=ImGui::GetBackgroundDrawList();for(const auto& command:draw->CmdBuffer)if(command.ElemCount&&command.GetTexID()!=io.Fonts->TexID){auto index=draw->IdxBuffer[command.IdxOffset];return draw->VtxBuffer[command.VtxOffset+index].pos;}assert(false);return ImVec2{};};
 frame(12,0);auto actual=primaryPosition();auto layout=SessionSketch::layout(state);
 assert(std::fabs(actual.x-(layout.artMin.x+std::sin(12*.035*1.10)*layout.diameter*.007f))<.001f);
 assert(std::fabs(actual.y-(layout.artMin.y+std::sin(12*.027*1.10)*layout.diameter*.004f))<.001f);
 const double peak=3.141592653589793/2/(.035*1.10);frame(peak,0);assert(std::fabs(primaryPosition().x-layout.artMin.x-layout.diameter*.007f)<.001f);
 state.animateClouds=false;frame(12,0);auto stationary=primaryPosition();assert(stationary.x==layout.artMin.x&&stationary.y==layout.artMin.y);
 auto array=[](ImVec4 c){std::cout<<'['<<c.x<<','<<c.y<<','<<c.z<<','<<c.w<<']';};
 std::cout<<std::setprecision(9)<<"{\"gentle_clouds\":"<<(defaults.animateClouds?"true":"false")<<",\"background_opacity\":"<<defaults.backgroundOpacity<<",\"wheel_rgba\":";array(defaults.wheel.background);std::cout<<",\"segment_rgba\":";array(defaults.wheel.segment);std::cout<<",\"selected_rgba\":";array(defaults.wheel.selected);std::cout<<",\"reference_position\":["<<defaults.position.x<<','<<defaults.position.y<<"],\"size\":["<<defaults.size.x<<','<<defaults.size.y<<"],\"cloud_speed_multiplier\":"<<SessionAppearanceDefaults::cloudSpeedMultiplier<<",\"background_blend_delta\":["<<bright[0]-dark[0]<<','<<bright[1]-dark[1]<<','<<bright[2]-dark[2]<<"]}\n";
 SessionSketch::reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
}
