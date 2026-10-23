#include "pch.h"
#include "SessionSketch.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
using Microsoft::WRL::ComPtr;
namespace SessionSketch {
namespace {
ComPtr<ID3D11Device> owner;
ComPtr<ID3D11ShaderResourceView> ring,clouds;
std::vector<uint8_t> ringMask,cloudMask;UINT maskWidth=0,maskHeight=0;
bool frameLayers=false;
ImFont* bodyFont=nullptr;ImFont* titleFont=nullptr;
struct ComScope {HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);~ComScope(){if(SUCCEEDED(hr))CoUninitialize();}};
bool load(ID3D11Device* device,int id,ComPtr<ID3D11ShaderResourceView>& out) {
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)&load,&module))return false;
 auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);if(!resource)return false;
 auto handle=LoadResource(module,resource);auto data=(BYTE*)LockResource(handle);auto bytes=SizeofResource(module,resource);if(!data||!bytes)return false;
 ComScope com;ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converted;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))||FAILED(factory->CreateStream(&stream))||FAILED(stream->InitializeFromMemory(data,bytes))||FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder))||FAILED(decoder->GetFrame(0,&frame))||FAILED(factory->CreateFormatConverter(&converted))||FAILED(converted->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 UINT w=0,h=0;if(FAILED(converted->GetSize(&w,&h))||!w||!h||w>4096||h>4096)return false;
 std::vector<uint8_t> pixels(size_t(w)*h*4);if(FAILED(converted->CopyPixels(nullptr,w*4,UINT(pixels.size()),pixels.data())))return false;
 auto& mask=id==6001?ringMask:cloudMask;mask.resize(size_t(w)*h);for(size_t i=0;i<mask.size();++i)mask[i]=pixels[i*4+3];maskWidth=w;maskHeight=h;
 D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA init{pixels.data(),w*4,0};ComPtr<ID3D11Texture2D> texture;
 return SUCCEEDED(device->CreateTexture2D(&desc,&init,&texture))&&SUCCEEDED(device->CreateShaderResourceView(texture.Get(),nullptr,&out));
}
struct Styles {
 Styles(){
  auto accent=ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
  auto tint=[&](float strength){return ImVec4{1-(1-accent.x)*strength,1-(1-accent.y)*strength,1-(1-accent.z)*strength,.93f};};
  const std::pair<ImGuiCol,ImVec4> colors[]={
   {ImGuiCol_Text,{.12f,.16f,.18f,1}},{ImGuiCol_TextDisabled,{.32f,.39f,.42f,1}},
   {ImGuiCol_WindowBg,{0,0,0,0}},{ImGuiCol_ChildBg,{0,0,0,0}},{ImGuiCol_Border,{.24f,.43f,.47f,.45f}},
   {ImGuiCol_FrameBg,{.83f,.90f,.92f,.85f}},{ImGuiCol_FrameBgHovered,{.61f,.87f,.91f,1}},{ImGuiCol_FrameBgActive,{.40f,.80f,.86f,1}},
   {ImGuiCol_Button,tint(.25f)},{ImGuiCol_ButtonHovered,tint(.55f)},{ImGuiCol_ButtonActive,tint(.7f)},
   {ImGuiCol_Header,{.74f,.88f,.91f,.65f}},{ImGuiCol_HeaderHovered,{.52f,.84f,.90f,.85f}},{ImGuiCol_HeaderActive,{.30f,.73f,.81f,1}},
   {ImGuiCol_CheckMark,{accent.x*.7f,accent.y*.7f,accent.z*.7f,1}},{ImGuiCol_SliderGrab,accent},{ImGuiCol_SliderGrabActive,accent},
   {ImGuiCol_ScrollbarBg,{.85f,.92f,.94f,.35f}},{ImGuiCol_ScrollbarGrab,{.31f,.59f,.64f,.7f}},{ImGuiCol_ScrollbarGrabHovered,{.08f,.59f,.69f,1}},{ImGuiCol_ScrollbarGrabActive,{.0f,.42f,.54f,1}},
   {ImGuiCol_Separator,{.30f,.61f,.67f,.5f}},{ImGuiCol_PopupBg,{.96f,.98f,.98f,.99f}},
   {ImGuiCol_Tab,{.75f,.88f,.91f,.6f}},{ImGuiCol_TabHovered,{.43f,.85f,.91f,1}},{ImGuiCol_TabActive,{.49f,.83f,.88f,1}}};
  for(auto& c:colors)ImGui::PushStyleColor(c.first,c.second);count=int(std::size(colors));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{6,5});ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,0.f);ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,5.f);ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
 }
 int count=0;~Styles(){ImGui::PopStyleVar(5);ImGui::PopStyleColor(count);}
};
}
void beginFrame(){auto d=ImGui::GetBackgroundDrawList();d->ChannelsSplit(2);d->ChannelsSetCurrent(1);frameLayers=true;}
void endFrame(){if(frameLayers){ImGui::GetBackgroundDrawList()->ChannelsMerge();frameLayers=false;}}
void addFonts(){
 wchar_t path[MAX_PATH]{};GetWindowsDirectoryW(path,MAX_PATH);std::filesystem::path root(path);
 auto body=root/L"Fonts"/L"segoeui.ttf",title=root/L"Fonts"/L"segoepr.ttf";
 if(std::filesystem::exists(body))bodyFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(body.string().c_str(),16.f);
 if(std::filesystem::exists(title))titleFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(title.string().c_str(),32.f);
}
void prepare(ID3D11Device* device){if(owner.Get()==device)return;ring.Reset();clouds.Reset();ringMask.clear();cloudMask.clear();owner=device;if(device){if(!load(device,6001,ring)||!load(device,6002,clouds)){ring.Reset();clouds.Reset();PLOG_ERROR<<"Session sketch artwork unavailable; using cloud-shaped fallback";}}}
void reset(){frameLayers=false;ring.Reset();clouds.Reset();ringMask.clear();cloudMask.clear();owner.Reset();bodyFont=titleFont=nullptr;}
bool ready(){return ring&&clouds;}
ImFont* font(){return bodyFont?bodyFont:ImGui::GetFont();}
void clamp(State& s,ImVec2 screen){
 float maxW=std::max(240.f,screen.x-16),maxH=std::max(260.f,screen.y-16);
 s.size.x=std::clamp(s.size.x,std::min(620.f,maxW),maxW);s.size.y=std::clamp(s.size.y,std::min(600.f,maxH),maxH);
 s.position.x=std::clamp(s.position.x,0.f,std::max(0.f,screen.x-s.size.x));s.position.y=std::clamp(s.position.y,0.f,std::max(0.f,screen.y-s.size.y));
}
bool hit(const State& s,ImVec2 point){
 float x=(point.x-s.position.x)/s.size.x,y=(point.y-s.position.y)/s.size.y;if(x<0||y<0||x>=1||y>=1)return false;
 // Solid interior, independent of transparent-looking holes in the artwork.
 float dx=(x-.54f)/.30f,dy=(y-.52f)/.42f;if(dx*dx+dy*dy<=1)return true;
 if(!ringMask.empty()&&!cloudMask.empty()){size_t i=size_t(y*maskHeight)*maskWidth+size_t(x*maskWidth);return i<ringMask.size()&&i<cloudMask.size()&&std::max(ringMask[i],cloudMask[i])>=32;}
 dx=(x-.54f)/.38f;dy=(y-.52f)/.47f;return dx*dx+dy*dy<=1;
}
void render(State& s,ImVec2 screen,bool inputs,bool& open,const std::function<void(int)>& contents){
 if(!open)return;
 if(!s.initialized){float scale=std::min({1.f,(screen.y-20)/1000.f,(screen.x-20)/900.f});s.size={900*scale,1000*scale};s.initialized=true;}
 clamp(s,screen);auto a=s.position,z=ImVec2(a.x+s.size.x,a.y+s.size.y);
 backdrop(s);
 ImGui::SetNextWindowPos(a);ImGui::SetNextWindowSize(s.size);
 auto captureFlags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoBringToFrontOnFocus;
 if(!inputs||!hit(s,ImGui::GetIO().MousePos))captureFlags|=ImGuiWindowFlags_NoInputs;
 ImGui::Begin("##SessionSilhouette",nullptr,captureFlags);ImGui::End();
 Styles styles;ImGui::SetNextWindowPos({a.x+s.size.x*.365f,a.y+s.size.y*.275f});ImGui::SetNextWindowSize({s.size.x*.40f,s.size.y*.46f});
 auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse;
 if(!inputs)flags|=ImGuiWindowFlags_NoInputs;
 ImGui::Begin("Session###PCM",nullptr,flags);
 if(bodyFont)ImGui::PushFont(bodyFont);
 float scale=std::clamp(s.size.x/900.f,.78f,1.15f);ImGui::SetWindowFontScale(scale);
 float width=ImGui::GetContentRegionAvail().x;auto titlePos=ImGui::GetCursorScreenPos();
 ImGui::InvisibleButton("##drag-session",{std::max(20.f,width-35.f),43*scale});
 if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0)){s.position.x+=ImGui::GetIO().MouseDelta.x;s.position.y+=ImGui::GetIO().MouseDelta.y;}
 ImGui::GetWindowDrawList()->AddText(titleFont?titleFont:ImGui::GetFont(),30*scale,titlePos,IM_COL32(39,53,59,255),"Session");
 if(ImGui::IsItemHovered())ImGui::SetTooltip("Drag to move Session");
 ImGui::SameLine();if(ImGui::Button("X##close-session",{28*scale,28*scale}))open=false;
 if(ImGui::IsItemHovered())ImGui::SetTooltip("Close menu");
 const char* tabs[]={"Practice","Control","Credits","Quick Menu"};
 for(int i=0;i<4;++i){if(i)ImGui::SameLine();bool selected=s.tab==i;if(selected)ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));if(ImGui::Button(tabs[i],{(width-ImGui::GetStyle().ItemSpacing.x*3)/4,28*scale}))s.tab=i;if(selected)ImGui::PopStyleColor();}
 ImGui::Separator();
 ImGui::PushID(s.tab);ImGui::PushID(s.contentRevision);
 ImGui::SetNextWindowContentSize({440*scale,0});
 ImGui::BeginChild("##session-scroll",{0,-26*scale},false,ImGuiWindowFlags_HorizontalScrollbar);
 ImGui::PushItemWidth(120*scale);contents(s.tab);ImGui::PopItemWidth();
 ImGui::EndChild();ImGui::PopID();ImGui::PopID();
 ImGui::TextDisabled("Drag title to move");ImGui::SameLine(std::max(0.f,width-26*scale));
 ImGui::InvisibleButton("##resize-session",{24*scale,22*scale});auto q=ImGui::GetItemRectMin();auto e=ImGui::GetItemRectMax();ImGui::GetWindowDrawList()->AddLine({q.x,e.y},{e.x,q.y},IM_COL32(0,153,174,255),2);
 if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0)){s.size.x+=ImGui::GetIO().MouseDelta.x/.765f;s.size.y+=ImGui::GetIO().MouseDelta.y/.735f;}
 if(ImGui::IsItemHovered()){ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);ImGui::SetTooltip("Drag to resize");}
 ImGui::SetWindowFontScale(1.f);if(bodyFont)ImGui::PopFont();ImGui::End();clamp(s,screen);
}
void backdrop(const State& s) {
 auto draw=ImGui::GetBackgroundDrawList();if(frameLayers)draw->ChannelsSetCurrent(0);
 auto a=s.position,z=ImVec2(a.x+s.size.x,a.y+s.size.y);
 if(ready()){draw->AddImage((ImTextureID)clouds.Get(),a,z);draw->AddImage((ImTextureID)ring.Get(),a,z);}
 else draw->AddEllipseFilled({a.x+s.size.x*.55f,a.y+s.size.y*.52f},{s.size.x*.35f,s.size.y*.46f},IM_COL32(244,249,249,245));
 if(frameLayers)draw->ChannelsSetCurrent(1);
}
}
