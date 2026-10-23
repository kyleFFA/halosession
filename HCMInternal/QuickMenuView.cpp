#include "pch.h"
#include "QuickMenuView.h"
namespace quickmenu {
void renderWheel(const SessionSketch::State& s,const WheelView& v) {
 SessionSketch::backdrop(s);
 const auto g=geometry(s);auto d=ImGui::GetForegroundDrawList();auto font=SessionSketch::font();
 const auto c=g.center,r=g.radius;constexpr float pi=3.14159265359f,step=2*pi/5;
 d->PushClipRect(s.position,{s.position.x+s.size.x,s.position.y+s.size.y},true);
 d->AddEllipseFilled(c,r,IM_COL32(9,24,34,235),0,96);
 d->AddEllipse(c,r,IM_COL32(146,230,246,255),0,96,2.f*s.size.x/900.f);
 auto point=[&](float angle,float f){return ImVec2{c.x+std::sin(angle)*r.x*f,c.y-std::cos(angle)*r.y*f};};
 auto text=[&](const std::string& text,ImVec2 at,float wrap,ImU32 color,float size){
  auto extent=font->CalcTextSizeA(size,10000,wrap,text.c_str());
  d->AddText(font,size,{at.x-extent.x/2,at.y-extent.y/2},color,text.c_str(),nullptr,wrap);
 };
 for(int i=0;i<5;++i) {
  float angle=i*step;
  if(v.selected==i) {
   d->PathLineTo(c);d->PathEllipticalArcTo(c,r,0,angle-pi/2-step/2+.02f,angle-pi/2+step/2-.02f,32);
   d->PathFillConvex(v.enabled[i]?IM_COL32(43,129,153,235):IM_COL32(95,61,61,235));
  }
  d->AddLine(point(angle-step/2,.25f),point(angle-step/2,.98f),IM_COL32(96,157,175,255),s.size.x/900.f);
  text(v.labels[i],point(angle,.64f),r.x*.70f,v.enabled[i]?IM_COL32(234,249,253,255):IM_COL32(168,168,168,255),g.font);
 }
 d->AddEllipseFilled(c,{r.x*.34f,r.y*.22f},IM_COL32(12,31,43,255),0,48);
 text(v.selected>=0?v.labels[v.selected]:"Center cancels",c,r.x*.57f,IM_COL32(246,253,255,255),g.font*1.15f);
 text("Release to use",{c.x,c.y+r.y*.89f},r.x*1.3f,IM_COL32(208,232,239,255),g.font*.85f);
 d->PopClipRect();
}
}
