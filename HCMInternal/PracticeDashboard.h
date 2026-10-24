#pragma once
#include "imgui.h"
#include "h3sb_api.h"
#include "SessionSketch.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
namespace practiceui {
enum class Branch {Unloaded,Stale,Flying,PhysicsOff,MissingPlayer,Invalid,Valid};
struct View {Branch branch;bool speedAvailable=false,bounceAvailable=false,on=false;float speed=0,threshold=0,fraction=0;std::string reason,map;};
inline View describe(bool loaded,bool reported,const H3sbStatus& s,uint64_t now,const std::string& error){
 View v{};size_t count=0;while(count<sizeof s.map&&s.map[count])++count;v.map.assign(s.map,count);
 if(!loaded){v.branch=Branch::Unloaded;v.reason=error.empty()?"Halo 2 physics is off":error;return v;}
 if(!reported||s.updated_ms>now||now-s.updated_ms>=1500){v.branch=Branch::Stale;v.reason="Waiting for a fresh physics tick (paused, menu or loading).";return v;}
 if(s.flying){v.branch=Branch::Flying;v.reason="FLYING - bounce state unavailable";v.speedAvailable=std::isfinite(s.speed)&&s.speed>=0;v.speed=s.speed;return v;}
 if(s.mode==0){v.branch=Branch::PhysicsOff;v.reason="Halo 2 physics is off (fly is ready)";return v;}
 if(!s.have_body){v.branch=Branch::MissingPlayer;v.reason="Waiting for the player";return v;}
 if(s.game<1||s.game>2||s.mode<1||s.mode>3||!std::isfinite(s.speed)||s.speed<0||!std::isfinite(s.sweep_speed)||s.sweep_speed<0){v.branch=Branch::Invalid;v.reason="Physics telemetry is unavailable";return v;}
 v.branch=Branch::Valid;v.speedAvailable=v.bounceAvailable=true;v.on=s.armed!=0;v.speed=s.speed;v.threshold=s.sweep_speed;v.fraction=s.sweep_speed>0?std::clamp(s.speed/s.sweep_speed,0.f,1.f):0.f;
 if(!v.on)v.reason=s.asleep?"asleep":"not armed";return v;
}
struct Metrics {float numberWidth=0,numberHeight=0,numberFontSize=0,numberInkWidth=0,barHeight=0,bounceFont=0;ImVec2 numberMin{},numberMax{},bounceCenter{};};
inline Metrics render(const View& view){
 Metrics metrics;const float base=ImGui::GetFontSize(),width=std::max(1.f,ImGui::GetContentRegionAvail().x);auto draw=ImGui::GetWindowDrawList();
 auto row=[&](const std::string& text,float size,ImU32 color,bool outline=false){const float rowHeight=size;auto face=SessionSketch::textFont(size);auto measured=face->CalcTextSizeA(size,100000,0,text.c_str());if(measured.x>width)size*=width/measured.x;auto extent=face->CalcTextSizeA(size,100000,0,text.c_str());auto p=ImGui::GetCursorScreenPos();auto at=ImVec2{p.x+(width-extent.x)*.5f,p.y+(rowHeight-size)*.5f};if(outline){for(auto delta:{ImVec2{-1,0},ImVec2{1,0},ImVec2{0,-1},ImVec2{0,1}})draw->AddText(face,size,{at.x+delta.x,at.y+delta.y},IM_COL32(0,0,0,170),text.c_str());}draw->AddText(face,size,at,color,text.c_str());ImGui::Dummy({width,rowHeight+3});return rowHeight;};
 auto foreground=ImGui::GetColorU32(ImGuiCol_Text),neutral=ImGui::GetColorU32(ImGuiCol_TextDisabled);
 row("Speed (wu/s)",base,foreground);
 char buffer[64];if(view.speedAvailable)std::snprintf(buffer,sizeof buffer,"%.2f",double(view.speed));else std::snprintf(buffer,sizeof buffer,"--");std::string value(buffer);
 auto digits=SessionSketch::speedFont();const float unit=digits->FontSize;
 float advance=0;for(char c:std::string("0123456789"))advance=std::max(advance,digits->CalcTextSizeA(unit,100000,0,std::string(1,c).c_str()).x);
 auto characterWidth=[&](char c){return c>='0'&&c<='9'?advance:digits->CalcTextSizeA(unit,100000,0,std::string(1,c).c_str()).x;};
 const float reference=advance*3.f+characterWidth('.');float valueWidth=0;for(char c:value)valueWidth+=characterWidth(c);
 const float opening=SessionSketch::readoutWidth()>0?SessionSketch::readoutWidth():width*1.5f;
 // Four-character reference (9.99) occupies 40% of the opening. Its allocated
 // field and row never change with the value, including long-number shrinkage.
 const float rowSize=std::min(opening*.40f*unit/reference,width*unit/reference);
 const float size=std::min(rowSize,width*unit/valueWidth);
 auto p=ImGui::GetCursorScreenPos();float left=p.x+(width-valueWidth*size/unit)*.5f;
 metrics.numberMin={p.x,p.y};metrics.numberMax={p.x+width,p.y+rowSize};metrics.numberWidth=width;metrics.numberHeight=rowSize;
 metrics.numberFontSize=size;metrics.numberInkWidth=valueWidth*size/unit;
 for(char c:value){char text[2]={c,0};draw->AddText(digits,size,{left,p.y+(rowSize-size)*.5f},foreground,text);left+=characterWidth(c)*size/unit;}
 ImGui::Dummy({width,rowSize+3});metrics.barHeight=ImGui::GetFrameHeight()*1.2f;ImGui::ProgressBar(view.speedAvailable?view.fraction:0,{width,metrics.barHeight},"");
 const char* bounce=view.bounceAvailable?(view.on?"Bounce State: ON":"Bounce State: OFF"):"Bounce State: Unavailable";
 const ImU32 color=view.bounceAvailable?(view.on?IM_COL32(39,219,75,255):IM_COL32(239,70,70,255)):neutral;
 p=ImGui::GetCursorScreenPos();metrics.bounceFont=row(bounce,base*1.6f,color,view.bounceAvailable);metrics.bounceCenter={p.x+width*.5f,p.y+metrics.bounceFont*.5f};
 if(view.bounceAvailable){char diagnostic[80];std::snprintf(diagnostic,sizeof diagnostic,"Pass threshold %.2f wu/s",double(view.threshold));row(diagnostic,base*.8f,neutral);}
 if(!view.bounceAvailable&&!view.reason.empty()){ImGui::PushTextWrapPos(0);ImGui::TextDisabled("%s",view.reason.c_str());ImGui::PopTextWrapPos();}
 if(!view.map.empty())row("Map: "+view.map,base*.75f,neutral);
 row("Halo 2 physics by Poorboy",base*.75f,neutral);return metrics;
}
}
