#include "pch.h"
#include "SessionSketch.h"
#include "QuickMenuView.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <cassert>
#include <atomic>
using Microsoft::WRL::ComPtr;
static void png(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* image,const wchar_t* path){
 D3D11_TEXTURE2D_DESC d{};image->GetDesc(&d);d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;assert(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&staging)));context->CopyResource(staging.Get(),image);
 D3D11_MAPPED_SUBRESOURCE mapped{};assert(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)));
 ComPtr<IWICImagingFactory> f;ComPtr<IWICStream> stream;ComPtr<IWICBitmapEncoder> encoder;ComPtr<IWICBitmapFrameEncode> frame;assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f))));assert(SUCCEEDED(f->CreateStream(&stream)));assert(SUCCEEDED(stream->InitializeFromFilename(path,GENERIC_WRITE)));assert(SUCCEEDED(f->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder)));assert(SUCCEEDED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)));assert(SUCCEEDED(encoder->CreateNewFrame(&frame,nullptr)));assert(SUCCEEDED(frame->Initialize(nullptr)));assert(SUCCEEDED(frame->SetSize(d.Width,d.Height)));auto format=GUID_WICPixelFormat32bppRGBA;assert(SUCCEEDED(frame->SetPixelFormat(&format)));std::vector<BYTE> output(size_t(d.Width)*d.Height*4);for(UINT y=0;y<d.Height;++y){auto src=(BYTE*)mapped.pData+y*mapped.RowPitch;auto dst=output.data()+size_t(y)*d.Width*4;memcpy(dst,src,d.Width*4);if(format==GUID_WICPixelFormat32bppBGRA)for(UINT x=0;x<d.Width;++x)std::swap(dst[x*4],dst[x*4+2]);}assert(SUCCEEDED(frame->WritePixels(d.Height,d.Width*4,UINT(output.size()),output.data())));assert(SUCCEEDED(frame->Commit()));assert(SUCCEEDED(encoder->Commit()));context->Unmap(staging.Get(),0);
}
int main(int argc,char** argv){
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)));
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize={1280,1000};io.DeltaTime=1.f/60;
 io.Fonts->AddFontDefault();SessionSketch::addFonts();ImGui_ImplDX11_Init(device.Get(),context.Get());SessionSketch::prepare(device.Get());assert(SessionSketch::ready());if(argc>=3){SessionSketch::setSkinDirectory(std::filesystem::path(argv[1]));SessionSketch::requestSkin(argv[2]);}
 D3D11_TEXTURE2D_DESC d{};d.Width=1280;d.Height=1000;d.ArraySize=1;d.MipLevels=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D> target;ComPtr<ID3D11RenderTargetView> rtv;assert(SUCCEEDED(device->CreateTexture2D(&d,nullptr,&target)));assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
 SessionSketch::State state;state.position={12,12};state.size={1250,900};state.initialized=true;
 quickmenu::WheelView view{{"Save spot","Back to spot","Auto-crouch","Fly [Off] (unavailable)","Invulnerability [On]"},{true,true,true,false,true},0};
 auto shot=[&](const wchar_t* original,const char* suffix){std::filesystem::path path=argc>=4?std::filesystem::path(std::string(argv[3])+suffix):std::filesystem::path(original);png(device.Get(),context.Get(),target.Get(),path.c_str());};
 auto frame=[&]{
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();
  auto bg=ImGui::GetBackgroundDrawList();bg->AddRectFilled({0,0},io.DisplaySize,IM_COL32(27,46,57,255));SessionSketch::beginFrame();
  SessionSketch::clamp(state,io.DisplaySize);quickmenu::renderWheel(state,view);
  SessionSketch::endFrame();ImGui::EndFrame();ImGui::Render();float clear[4]={.1f,.15f,.2f,1};context->ClearRenderTargetView(rtv.Get(),clear);auto rt=rtv.Get();context->OMSetRenderTargets(1,&rt,nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
 };
 frame();frame();shot(L"out/quick-wheel-large.png","-wheel.png");
 state.wheel.background.w=.10f;state.wheel.segment.w=.04f;state.wheel.selected.w=.32f;frame();shot(L"out/quick-wheel-transparent.png","-wheel-transparent.png");state.wheel=SessionSketch::WheelAppearance{};
 auto original=quickmenu::geometry(state);assert(original.center.x==SessionSketch::layout(state).center.x);assert(original.radius.x==original.radius.y);
 for(int i=0;i<5;++i){view.selected=i;frame();assert(ImGui::GetDrawData()->TotalVtxCount>0);}
 state.position={370,390};state.size={900,600};view.selected=3;frame();frame();
 auto smallGeometry=quickmenu::geometry(state);assert(smallGeometry.center.x>original.center.x && smallGeometry.radius.x<original.radius.x);
 assert(smallGeometry.center.x+smallGeometry.radius.x<state.position.x+state.size.x && smallGeometry.center.y+smallGeometry.radius.y*1.12f<state.position.y+state.size.y);
 shot(L"out/quick-wheel-small-bottom-right.png","-wheel-small.png");
 // No input window or screen-center fallback; mouse capture stays false even inside the ring.
 io.AddMousePosEvent(smallGeometry.center.x,smallGeometry.center.y);frame();frame();assert(!io.WantCaptureMouse && !io.WantCaptureKeyboard);
 for(auto screen:{ImVec2{800,600},ImVec2{1280,720},ImVec2{2560,1440},ImVec2{320,240}}){io.DisplaySize=screen;frame();auto g=quickmenu::geometry(state);assert(g.center.x>=0 && g.center.y>=0 && g.center.x<screen.x && g.center.y<screen.y);assert(g.center.x-g.radius.x>=0 && g.center.x+g.radius.x<=screen.x);}
 // Geometry uses uniform scale and the shared center even for a legacy rectangular size.
 SessionSketch::State a;a.position={4,8};a.size={800,900};auto ga=quickmenu::geometry(a);a.position={104,58};a.size={400,450};auto gb=quickmenu::geometry(a);
 assert(std::fabs(gb.radius.x-ga.radius.x*.5f)<.001f && std::fabs(gb.font-ga.font*.5f)<.001f);assert(ga.radius.x==ga.radius.y && gb.radius.x==gb.radius.y);assert(gb.center.x==SessionSketch::layout(a).center.x && gb.center.y==SessionSketch::layout(a).center.y);
 if(argc>=3)assert(SessionSketch::activeSkin().id==argv[2]);SessionSketch::reset();ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();CoUninitialize();
 std::cout<<"PASS: production radial renderer and ring/cloud artwork on WARP; five highlights/disabled labels; saved menu geometry; drag/resize proportions; bottom-right/minimum/tiny viewport clamping; no input capture\n";
}
