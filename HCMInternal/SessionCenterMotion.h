#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>
namespace SessionCenter {
inline constexpr double tau=6.283185307179586;
// Called only while the menu or wheel actually draws. Hidden frames do no animation work.
struct Clock {
 double elapsed=0,last=0;int frame=-2;
 double visible(double now,int current,bool animate,float speed){
  if(current!=frame){if(frame==current-1&&animate)elapsed+=std::clamp(now-last,0.,2.)*speed;last=now;frame=current;}
  return animate?elapsed:0.;
 }
};
inline int approvedFrame(double seconds,double period){double group=std::fmod(seconds,period)/(period/32.);int whole=int(group);double fraction=group-whole;return whole*3+(fraction>=.64?2:fraction>=.32?1:0);}
inline ImVec2 cloudCrop(double seconds){double t=tau*approvedFrame(seconds,32.)/96.;return {float(int(50+22*std::sin(t)))/420.f,float(int(50+12*std::cos(t)))/420.f};}
inline double starPhase(double seconds){return tau*approvedFrame(seconds,8.)/96.;}
inline bool clipLine(ImVec2& a,ImVec2& b,ImVec2 c,float radius){
 float x=a.x-c.x,y=a.y-c.y,dx=b.x-a.x,dy=b.y-a.y;
 float aa=dx*dx+dy*dy;if(aa<1e-10f)return x*x+y*y<=radius*radius;
 float bb=2*(x*dx+y*dy),cc=x*x+y*y-radius*radius,disc=bb*bb-4*aa*cc;if(disc<0)return false;
 float low=std::max(0.f,(-bb-std::sqrt(disc))/(2*aa)),high=std::min(1.f,(-bb+std::sqrt(disc))/(2*aa));if(low>high)return false;
 auto origin=a;a={origin.x+dx*low,origin.y+dy*low};b={origin.x+dx*high,origin.y+dy*high};return true;
}
}
