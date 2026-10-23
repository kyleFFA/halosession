#include "pch.h"
#include "SessionSketch.h"
#include "QuickMenuView.h"
#include "imgui_impl_dx11.h"
#include <wrl/client.h>
#include <cassert>
#include <chrono>
#include <fstream>
#include <numeric>
using Microsoft::WRL::ComPtr;
int main(int argc,char** argv){
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 bool hardware=SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
 if(!hardware)assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
 ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC info{};
 device.As(&dxgi);dxgi->GetAdapter(&adapter);adapter->GetDesc(&info);std::string name;
 for(auto c:info.Description){if(!c)break;name.push_back(c<128?char(c):'?');}
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1920,1080};io.DeltaTime=1.f/60;
 io.Fonts->AddFontDefault();SessionSketch::addFonts();ImGui_ImplDX11_Init(device.Get(),context.Get());if(argc>=3)SessionSketch::setSkinDirectory(std::filesystem::path(argv[1]));SessionSketch::prepare(device.Get());assert(SessionSketch::ready());if(argc>=3)SessionSketch::requestSkin(argv[2]);
 D3D11_TEXTURE2D_DESC desc{};desc.Width=1920;desc.Height=1080;desc.ArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rtv;assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target)));assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
 SessionSketch::State state;state.position={20,20};state.size={1410,940};state.initialized=true;
 quickmenu::WheelView view{{"Save spot","Back to spot","Auto-crouch","Fly [Off]","Invulnerability [On]"},{true,true,true,true,true},1};
 auto submit=[&](bool wheel,bool backdropOnly,const float* clear){
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();SessionSketch::beginFrame();
  if(backdropOnly)SessionSketch::backdrop(state);
  else if(wheel)quickmenu::renderWheel(state,view);
  else{bool open=true;SessionSketch::render(state,io.DisplaySize,true,open,[](int){
   ImGui::TextUnformatted("Offline rendering fixture");ImGui::Separator();
   for(int i=0;i<10;++i){ImGui::PushID(i);ImGui::Button("Practice control");ImGui::SameLine();bool value=false;ImGui::Checkbox("Enabled",&value);ImGui::PopID();}
  });}
  SessionSketch::endFrame();ImGui::Render();context->ClearRenderTargetView(rtv.Get(),clear);auto rt=rtv.Get();context->OMSetRenderTargets(1,&rt,nullptr);
 };
 auto draw=[&]{ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());};
 const float dark[]={.04f,.06f,.08f,1},bright[]={.92f,.94f,.96f,1};
 auto centerPixel=[&](const float* clear){
  submit(false,true,clear);draw();auto readDesc=desc;readDesc.Usage=D3D11_USAGE_STAGING;readDesc.BindFlags=0;readDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  ComPtr<ID3D11Texture2D> staging;assert(SUCCEEDED(device->CreateTexture2D(&readDesc,nullptr,&staging)));context->CopyResource(staging.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)));
  auto c=SessionSketch::layout(state).center;auto pixel=(BYTE*)mapped.pData+UINT(c.y)*mapped.RowPitch+UINT(c.x)*4;std::array<int,3> result{pixel[0],pixel[1],pixel[2]};context->Unmap(staging.Get(),0);return result;
 };
 auto low=centerPixel(dark),high=centerPixel(bright);for(int i=0;i<3;++i)assert(high[i]>low[i]&&high[i]-low[i]<25);
 std::ofstream out(argc>=4?argv[3]:"out/circular-skin-benchmark.json");out<<"{\n  \"skin_id\": \""<<SessionSketch::activeSkin().id<<"\",\n  \"adapter\": \""<<name<<"\",\n  \"hardware\": "<<(hardware?"true":"false")<<",\n  \"viewport\": [1920,1080],\n  \"diameter\": 940,\n  \"samples_per_case\": 100,\n  \"center_dark_scene_rgb\": ["<<low[0]<<","<<low[1]<<","<<low[2]<<"],\n  \"center_bright_scene_rgb\": ["<<high[0]<<","<<high[1]<<","<<high[2]<<"],\n  \"cases\": [\n";
 bool first=true;
 for(bool wheel:{false,true})for(bool drift:{false,true}){
  state.animateClouds=drift;for(int i=0;i<15;++i){submit(wheel,false,dark);draw();}
  std::vector<double> cpu,gpu;int vertices=0,indices=0,commands=0;
  for(int sample=0;sample<100;++sample){
   ComPtr<ID3D11Query> disjoint,start,end;D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP_DISJOINT,0};assert(SUCCEEDED(device->CreateQuery(&q,&disjoint)));q.Query=D3D11_QUERY_TIMESTAMP;assert(SUCCEEDED(device->CreateQuery(&q,&start)));assert(SUCCEEDED(device->CreateQuery(&q,&end)));
   auto t0=std::chrono::steady_clock::now();submit(wheel,false,dark);context->Begin(disjoint.Get());context->End(start.Get());draw();context->End(end.Get());context->End(disjoint.Get());
   cpu.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count());
   context->Flush();D3D11_QUERY_DATA_TIMESTAMP_DISJOINT frequency{};auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   HRESULT result;while((result=context->GetData(disjoint.Get(),&frequency,sizeof(frequency),0))==S_FALSE){assert(std::chrono::steady_clock::now()<deadline);Sleep(0);}assert(SUCCEEDED(result));
   UINT64 a=0,b=0;assert(context->GetData(start.Get(),&a,sizeof(a),0)==S_OK);assert(context->GetData(end.Get(),&b,sizeof(b),0)==S_OK);if(!frequency.Disjoint)gpu.push_back(double(b-a)*1000./frequency.Frequency);
   auto data=ImGui::GetDrawData();vertices=data->TotalVtxCount;indices=data->TotalIdxCount;commands=0;for(auto list:data->CmdLists)commands+=list->CmdBuffer.Size;
  }
  assert(!gpu.empty());auto mean=[](const std::vector<double>& values){return std::accumulate(values.begin(),values.end(),0.)/values.size();};std::sort(cpu.begin(),cpu.end());std::sort(gpu.begin(),gpu.end());
  if(!first)out<<",\n";first=false;
  out<<"    {\"view\":\""<<(wheel?"wheel":"menu")<<"\",\"drift\":"<<(drift?"true":"false")<<",\"cpu_mean_ms\":"<<mean(cpu)<<",\"cpu_median_ms\":"<<cpu[cpu.size()/2]<<",\"gpu_mean_ms\":"<<mean(gpu)<<",\"gpu_median_ms\":"<<gpu[gpu.size()/2]<<",\"gpu_p95_ms\":"<<gpu[gpu.size()*95/100]<<",\"vertices\":"<<vertices<<",\"indices\":"<<indices<<",\"draw_commands\":"<<commands<<"}";
 }
 out<<"\n  ]\n}\n";out.close();SessionSketch::reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
 std::cout<<"PASS: "<<name<<"; 400 timestamped offline frames; center remains mostly opaque with measurable scene visibility. See out/circular-skin-benchmark.json\n";
}
