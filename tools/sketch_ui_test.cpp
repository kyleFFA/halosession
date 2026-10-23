#include "pch.h"
#include "SessionSketch.h"
#include "GUISimpleButton.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <cassert>
#include <atomic>
using Microsoft::WRL::ComPtr;
struct Keys:IHotkeyRendererImpl {void renderHotkey(std::optional<RebindableHotkeyEnum>,int)override{ImGui::Button("Unbound##testkey");}};
static void png(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* image,const wchar_t* path){
 D3D11_TEXTURE2D_DESC d{};image->GetDesc(&d);d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;assert(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&staging)));context->CopyResource(staging.Get(),image);
 D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)));
 ComPtr<IWICImagingFactory> f;ComPtr<IWICStream> stream;ComPtr<IWICBitmapEncoder> encoder;ComPtr<IWICBitmapFrameEncode> frame;assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))));assert(SUCCEEDED(f->CreateStream(&stream)));assert(SUCCEEDED(stream->InitializeFromFilename(path,GENERIC_WRITE)));assert(SUCCEEDED(f->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder)));assert(SUCCEEDED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)));assert(SUCCEEDED(encoder->CreateNewFrame(&frame,nullptr)));assert(SUCCEEDED(frame->Initialize(nullptr)));assert(SUCCEEDED(frame->SetSize(d.Width,d.Height)));auto format=GUID_WICPixelFormat32bppRGBA;assert(SUCCEEDED(frame->SetPixelFormat(&format)));std::vector<BYTE> output(size_t(d.Width)*d.Height*4);for(UINT y=0;y<d.Height;++y){auto src=(BYTE*)mapped.pData+y*mapped.RowPitch;auto dst=output.data()+size_t(y)*d.Width*4;memcpy(dst,src,d.Width*4);if(format==GUID_WICPixelFormat32bppBGRA)for(UINT x=0;x<d.Width;++x)std::swap(dst[x*4],dst[x*4+2]);}assert(SUCCEEDED(frame->WritePixels(d.Height,d.Width*4,UINT(output.size()),output.data())));assert(SUCCEEDED(frame->Commit()));assert(SUCCEEDED(encoder->Commit()));context->Unmap(staging.Get(),0);
}
int main(){
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1280,1000};io.DeltaTime=1.f/60;io.Fonts->AddFontDefault();SessionSketch::addFonts();ImGui_ImplDX11_Init(device.Get(),context.Get());SessionSketch::prepare(device.Get());assert(SessionSketch::ready());
 D3D11_TEXTURE2D_DESC d{};d.Width=1280;d.Height=1000;d.ArraySize=1;d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_RENDER_TARGET;ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rtv;assert(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&target)));assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
 SessionSketch::State state;bool open=true;bool input=true;std::atomic<int> actions=0;auto event=std::make_shared<ActionEvent>();auto subscription=event->append([&]{++actions;});
 HotkeyRenderer keys(std::make_unique<Keys>());GUISimpleButton<true> action(GameState::Value::Halo2,ToolTipCollection("Momentary action"),RebindableHotkeyEnum::autoCrouchHotkey,"Auto-crouch",event);
 bool stress=false;ImVec2 actionCenter{},dragCenter{},resizeCenter{};ImGuiWindow* scroll=nullptr;
 auto frame=[&]{
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
  // A contrasting offline test backdrop makes alpha edges and text contrast visible.
  auto bg=ImGui::GetBackgroundDrawList();bg->AddRectFilled({0,0},{1280,1000},IM_COL32(28,46,58,255));for(int i=0;i<20;++i)bg->AddLine({float(i*80),0},{float(i*80-300),1000},IM_COL32(40,65,78,255));
  SessionSketch::beginFrame();
  if(stress){auto q=ImVec2{state.position.x+state.size.x*.5f,state.position.y+state.size.y*.84f};bg->AddRectFilled(q,{q.x+8,q.y+8},IM_COL32(231,15,85,255));}
  SessionSketch::render(state,io.DisplaySize,input,open,[&](int tab){
   scroll=ImGui::GetCurrentWindow();
   if(tab==2){ImGui::TextWrapped("Credits: Burnt / Poorboy");ImGui::TextWrapped("Based on Poorboys / HCM by Poorboy, Burnt-o and Harc");ImGui::TextWrapped("KyleFFA (badstrafe)");return;}
   if(tab==1){ImGui::Text("Menu toggle");ImGui::SameLine();ImGui::Button("` / ~##example-default");ImGui::TextWrapped("Your saved binding is preserved.");return;}
   ImGui::Text("Halo 2  |  Offline UI preview");ImGui::Separator();ImGui::Text("Save spot");ImGui::Button("Save spot##test");ImGui::SameLine();ImGui::Button("Back to spot##test");
   action.render(keys);auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();actionCenter={(a.x+b.x)*.5f,(a.y+b.y)*.5f};
   static bool hud=false;ImGui::Checkbox("Hide HUD",&hud);ImGui::TextWrapped("0.2 units in look direction, including pitch.");
   for(int i=0;i<(stress?65:0);++i){ImGui::PushID(i);ImGui::Button("Long menu control");ImGui::SameLine();ImGui::Text("%02d",i);ImGui::PopID();}
  });
  SessionSketch::endFrame();ImGui::Render();float clear[]={.10f,.14f,.18f,1};context->ClearRenderTargetView(rtv.Get(),clear);auto rt=rtv.Get();context->OMSetRenderTargets(1,&rt,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
 };
 auto mouse=[&](ImVec2 pos,bool down){io.AddMousePosEvent(pos.x,pos.y);io.AddMouseButtonEvent(0,down);frame();};
 frame();frame();assert(ImGui::GetDrawData()->TotalVtxCount>0);png(device.Get(),context.Get(),target.Get(),L"out/ui-preview-1280.png");stress=true;frame();
 // Production GUISimpleButton is momentary and does not fire repeatedly while held.
 mouse(actionCenter,true);for(int i=0;i<35;++i)frame();assert(actions==0);mouse(actionCenter,false);for(int i=0;i<50&&actions!=1;++i)Sleep(1);assert(actions==1);for(int i=0;i<15;++i)frame();assert(actions==1);
 auto root=ImGui::FindWindowByName("Session###PCM");assert(root);dragCenter={root->Pos.x+45,root->Pos.y+20};auto before=state.position;mouse(dragCenter,true);io.AddMousePosEvent(dragCenter.x+45,dragCenter.y+15);frame();mouse({dragCenter.x+45,dragCenter.y+15},false);assert(state.position.x>before.x);
 frame();root=ImGui::FindWindowByName("Session###PCM");resizeCenter={root->Pos.x+root->Size.x-20,root->Pos.y+root->Size.y-15};auto oldSize=state.size;mouse(resizeCenter,true);io.AddMousePosEvent(resizeCenter.x+50,resizeCenter.y+20);frame();mouse({resizeCenter.x+50,resizeCenter.y+20},false);assert(state.size.x>oldSize.x);
 // Wheel reaches long content; horizontal scrollbar handles narrow layouts.
 frame();assert(scroll&&scroll->ScrollMax.y>0);
 {D3D11_TEXTURE2D_DESC readDesc=d;readDesc.BindFlags=0;readDesc.Usage=D3D11_USAGE_STAGING;readDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> readback;assert(SUCCEEDED(device->CreateTexture2D(&readDesc,nullptr,&readback)));context->CopyResource(readback.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE sample{};assert(SUCCEEDED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&sample)));auto x=UINT(state.position.x+state.size.x*.5f)+3,y=UINT(state.position.y+state.size.y*.84f)+3;auto pixel=(BYTE*)sample.pData+y*sample.RowPitch+x*4;assert(pixel[0]==231&&pixel[1]==15&&pixel[2]==85);context->Unmap(readback.Get(),0);}
 io.AddMousePosEvent(scroll->Pos.x+40,scroll->Pos.y+60);io.AddMouseWheelEvent(0,-8);frame();frame();assert(scroll->Scroll.y>0);++state.contentRevision;frame();frame();assert(scroll->Scroll.y==0);
 state.tab=1;frame();frame();assert(scroll->Scroll.y==0);png(device.Get(),context.Get(),target.Get(),L"out/ui-preview-control.png");state.tab=2;frame();frame();png(device.Get(),context.Get(),target.Get(),L"out/ui-preview-credits.png");
 for(ImVec2 screen:{ImVec2{800,600},ImVec2{1280,720},ImVec2{2560,1440}}){io.DisplaySize=screen;state.tab=0;frame();assert(state.position.x>=0&&state.position.y>=0&&state.position.x+state.size.x<=screen.x&&state.position.y+state.size.y<=screen.y);}
 io.DisplaySize={1280,1000};state.position={12,12};state.size={810,900};state.tab=0;frame();io.AddMousePosEvent(2,2);frame();frame();assert(!io.WantCaptureMouse);assert(!SessionSketch::hit(state,{state.position.x,state.position.y}));
 auto center=ImVec2{state.position.x+state.size.x*.53f,state.position.y+state.size.y*.20f};assert(SessionSketch::hit(state,center));io.AddMousePosEvent(center.x,center.y);frame();frame();assert(io.WantCaptureMouse);
 input=false;frame();assert(actions==1);open=false;frame();assert(actions==1);
 SessionSketch::reset();assert(!SessionSketch::ready());SessionSketch::prepare(device.Get());assert(SessionSketch::ready());ComPtr<ID3D11Device> second;ComPtr<ID3D11DeviceContext> secondContext;assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&second,nullptr,&secondContext)));SessionSketch::prepare(second.Get());assert(SessionSketch::ready());SessionSketch::reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();event->remove(subscription);CoUninitialize();
 std::cout<<"PASS: real sketch renderer/WARP PNGs, momentary production action, dragging, resize, scrolling, scale/clamp, transparent exterior hit testing, hidden/no-input behavior and texture reset/device replacement\n";
}
