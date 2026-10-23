#include "pch.h"
#include "QuickMenuView.h"
namespace quickmenu {
void renderWheel(const SessionSketch::State& s,const WheelView& v) {
 SessionSketch::backdrop(s);
 const auto g=geometry(s);auto d=ImGui::GetForegroundDrawList();auto font=SessionSketch::font();
 const auto c=g.center;const float radius=g.radius.x,scale=SessionSketch::layout(s).diameter/900.f;
 constexpr float pi=3.14159265359f,step=2*pi/5;
 d->PushClipRect(s.position,{s.position.x+s.size.x,s.position.y+s.size.y},true);
 d->AddCircleFilled(c,radius,ImGui::ColorConvertFloat4ToU32(s.wheel.background),96);
 auto point=[&](float angle,float f){return ImVec2{c.x+std::sin(angle)*radius*f,c.y-std::cos(angle)*radius*f};};
 auto text=[&](const std::string& text,ImVec2 at,float wrap,ImU32 color,float size,bool backing){
  auto extent=font->CalcTextSizeA(size,10000,wrap,text.c_str());ImVec2 a{at.x-extent.x/2,at.y-extent.y/2};
  if(backing)d->AddRectFilled({a.x-5*scale,a.y-3*scale},{a.x+extent.x+5*scale,a.y+extent.y+3*scale},IM_COL32(9,24,34,205),5*scale);
  d->AddText(font,size,a,color,text.c_str(),nullptr,wrap);
 };
 for(int i=0;i<5;++i) {
  const float angle=i*step;
  auto sector=[&]{d->PathLineTo(c);d->PathArcTo(c,radius,angle-pi/2-step/2+.02f,angle-pi/2+step/2-.02f,32);};
  sector();d->PathFillConvex(ImGui::ColorConvertFloat4ToU32(v.selected==i?s.wheel.selected:s.wheel.segment));
  if(v.selected==i){sector();d->PathStroke(IM_COL32(9,24,34,240),ImDrawFlags_Closed,4.f*scale);sector();d->PathStroke(IM_COL32(230,253,255,255),ImDrawFlags_Closed,2.f*scale);}
  d->AddLine(point(angle-step/2,.28f),point(angle-step/2,.98f),IM_COL32(96,157,175,230),scale);
  text(v.labels[i],point(angle,.64f),radius*.70f,v.enabled[i]?IM_COL32(242,252,255,255):IM_COL32(183,194,199,255),g.font,true);
 }
 d->AddCircle(c,radius,IM_COL32(146,230,246,255),96,2.f*scale);
 const float badgeRadius=radius*.28f;
 d->AddCircleFilled(c,badgeRadius,IM_COL32(12,31,43,235),64);
 d->AddCircle(c,badgeRadius,IM_COL32(129,202,218,240),64,1.f*scale);
 text(v.selected>=0?v.labels[v.selected]:"Center cancels",c,badgeRadius*1.7f,IM_COL32(246,253,255,255),g.font*.85f,false);
 text("Release to use",{c.x,c.y+radius*.89f},radius*1.3f,IM_COL32(225,242,247,255),g.font*.85f,true);
 d->PopClipRect();
}
}
