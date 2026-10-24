#pragma once
#include "imgui.h"
#include "PracticeDashboard.h"
#include "SessionSketch.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <array>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#pragma comment(lib,"dwrite.lib")
#pragma comment(lib,"d2d1.lib")
#pragma comment(lib,"windowscodecs.lib")
namespace speedoverlay {
using Microsoft::WRL::ComPtr;
struct Options {bool visible=false;float height=.045f;ImVec4 color{1,1,1,1};ImVec2 position{.80f,.13f};};
struct Reading {bool available=false;float speed=0;};
struct Capture {bool cursor=false,pointer=false;};
inline Capture capture(bool visible,bool edit,bool available,bool held,bool& wasEditing,bool& releaseGuard){
 const bool editing=visible&&edit&&available;if(wasEditing&&!editing&&held)releaseGuard=true;if(!held)releaseGuard=false;wasEditing=editing;return {editing,editing||releaseGuard};
}
inline Reading reading(unsigned game,bool ingame,bool verified,bool loaded,bool reported,const H3sbStatus& status,uint64_t now,uint64_t transition){
 if(!ingame||!verified||(game!=2&&game!=5&&game!=6)||!reported||status.size<sizeof status||status.updated_ms<=transition||status.game!=(game==5?2:1))return {};
 auto view=practiceui::describe(loaded,reported,status,now,{});return {view.speedAvailable,view.speedAvailable?view.speed:0};
}
inline void sanitize(Options& o){
 o.height=std::isfinite(o.height)?std::clamp(o.height,.015f,.50f):.045f;
 for(float* v:{&o.position.x,&o.position.y})*v=std::isfinite(*v)?std::clamp(*v,0.f,1.f):.5f;
 for(float* v:{&o.color.x,&o.color.y,&o.color.z,&o.color.w})*v=std::isfinite(*v)?std::clamp(*v,0.f,1.f):1.f;
}
inline int heightKey(float pixels){return std::clamp(int(std::ceil(pixels/16.f))*16,16,1088);}
struct Glyph {float x=0,y=0,w=0,h=0,bearingX=0,bearingY=0,advance=0;};
struct Raster {
 int key=0,width=0,height=0;float ascent=0,descent=0,digitAdvance=0;double buildMs=0;
 std::array<Glyph,12> glyphs{};std::vector<uint32_t> pixels;std::string error;
 float inkHeight()const{return ascent+descent;}
};
inline int index(char c){return c>='0'&&c<='9'?c-'0':c=='.'?10:11;}
inline void checked(HRESULT result){if(FAILED(result))throw std::runtime_error("Speed font rendering is unavailable ("+std::to_string(uint32_t(result))+")");}
// CPU-only DirectWrite/Direct2D work. The worker never touches ImGui, game memory
// or a graphics device. Only 12 numeric glyphs are rasterized, once per size key.
inline std::unique_ptr<Raster> rasterize(int key){
 auto out=std::make_unique<Raster>();out->key=key;auto start=std::chrono::steady_clock::now();
 const auto com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 try{
  ComPtr<IDWriteFactory> write;checked(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(write.GetAddressOf())));
  ComPtr<IDWriteFontCollection> fonts;checked(write->GetSystemFontCollection(&fonts));UINT32 familyIndex=0;BOOL found=FALSE;checked(fonts->FindFamilyName(L"Segoe UI",&familyIndex,&found));if(!found)throw std::runtime_error("Segoe UI is unavailable");
  ComPtr<IDWriteFontFamily> family;ComPtr<IDWriteFont> font;ComPtr<IDWriteFontFace> face;checked(fonts->GetFontFamily(familyIndex,&family));checked(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STRETCH_NORMAL,DWRITE_FONT_STYLE_NORMAL,&font));checked(font->CreateFontFace(&face));
  DWRITE_FONT_METRICS fm{};face->GetMetrics(&fm);UINT32 nine='9';UINT16 glyphIndex=0;checked(face->GetGlyphIndices(&nine,1,&glyphIndex));DWRITE_GLYPH_METRICS gm{};checked(face->GetDesignGlyphMetrics(&glyphIndex,1,&gm));
  const float ink=float(gm.advanceHeight-gm.topSideBearing-gm.bottomSideBearing);if(ink<=0)throw std::runtime_error("Speed font metrics are invalid");const float em=std::ceil(key*float(fm.designUnitsPerEm)/ink);
  ComPtr<IDWriteTextFormat> format;checked(write->CreateTextFormat(L"Segoe UI",nullptr,DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,em,L"en-US",&format));checked(format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
  ComPtr<IWICImagingFactory> wic;checked(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&wic)));ComPtr<ID2D1Factory> d2d;checked(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,d2d.GetAddressOf()));
  struct Tile {int w=0,h=0;std::vector<uint32_t> pixels;};std::array<Tile,12> tiles;const wchar_t chars[]=L"0123456789.-";int totalWidth=0;
  for(int i=0;i<12;++i){
   ComPtr<IDWriteTextLayout> layout;checked(write->CreateTextLayout(chars+i,1,format.Get(),10000,10000,&layout));DWRITE_TEXT_METRICS metrics{};checked(layout->GetMetrics(&metrics));DWRITE_LINE_METRICS line{};UINT32 lines=0;checked(layout->GetLineMetrics(&line,1,&lines));
   constexpr int pad=8;int w=int(std::ceil(metrics.widthIncludingTrailingWhitespace))+pad*2+2,h=int(std::ceil(metrics.height))+pad*2+2;if(w<=0||h<=0||w>4096||h>4096)throw std::runtime_error("Speed font exceeds the 4K rendering bound");
   ComPtr<IWICBitmap> bitmap;checked(wic->CreateBitmap(w,h,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnLoad,&bitmap));ComPtr<ID2D1RenderTarget> target;auto props=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96,96);checked(d2d->CreateWicBitmapRenderTarget(bitmap.Get(),props,&target));target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);ComPtr<ID2D1SolidColorBrush> brush;checked(target->CreateSolidColorBrush(D2D1::ColorF(1,1,1,1),&brush));
   target->BeginDraw();target->Clear(D2D1::ColorF(0,0,0,0));target->DrawTextLayout({float(pad),float(pad)},layout.Get(),brush.Get(),D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);checked(target->EndDraw());
   std::vector<uint32_t> pixels(size_t(w)*h);checked(bitmap->CopyPixels(nullptr,w*4,UINT(pixels.size()*4),reinterpret_cast<BYTE*>(pixels.data())));
   int x0=w,y0=h,x1=-1,y1=-1;for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(pixels[size_t(y)*w+x]>>24){x0=std::min(x0,x);y0=std::min(y0,y);x1=std::max(x1,x);y1=std::max(y1,y);}if(x1<x0)throw std::runtime_error("Speed font glyph is empty");
   auto& tile=tiles[i];tile.w=x1-x0+1;tile.h=y1-y0+1;tile.pixels.resize(size_t(tile.w)*tile.h);for(int y=0;y<tile.h;++y)for(int x=0;x<tile.w;++x)tile.pixels[size_t(y)*tile.w+x]=0x00FFFFFF|(pixels[size_t(y+y0)*w+x+x0]&0xFF000000);
   auto& g=out->glyphs[i];g.w=float(tile.w);g.h=float(tile.h);g.bearingX=float(x0-pad);g.bearingY=float(y0-pad)-line.baseline;g.advance=metrics.widthIncludingTrailingWhitespace;
   if(i<10){out->ascent=std::max(out->ascent,-g.bearingY);out->descent=std::max(out->descent,g.bearingY+g.h);out->digitAdvance=std::max(out->digitAdvance,g.advance);}totalWidth+=tile.w+4;
  }
  out->width=std::min(4096,totalWidth);int x=2,y=2,rowHeight=0;for(auto& g:out->glyphs){if(x+int(g.w)+2>out->width){y+=rowHeight+4;x=2;rowHeight=0;}g.x=float(x);g.y=float(y);x+=int(g.w)+4;rowHeight=std::max(rowHeight,int(g.h));}out->height=y+rowHeight+2;
  const uint64_t bytes=uint64_t(out->width)*out->height*4;if(bytes>64*1024*1024)throw std::runtime_error("Speed font exceeds its 64 MiB cache budget");out->pixels.assign(size_t(out->width)*out->height,0x00FFFFFF);
  for(int i=0;i<12;++i){auto& g=out->glyphs[i];auto& t=tiles[i];for(int yy=0;yy<t.h;++yy)std::copy_n(t.pixels.data()+size_t(yy)*t.w,t.w,out->pixels.data()+size_t(int(g.y)+yy)*out->width+int(g.x));}
 }catch(const std::exception& e){out->error=e.what();out->pixels.clear();}
 if(SUCCEEDED(com))CoUninitialize();out->buildMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return out;
}
class Worker {
 std::mutex mutex;std::condition_variable cv;int requested=0;uint64_t generation=0;bool closing=false;std::unique_ptr<Raster> ready;std::thread thread;
public:
 Worker():thread([this]{for(;;){std::unique_lock lock(mutex);cv.wait(lock,[&]{return closing||requested;});if(closing)return;const int key=requested;const auto epoch=generation;requested=0;lock.unlock();auto result=rasterize(key);lock.lock();if(!closing&&generation==epoch)ready=std::move(result);}}){}
 ~Worker(){{std::lock_guard lock(mutex);closing=true;}cv.notify_one();thread.join();}
 void request(int key){std::lock_guard lock(mutex);++generation;requested=key;cv.notify_one();}
 void cancel(){std::lock_guard lock(mutex);++generation;requested=0;ready.reset();}
 std::unique_ptr<Raster> take(){std::lock_guard lock(mutex);return std::move(ready);}
};
struct Metrics {ImVec2 minimum{},maximum{};float digitHeight=0,inkWidth=0;bool capturing=false;};
struct Stats {uint64_t bytes=0;unsigned uploads=0,builds=0;double buildMs=0,uploadMs=0;int key=0;};
class Display {
 Worker worker;ComPtr<ID3D11Device> device;ComPtr<ID3D11ShaderResourceView> texture;std::unique_ptr<Raster> atlas;
 int desired=0,queued=0,failed=0;bool wasVisible=false;double changed=0;Stats data;std::string error;
public:
 const Stats& stats()const{return data;}const std::string& status()const{return error;}
 bool ready()const{return bool(texture);}
 void prepare(ID3D11Device* next,int key,double now){
  if(device.Get()!=next){worker.cancel();device=next;texture.Reset();atlas.reset();desired=queued=failed=0;}
  if(key!=desired){desired=key;changed=now;failed=0;}
  if(auto result=worker.take()){++data.builds;data.buildMs=result->buildMs;if(result->key==queued)queued=0;if(result->key==desired){if(!result->error.empty()){error=result->error;failed=desired;}else if(device){
   auto uploadStart=std::chrono::steady_clock::now();D3D11_TEXTURE2D_DESC desc{};desc.Width=result->width;desc.Height=result->height;desc.ArraySize=desc.MipLevels=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA input{result->pixels.data(),UINT(result->width*4),0};ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11ShaderResourceView> srv;
   if(SUCCEEDED(device->CreateTexture2D(&desc,&input,&image))&&SUCCEEDED(device->CreateShaderResourceView(image.Get(),nullptr,&srv))){texture=std::move(srv);data.bytes=uint64_t(result->width)*result->height*4;data.key=result->key;data.uploadMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-uploadStart).count();++data.uploads;result->pixels.clear();result->pixels.shrink_to_fit();atlas=std::move(result);error.clear();}else{error="Speed font texture could not be created";failed=desired;}
  }}}
  if(device&&(!atlas||atlas->key!=desired)&&!queued&&failed!=desired&&(!atlas||now-changed>=.25)){queued=desired;worker.request(desired);}
 }
 Metrics render(Options& options,const Reading& speed,ImVec2 screen,ID3D11Device* graphics,bool& edit,bool interactive=true){
  Metrics m;sanitize(options);if(!options.visible){if(wasVisible){worker.cancel();desired=queued=failed=0;}wasVisible=false;edit=false;return m;}wasVisible=true;if(!std::isfinite(screen.x)||!std::isfinite(screen.y)||screen.x<64||screen.y<64){worker.cancel();desired=queued=failed=0;edit=false;return m;}const auto dpi=ImGui::GetIO().DisplayFramebufferScale;const float dx=std::max(1.f,dpi.x),dy=std::max(1.f,dpi.y);
  const float target=std::min(screen.y*options.height,1080.f/dy);prepare(graphics,heightKey(target*dy),ImGui::GetTime());
  const float label=18.f;auto face=SessionSketch::textFont(label);const float captionWidth=face->CalcTextSizeA(label,10000,0,"Speed (wu/s) - unavailable").x;
  const ImVec2 field{std::min(screen.x-16,std::max(target*4.5f,captionWidth+4)),target+label+4};ImVec2 center{std::clamp(options.position.x*screen.x,field.x*.5f+8,screen.x-field.x*.5f-8),std::clamp(options.position.y*screen.y,field.y*.5f+8,screen.y-field.y*.5f-8)};
  if(edit)center.y=std::min(center.y,screen.y-field.y*.5f-44);
  m.minimum={center.x-field.x*.5f,center.y-field.y*.5f};m.maximum={m.minimum.x+field.x,m.minimum.y+field.y};
  if(edit&&interactive){ImGui::SetNextWindowPos({m.minimum.x-4,m.minimum.y-4});ImGui::SetNextWindowSize({field.x+8,field.y+40});auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoNavInputs|ImGuiWindowFlags_NoNavFocus;ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{4,4});ImGui::Begin("##speed-overlay-position",nullptr,flags);ImGui::InvisibleButton("Drag speed overlay",field);m.capturing=ImGui::IsItemActive();if(m.capturing&&ImGui::IsMouseDragging(0)){auto delta=ImGui::GetIO().MouseDelta;options.position.x=std::clamp((center.x+delta.x)/screen.x,0.f,1.f);options.position.y=std::clamp((center.y+delta.y)/screen.y,0.f,1.f);}if(ImGui::Button("Lock position")||ImGui::IsKeyPressed(ImGuiKey_Escape,false))edit=false;ImGui::GetWindowDrawList()->AddRect(m.minimum,m.maximum,IM_COL32(120,220,240,200));ImGui::End();ImGui::PopStyleVar();}
  auto draw=ImGui::GetBackgroundDrawList();ImVec4 tint=speed.available?options.color:ImVec4{.65f,.68f,.70f,options.color.w};const auto color=ImGui::ColorConvertFloat4ToU32(tint);
  const char* caption=speed.available?"Speed (wu/s)":"Speed (wu/s) - unavailable";
  auto size=face->CalcTextSizeA(label,10000,0,caption);draw->AddText(face,label,{center.x-size.x*.5f,m.maximum.y-label},color,caption);
  if(!atlas||!texture){const char* text=error.empty()?"Preparing speed font...":"Speed font unavailable";draw->AddText(face,label,{m.minimum.x,m.minimum.y},color,text);return m;}
  char buffer[64];if(speed.available)std::snprintf(buffer,sizeof buffer,"%.2f",double(speed.speed));else std::snprintf(buffer,sizeof buffer,"--");std::string value(buffer);
  float physical=std::min(1.f,target*dy/atlas->inkHeight()),width=0;for(char c:value)width+=(c>='0'&&c<='9'?atlas->digitAdvance:atlas->glyphs[index(c)].advance)*physical/dx;
  const float fit=std::min(1.f,field.x/std::max(width,1.f));physical*=fit;width*=fit;m.inkWidth=width;m.digitHeight=atlas->inkHeight()*physical/dy;
  float x=center.x-width*.5f;const float baseline=m.minimum.y+(target-m.digitHeight)*.5f+atlas->ascent*physical/dy;
  for(char c:value){const auto& g=atlas->glyphs[index(c)];ImVec2 a{x+g.bearingX*physical/dx,baseline+g.bearingY*physical/dy},b{a.x+g.w*physical/dx,a.y+g.h*physical/dy};draw->AddImage((ImTextureID)texture.Get(),a,b,{g.x/atlas->width,g.y/atlas->height},{(g.x+g.w)/atlas->width,(g.y+g.h)/atlas->height},color);x+=(c>='0'&&c<='9'?atlas->digitAdvance:g.advance)*physical/dx;}
  return m;
 }
};
}
