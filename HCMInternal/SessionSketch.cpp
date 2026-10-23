#include "pch.h"
#include "SessionSketch.h"
#include "SessionRingOpening.h"
#include "SessionSkinDefinition.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
using Microsoft::WRL::ComPtr;
namespace SessionSketch {
namespace {
ComPtr<ID3D11Device> owner;
ComPtr<ID3D11ShaderResourceView> ring,clouds,nodeTexture,plaqueTexture;
SessionSkin::Definition selectedSkin;
std::vector<SessionSkin::Definition> skinCatalog;
std::filesystem::path skinDirectory;
std::optional<std::string> pendingSkin;
std::string skinStatus;
struct AlphaMask {
 std::vector<uint8_t> pixels;UINT width=0,height=0;
 void clear(){pixels.clear();width=height=0;}
 bool hit(float x,float y)const {
  if(x<0||y<0||x>=1||y>=1||pixels.empty())return false;
  return pixels[size_t(y*height)*width+size_t(x*width)]>=32;
 }
};
AlphaMask ringMask,cloudMask;
bool frameLayers=false;
ImFont* bodyFont=nullptr;ImFont* titleFont=nullptr;
struct ComScope {HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);~ComScope(){if(SUCCEEDED(hr))CoUninitialize();}};
bool decode(ID3D11Device* device,BYTE* data,DWORD bytes,ComPtr<ID3D11ShaderResourceView>& out,AlphaMask* mask,uint64_t& pixelBudget) {
 ComScope com;ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converted;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))||FAILED(factory->CreateStream(&stream))||FAILED(stream->InitializeFromMemory(data,bytes))||FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder))||FAILED(decoder->GetFrame(0,&frame))||FAILED(factory->CreateFormatConverter(&converted))||FAILED(converted->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 GUID container{};if(FAILED(decoder->GetContainerFormat(&container))||container!=GUID_ContainerFormatPng)return false;
 UINT w=0,h=0;if(FAILED(converted->GetSize(&w,&h))||!w||!h||w>2048||h>2048||pixelBudget+uint64_t(w)*h>8000000)return false;
 std::vector<uint8_t> pixels(size_t(w)*h*4);if(FAILED(converted->CopyPixels(nullptr,w*4,UINT(pixels.size()),pixels.data())))return false;
 if(mask){mask->width=w;mask->height=h;mask->pixels.resize(size_t(w)*h);for(size_t i=0;i<mask->pixels.size();++i)mask->pixels[i]=pixels[i*4+3];}
 D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA init{pixels.data(),w*4,0};ComPtr<ID3D11Texture2D> texture;
 if(FAILED(device->CreateTexture2D(&desc,&init,&texture))||FAILED(device->CreateShaderResourceView(texture.Get(),nullptr,&out)))return false;
 pixelBudget+=uint64_t(w)*h;return true;
}
bool load(ID3D11Device* device,int id,ComPtr<ID3D11ShaderResourceView>& out) {
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)&load,&module))return false;
 auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);if(!resource)return false;
 auto handle=LoadResource(module,resource);auto data=(BYTE*)LockResource(handle);auto bytes=SizeofResource(module,resource);if(!data||!bytes)return false;
 uint64_t budget=0;return decode(device,data,bytes,out,id==6001?&ringMask:&cloudMask,budget);
}
bool fileTexture(const std::filesystem::path& path,ComPtr<ID3D11ShaderResourceView>& out,AlphaMask* mask,uint64_t& budget){
 try{
  auto bytes=std::filesystem::file_size(path);if(!bytes||bytes>16*1024*1024)return false;
  std::vector<BYTE> data(size_t(bytes),0);std::ifstream file(path,std::ios::binary);if(!file.read(reinterpret_cast<char*>(data.data()),std::streamsize(bytes)))return false;
  return decode(owner.Get(),data.data(),DWORD(bytes),out,mask,budget);
 }catch(...){return false;}
}
void defaultSkin(std::string message={}){
 nodeTexture.Reset();plaqueTexture.Reset();ring.Reset();clouds.Reset();ringMask.clear();cloudMask.clear();selectedSkin=SessionSkin::Definition{};skinStatus=std::move(message);
 if(owner&&(!load(owner.Get(),6001,ring)||!load(owner.Get(),6002,clouds))){ring.Reset();clouds.Reset();skinStatus="Session artwork is unavailable.";}
}
void applyRequestedSkin(){
 if(!pendingSkin||!owner)return;const auto id=std::move(*pendingSkin);pendingSkin.reset();
 if(id=="default"){defaultSkin();return;}
 auto found=std::find_if(skinCatalog.begin(),skinCatalog.end(),[&](const auto& skin){return skin.id==id;});
 auto definition=found==skinCatalog.end()?std::optional<SessionSkin::Definition>{}:SessionSkin::read(found->folder);
 auto fail=[&]{defaultSkin("Skin could not be loaded. Session V1 restored.");};
 if(!definition||!SessionSkin::contained(std::filesystem::weakly_canonical(skinDirectory),definition->folder)){fail();return;}
 ComPtr<ID3D11ShaderResourceView> nextRing,nextClouds,nextNode,nextPlaque;AlphaMask nextRingMask,nextCloudMask;uint64_t budget=0;
 if(!fileTexture(definition->ring,nextRing,&nextRingMask,budget)||!fileTexture(definition->clouds,nextClouds,&nextCloudMask,budget)||(!definition->node.empty()&&!fileTexture(definition->node,nextNode,nullptr,budget))||(!definition->plaque.empty()&&!fileTexture(definition->plaque,nextPlaque,nullptr,budget))){fail();return;}
 auto clearCorners=[](const AlphaMask& mask){return !mask.hit(0,0)&&!mask.hit(.999f,0)&&!mask.hit(0,.999f)&&!mask.hit(.999f,.999f);};
 if(nextRingMask.width!=nextRingMask.height||!clearCorners(nextRingMask)||!clearCorners(nextCloudMask)){fail();return;}
 // A replacement ring must leave a genuine central opening for the real UI.
 for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)if(nextRingMask.hit(definition->centerX+x*definition->radius*.20f,definition->centerY+y*definition->radius*.20f)){fail();return;}
 ring=std::move(nextRing);clouds=std::move(nextClouds);nodeTexture=std::move(nextNode);plaqueTexture=std::move(nextPlaque);ringMask=std::move(nextRingMask);cloudMask=std::move(nextCloudMask);selectedSkin=*definition;skinStatus.clear();
}
void cloudLayer(ImDrawList* draw,ImVec2 a,ImVec2 z,ImU32 tint,bool rim){
 const float edge=selectedSkin.cloudRimOpacity*255.f;
 if(!rim||std::fabs(edge-100.f)<.01f){draw->AddImage((ImTextureID)clouds.Get(),a,z,{0,0},{1,1},tint);return;}
 // Fixed 12x12 mesh, standard ImGui shader. Only vertex alpha changes: no blur or render target.
 constexpr int steps=12,side=steps+1,count=side*side;
 struct Point {ImVec2 uv;float weight;};
 static const auto grid=[] {std::array<Point,count> result{};for(int y=0;y<side;++y)for(int x=0;x<side;++x){float u=float(x)/steps,v=float(y)/steps;float radius=std::sqrt((u-.5f)*(u-.5f)+(v-.5f)*(v-.5f));float t=std::clamp((radius-.26f)/.18f,0.f,1.f);result[y*side+x]={{u,v},t*t*(3-2*t)};}return result;}();
 draw->PushTextureID((ImTextureID)clouds.Get());draw->PrimReserve(steps*steps*6,count);const auto base=draw->_VtxCurrentIdx;
 const ImU32 rgb=tint&~(ImU32(255)<<IM_COL32_A_SHIFT);const float center=float((tint>>IM_COL32_A_SHIFT)&255);
 for(const auto& p:grid){ImU32 alpha=ImU32(std::clamp(center+(edge-center)*p.weight,0.f,255.f)+.5f);draw->PrimWriteVtx({a.x+(z.x-a.x)*p.uv.x,a.y+(z.y-a.y)*p.uv.y},p.uv,rgb|(alpha<<IM_COL32_A_SHIFT));}
 for(int y=0;y<steps;++y)for(int x=0;x<steps;++x){unsigned i=base+y*side+x;for(unsigned index:{i,i+1,i+side+1,i,i+side+1,i+side})draw->PrimWriteIdx(ImDrawIdx(index));}
 draw->PopTextureID();
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
void beginFrame(){applyRequestedSkin();auto d=ImGui::GetBackgroundDrawList();d->ChannelsSplit(2);d->ChannelsSetCurrent(1);frameLayers=true;}
void endFrame(){if(frameLayers){ImGui::GetBackgroundDrawList()->ChannelsMerge();frameLayers=false;}}
void addFonts(){
 wchar_t path[MAX_PATH]{};GetWindowsDirectoryW(path,MAX_PATH);std::filesystem::path root(path);
 auto body=root/L"Fonts"/L"segoeui.ttf",title=root/L"Fonts"/L"segoepr.ttf";
 if(std::filesystem::exists(body))bodyFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(body.string().c_str(),16.f);
 if(std::filesystem::exists(title))titleFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(title.string().c_str(),32.f);
}
void setSkinDirectory(const std::filesystem::path& directory){skinDirectory=directory;refreshSkins();pendingSkin="default";}
void refreshSkins(){
 skinCatalog.clear();try{
  if(std::filesystem::is_directory(skinDirectory))for(const auto& entry:std::filesystem::directory_iterator(skinDirectory)){
   if(skinCatalog.size()>=64)break;if(!entry.is_directory()||entry.is_symlink())continue;
   if(auto skin=SessionSkin::read(entry.path());skin&&SessionSkin::contained(std::filesystem::weakly_canonical(skinDirectory),skin->folder))skinCatalog.push_back(*skin);
  }
  std::sort(skinCatalog.begin(),skinCatalog.end(),[](const auto& a,const auto& b){return a.name<b.name;});
 }catch(...){skinCatalog.clear();}
 if(selectedSkin.id!="default")pendingSkin=std::any_of(skinCatalog.begin(),skinCatalog.end(),[](const auto& skin){return skin.id==selectedSkin.id;})?selectedSkin.id:std::string("default");
}
std::vector<SkinChoice> availableSkins(){std::vector<SkinChoice> result{{"default","Session V1"}};for(const auto& s:skinCatalog)result.push_back({s.id,s.name,!s.node.empty(),!s.plaque.empty()});return result;}
SkinChoice activeSkin(){return {selectedSkin.id,selectedSkin.name,bool(nodeTexture),bool(plaqueTexture)};}
SkinOpening skinOpening(){return {{selectedSkin.centerX,selectedSkin.centerY},selectedSkin.radius,selectedSkin.radius*(1.f-selectedSkin.insetRatio)};}
void requestSkin(std::string id){pendingSkin=std::move(id);}
std::string skinMessage(){return skinStatus;}
void prepare(ID3D11Device* device){
 if(owner.Get()==device)return;const auto restore=pendingSkin.value_or(selectedSkin.id);pendingSkin.reset();owner=device;
 if(skinDirectory.empty()){
  HMODULE module=nullptr;wchar_t path[32768]{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)&load,&module)){const DWORD length=GetModuleFileNameW(module,path,DWORD(std::size(path)));if(length>0&&length<std::size(path))skinDirectory=std::filesystem::path(path).parent_path()/L"skins";}
 }
 defaultSkin();refreshSkins();if(restore!="default")pendingSkin=restore;
}
void reset(){frameLayers=false;ring.Reset();clouds.Reset();nodeTexture.Reset();plaqueTexture.Reset();ringMask.clear();cloudMask.clear();owner.Reset();bodyFont=titleFont=nullptr;if(!pendingSkin)pendingSkin=selectedSkin.id;}
bool ready(){return ring&&clouds;}
ImFont* font(){return bodyFont?bodyFont:ImGui::GetFont();}
Layout layout(const State& s){
 const float d=std::min(s.size.x/1.5f,s.size.y);
 const ImVec2 frameCenter{s.position.x+s.size.x*.5f,s.position.y+s.size.y*.5f};
 const ImVec2 a{frameCenter.x-d*.5f,frameCenter.y-d*.5f};
 const auto opening=skinOpening();const ImVec2 c{a.x+d*opening.center.x,a.y+d*opening.center.y};
 const float area=opening.radius/SessionRingOpening::radius;
 return {c,a,{a.x+d,a.y+d},{c.x-d*.225f*area,c.y-d*.21f*area},{d*.45f*area,d*.42f*area},d};
}
NavigationNode navigationNode(const State& s,int index){
 const auto l=layout(s);index=std::clamp(index,0,3);
 const bool left=(index==0||index==2),top=index<2;
 const ImVec2 c{std::clamp(l.center.x+(left?-.48f:.48f)*l.diameter,s.position.x+l.diameter*.24f,s.position.x+s.size.x-l.diameter*.24f),
 std::clamp(l.center.y+(top?-.38f:.38f)*l.diameter,s.position.y+l.diameter*.04f,s.position.y+s.size.y-l.diameter*.04f)};
 const float r=l.diameter*.04f;
 return {c,{c.x-r-(left?l.diameter*.20f:0.f),c.y-r},
  {c.x+r+(left?0.f:l.diameter*.20f),c.y+r},r,left};
}
static void renderNavigation(State& s,bool inputs){
 const auto l=layout(s);const char* labels[]={"Practice","Control","Credits","Quick Menu"};
 for(int i=0;i<4;++i){
  const auto n=navigationNode(s,i);char id[48];snprintf(id,sizeof(id),"##SessionNavigation%d",i);
  ImGui::SetNextWindowPos(n.minimum);ImGui::SetNextWindowSize({n.maximum.x-n.minimum.x,n.maximum.y-n.minimum.y});
  auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoBringToFrontOnFocus;
  if(!inputs)flags|=ImGuiWindowFlags_NoInputs;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.f);
  ImGui::Begin(id,nullptr,flags);ImGui::SetCursorPos({0,0});
  ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0,0,0,0});ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0,0,0,0});
  if(ImGui::Button("##select-section",{n.maximum.x-n.minimum.x,n.maximum.y-n.minimum.y}))s.tab=i;
  ImGui::PopStyleColor(3);const bool selected=s.tab==i,hover=ImGui::IsItemHovered()||ImGui::IsItemFocused();
  auto draw=ImGui::GetWindowDrawList();const float h=n.radius*.58f,bevel=n.radius*.28f;
  const float x0=n.left?n.minimum.x:n.center.x,x1=n.left?n.center.x:n.maximum.x;
  const ImVec2 plate[]={{x0+bevel,n.center.y-h},{x1-bevel,n.center.y-h},{x1,n.center.y-h+bevel},{x1,n.center.y+h-bevel},{x1-bevel,n.center.y+h},{x0+bevel,n.center.y+h},{x0,n.center.y+h-bevel},{x0,n.center.y-h+bevel}};
  if(plaqueTexture)draw->AddImage((ImTextureID)plaqueTexture.Get(),{x0,n.center.y-h},{x1,n.center.y+h});else draw->AddConvexPolyFilled(plate,8,IM_COL32(22,39,48,242));
  draw->AddPolyline(plate,8,selected||hover?IM_COL32(99,224,244,255):IM_COL32(137,173,186,235),ImDrawFlags_Closed,std::max(1.f,l.diameter/650.f));
  if(nodeTexture)draw->AddImage((ImTextureID)nodeTexture.Get(),{n.center.x-n.radius,n.center.y-n.radius},{n.center.x+n.radius,n.center.y+n.radius});else{
  draw->AddCircleFilled(n.center,n.radius,IM_COL32(35,54,63,255),48);
  draw->AddCircle(n.center,n.radius*.93f,IM_COL32(180,207,213,255),48,std::max(1.f,l.diameter/500.f));
  draw->AddCircleFilled(n.center,n.radius*.77f,selected?IM_COL32(17,143,174,255):IM_COL32(44,91,111,255),48);
  draw->AddCircle(n.center,n.radius*.73f,IM_COL32(125,223,239,255),48,1.1f);
  draw->AddCircleFilled(n.center,n.radius*.55f,selected||hover?IM_COL32(92,220,244,255):IM_COL32(91,154,177,255),48);
  draw->AddCircleFilled(n.center,n.radius*.33f,selected||hover?IM_COL32(220,252,255,255):IM_COL32(173,214,224,255),40);
  for(int tick=0;tick<8;++tick){float a=tick*3.14159265f/4.f;draw->AddLine({n.center.x+std::cos(a)*n.radius*.83f,n.center.y+std::sin(a)*n.radius*.83f},{n.center.x+std::cos(a)*n.radius*.92f,n.center.y+std::sin(a)*n.radius*.92f},IM_COL32(218,234,238,255),1.f);}
  }
  if(nodeTexture&&(selected||hover))draw->AddCircle(n.center,n.radius*.96f,IM_COL32(150,241,255,255),48,std::max(1.f,l.diameter/450.f));
  const float textSize=18.f*l.diameter/900.f;auto textFont=font();const auto extent=textFont->CalcTextSizeA(textSize,10000,0,labels[i]);
  const float tx=n.left?(n.minimum.x+n.center.x-n.radius)*.5f:(n.center.x+n.radius+n.maximum.x)*.5f;
  if(plaqueTexture)draw->AddRectFilled({tx-extent.x*.5f-3,n.center.y-extent.y*.5f-2},{tx+extent.x*.5f+3,n.center.y+extent.y*.5f+2},IM_COL32(12,29,38,205),3);
  draw->AddText(textFont,textSize,{tx-extent.x*.5f,n.center.y-extent.y*.5f},IM_COL32(238,250,252,255),labels[i]);
  if(ImGui::IsItemHovered())ImGui::SetTooltip("Show %s",labels[i]);
  ImGui::End();ImGui::PopStyleVar(2);
 }
}
void clamp(State& s,ImVec2 screen){
 if(!s.initialized){
  const float side=std::min({SessionAppearanceDefaults::menuSize.y,(screen.y-24.f)*.9f,(screen.x-24.f)/1.5f*.9f});
  s.size={side*1.5f,side};
  s.position={screen.x*SessionAppearanceDefaults::normalizedFrameCenter.x-s.size.x*.5f,screen.y*SessionAppearanceDefaults::normalizedFrameCenter.y-s.size.y*.5f};
  s.initialized=true;
 }
 const float available=std::max(1.f,std::min((screen.x-16.f)/1.5f,screen.y-16.f));
 const float side=std::clamp(std::min(s.size.x/1.5f,s.size.y),std::min(600.f,available),available);
 s.size={side*1.5f,side};
 s.position.x=std::clamp(s.position.x,0.f,std::max(0.f,screen.x-s.size.x));s.position.y=std::clamp(s.position.y,0.f,std::max(0.f,screen.y-side));
}
bool hit(const State& s,ImVec2 point){
 const auto l=layout(s);if(l.diameter<=0)return false;
 const float x=(point.x-l.artMin.x)/l.diameter,y=(point.y-l.artMin.y)/l.diameter;
 for(int i=0;i<4;++i){const auto n=navigationNode(s,i);if(point.x>=n.minimum.x&&point.y>=n.minimum.y&&point.x<n.maximum.x&&point.y<n.maximum.y)return true;}
 const float dx=x-.5f,dy=y-.5f;
 const auto opening=skinOpening();const float ox=x-opening.center.x,oy=y-opening.center.y;if(ox*ox+oy*oy<=opening.radius*opening.radius)return true;
 // Keep the capture boundary steady while the optional visual layers move.
 return ringMask.hit(x,y)||cloudMask.hit(x,y);
}
void render(State& s,ImVec2 screen,bool inputs,bool& open,const std::function<void(int)>& contents){
 if(!open)return;
 clamp(s,screen);const auto l=layout(s);auto a=s.position;
 backdrop(s);
 ImGui::SetNextWindowPos(a);ImGui::SetNextWindowSize(s.size);
 auto captureFlags=ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoBringToFrontOnFocus;
 if(!inputs||!hit(s,ImGui::GetIO().MousePos))captureFlags|=ImGuiWindowFlags_NoInputs;
 ImGui::Begin("##SessionSilhouette",nullptr,captureFlags);ImGui::End();
 Styles styles;renderNavigation(s,inputs);
 if(s.measuredRevision!=s.contentRevision){s.contentHeights.fill(0);s.contentWidths.fill(0);s.measuredRevision=s.contentRevision;}
 s.tab=std::clamp(s.tab,0,3);
 const float groupScale=std::clamp(l.diameter/900.f,.82f,1.15f);
 const float chrome=102.f*groupScale;
 const float height=s.contentHeights[s.tab]>0?std::min(l.controlsSize.y,chrome+std::max(28.f*groupScale,s.contentHeights[s.tab])):l.controlsSize.y;
 const ImVec2 groupPosition{l.center.x-l.controlsSize.x*.5f,l.center.y-height*.5f};
 ImGui::SetNextWindowPos(groupPosition);ImGui::SetNextWindowSize({l.controlsSize.x,height});
 auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse;
 if(!inputs)flags|=ImGuiWindowFlags_NoInputs;
 ImGui::Begin("Session###PCM",nullptr,flags);
 if(bodyFont)ImGui::PushFont(bodyFont);
 float scale=std::clamp(l.diameter/900.f,.82f,1.15f);ImGui::SetWindowFontScale(scale);
 float width=ImGui::GetContentRegionAvail().x;
 auto titleFace=titleFont?titleFont:ImGui::GetFont();
 const float titleWidth=titleFace->CalcTextSizeA(30*scale,10000,0,"Session").x+12*scale;
 const float headerWidth=titleWidth+120*scale+16.f;
 ImGui::SetCursorPosX(std::max(6.f,(ImGui::GetWindowWidth()-headerWidth)*.5f));auto titlePos=ImGui::GetCursorScreenPos();
 ImGui::InvisibleButton("##drag-session",{titleWidth,43*scale});
 if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0)){s.position.x+=ImGui::GetIO().MouseDelta.x;s.position.y+=ImGui::GetIO().MouseDelta.y;}
 ImGui::GetWindowDrawList()->AddText(titleFont?titleFont:ImGui::GetFont(),30*scale,titlePos,IM_COL32(39,53,59,255),"Session");
 if(ImGui::IsItemHovered())ImGui::SetTooltip("Drag to move Session");
 ImGui::SameLine();if(ImGui::Button("Appearance",{92*scale,28*scale}))ImGui::OpenPopup("##session-appearance");
 if(ImGui::IsItemHovered())ImGui::SetTooltip("Cloud motion, background opacity and menu size");
 ImGui::SameLine();if(ImGui::Button("X##close-session",{28*scale,28*scale}))open=false;
 if(ImGui::IsItemHovered())ImGui::SetTooltip("Close menu");
 if(ImGui::BeginPopup("##session-appearance")){
  ImGui::TextUnformatted("Appearance");ImGui::Separator();
  if(ImGui::RadioButton("Static clouds",!s.animateClouds))s.animateClouds=false;
  if(ImGui::RadioButton("Gentle motion",s.animateClouds))s.animateClouds=true;
  float opacity=s.backgroundOpacity*100.f;
  ImGui::SetNextItemWidth(170*scale);
  if(ImGui::SliderFloat("Background opacity",&opacity,50.f,100.f,"%.0f%%"))s.backgroundOpacity=opacity/100.f;
  const float maxDiameter=std::max(1.f,std::min((screen.x-16.f)/1.5f,screen.y-16.f));
  float diameter=l.diameter;
  ImGui::SetNextItemWidth(170*scale);
  if(ImGui::SliderFloat("Menu size",&diameter,std::min(600.f,maxDiameter),maxDiameter,"%.0f px"))s.size={diameter*1.5f,diameter};
  ImGui::Separator();ImGui::TextUnformatted("Wheel and segments");
  auto tint=[&](const char* name,const char* opacityName,ImVec4& color){
   ImGui::SetNextItemWidth(170*scale);ImGui::ColorEdit3(name,&color.x,ImGuiColorEditFlags_NoInputs);
   float alpha=color.w*100.f;ImGui::SetNextItemWidth(170*scale);
   if(ImGui::SliderFloat(opacityName,&alpha,0.f,100.f,"%.0f%%"))color.w=alpha/100.f;
  };
  tint("Wheel color","Wheel opacity",s.wheel.background);
  tint("Segment color","Segment opacity",s.wheel.segment);
  tint("Selected color","Selected opacity",s.wheel.selected);
  ImGui::Separator();ImGui::TextUnformatted("Skin");
  if(ImGui::BeginCombo("Artwork",selectedSkin.name.c_str())){
   for(const auto& choice:availableSkins()){ImGui::PushID(choice.id.c_str());if(ImGui::Selectable(choice.name.c_str(),selectedSkin.id==choice.id)){requestSkin(choice.id);++s.contentRevision;}ImGui::PopID();}
   ImGui::EndCombo();
  }
  if(ImGui::Button("Refresh skins"))refreshSkins();
  if(!skinStatus.empty())ImGui::TextWrapped("%s",skinStatus.c_str());
  ImGui::EndPopup();
 }
 const char* sections[]={"Practice","Control","Credits","Quick Menu"};
 const float headingWidth=ImGui::CalcTextSize(sections[s.tab]).x;
 ImGui::SetCursorPosX(std::max(6.f,(ImGui::GetWindowWidth()-headingWidth)*.5f));ImGui::TextDisabled("%s",sections[s.tab]);ImGui::Spacing();
 ImGui::PushID(s.tab);ImGui::PushID(s.contentRevision);
 ImGui::BeginChild("##session-scroll",{0,-26*scale},false,ImGuiWindowFlags_HorizontalScrollbar);
 ImGui::PushItemWidth(120*scale);
 const float available=ImGui::GetContentRegionAvail().x;
 if(s.contentWidths[s.tab]>0&&s.contentWidths[s.tab]<available)ImGui::SetCursorPosX(ImGui::GetCursorPosX()+(available-s.contentWidths[s.tab])*.5f);
 ImGui::BeginGroup();contents(s.tab);ImGui::EndGroup();
 const auto contentSize=ImGui::GetItemRectSize();s.contentHeights[s.tab]=contentSize.y+8*scale;s.contentWidths[s.tab]=contentSize.x;
 ImGui::PopItemWidth();
 ImGui::EndChild();ImGui::PopID();ImGui::PopID();
 ImGui::SetCursorPosX(std::max(6.f,(ImGui::GetWindowWidth()-ImGui::CalcTextSize("Drag title to move").x)*.5f));ImGui::TextDisabled("Drag title to move");ImGui::SameLine(std::max(0.f,width-26*scale));
 ImGui::InvisibleButton("##resize-session",{24*scale,22*scale});auto q=ImGui::GetItemRectMin();auto e=ImGui::GetItemRectMax();ImGui::GetWindowDrawList()->AddLine({q.x,e.y},{e.x,q.y},IM_COL32(0,153,174,255),2);
 if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0)){const auto delta=ImGui::GetIO().MouseDelta;const float grow=(delta.x/.98f+delta.y/.72f)*.5f;const float diameter=l.diameter+grow;s.size={diameter*1.5f,diameter};}
 if(ImGui::IsItemHovered()){ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);ImGui::SetTooltip("Drag to resize");}
 ImGui::SetWindowFontScale(1.f);if(bodyFont)ImGui::PopFont();ImGui::End();clamp(s,screen);
}
void backdrop(const State& s) {
 auto draw=ImGui::GetBackgroundDrawList();if(frameLayers)draw->ChannelsSetCurrent(0);
 const auto l=layout(s);
 draw->PushClipRect(l.artMin,l.artMax,true);
 // A stable 90% backing carries the text; cloud alpha adds texture, never contrast flicker.
 draw->AddCircleFilled(l.center,l.diameter*skinOpening().radius*(.34f/SessionRingOpening::radius),IM_COL32(240,247,248,int(std::clamp(s.backgroundOpacity,.50f,1.f)*255.f+.5f)),96);
 if(ready()){
  const double t=ImGui::GetTime()*SessionAppearanceDefaults::cloudSpeedMultiplier;
  const float x=s.animateClouds?float(std::sin(t*.035))*l.diameter*.007f:0.f;
  const float y=s.animateClouds?float(std::sin(t*.027))*l.diameter*.004f:0.f;
  auto layer=[&](float dx,float dy,ImU32 tint,bool rim=false){cloudLayer(draw,{l.artMin.x+dx,l.artMin.y+dy},{l.artMax.x+dx,l.artMax.y+dy},tint,rim);};
  layer(x,y,IM_COL32(255,255,255,100),true);
  // Same two immutable texture draws in both modes; no shader, blur or per-frame upload.
  layer(-x*.65f,-y*.8f,IM_COL32(224,243,250,45));
  draw->AddImage((ImTextureID)ring.Get(),l.artMin,l.artMax);
 }else draw->AddCircle(l.center,l.diameter*.42f,IM_COL32(170,224,235,255),96,8.f);
 draw->PopClipRect();
 if(frameLayers)draw->ChannelsSetCurrent(1);
}
}
