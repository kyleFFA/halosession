#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

// No game calls, input ownership or pause tokens. Shared by the adapter and offline tests.
namespace quickmenu {
using Identity = std::array<uint64_t,12>;
struct Pick { int slot; Identity identity; uint64_t device, time; };
class Selection {
    bool armed=false;
    uint64_t owner=0,last=0;
    Identity identity{};
    std::optional<Pick> pending;
public:
    int slot=-1;
    bool active() const { return owner!=0; }
    void cancel() { owner=0;slot=-1;armed=false;pending.reset(); }
    void expire(uint64_t now) { if(active() && now-last>200)cancel(); }
    bool sample(uint64_t device,bool connected,bool down,float x,float y,
                bool eligible,const Identity& current,uint64_t now,float deadzone,float sensitivity) {
        if(active() && (now-last>200 || current!=identity || !eligible))cancel();
        if(!connected || !eligible) { if(!owner || owner==device)cancel();return false; }
        if(owner && owner!=device)return false;
        if(!down) {
            if(owner) { if(slot>=0)pending=Pick{slot,identity,device,now};owner=0;slot=-1; }
            armed=true;return false;
        }
        if(!owner) {
            if(!armed)return false;
            owner=device;identity=current;slot=-1;armed=false;pending.reset();
        }
        last=now;
        const float radius=std::hypot(x,y)*sensitivity;
        if(!std::isfinite(radius) || radius<deadzone) {slot=-1;return true;}
        if(slot<0 && radius<deadzone+.04f)return true;
        constexpr float pi=3.14159265359f,step=2*pi/5;
        float angle=std::atan2(x,y);if(angle<0)angle+=2*pi;
        const int candidate=int(std::floor((angle+step/2)/step))%5;
        if(slot<0)slot=candidate;
        else {
            float delta=std::remainder(angle-slot*step,2*pi);
            if(std::fabs(delta)>step/2+.07f)slot=candidate;
        }
        return true;
    }
    std::optional<Pick> take(const Identity& current,uint64_t now,bool eligible) {
        auto p=pending;pending.reset();
        if(!eligible || !p || p->identity!=current || now-p->time>100)return {};
        return p;
    }
};
// XINPUT_STATE, offsets independently checked against the Windows SDK.
struct XboxState {uint32_t packet;uint16_t buttons;uint8_t lt,rt;int16_t lx,ly,rx,ry;};
static_assert(sizeof(XboxState)==16);
inline void suppress(XboxState& state) {state.rx=0;state.ry=0;}
inline bool xboxDown(const XboxState& state,int binding) {
    constexpr uint16_t masks[]={0,0x0010,0x0020,0x0100,0x0200,0x0040,0x0080,0x1000,0x2000,0x4000,0x8000,1,2,4,8};
    if(binding<=0 || binding>16)return false;
    return binding==15?state.lt>127:binding==16?state.rt>127:(state.buttons&masks[binding])!=0;
}
inline bool nativeDown(const std::array<uint8_t,32>& buttons,uint32_t pov,int binding) {
    if(binding>0 && binding<=32)return (buttons[binding-1]&0x80)!=0;
    if(binding<33 || binding>36 || pov>=36000)return false;
    int difference=std::abs(int(pov)-(binding-33)*9000);difference=std::min(difference,36000-difference);
    return difference<=4500; // Diagonals retain both adjacent direction buttons.
}
// DIJOYSTATE2's position axes; never overwrite the rest of its 272-byte record.
struct Axis {int32_t minimum=0,maximum=0;bool valid()const{return int64_t(maximum)-minimum>=2;} int32_t center()const{return int32_t((int64_t(minimum)+maximum)/2);} float normalize(int32_t v)const{return valid()?float(int64_t(v)-center())/(float(int64_t(maximum)-minimum)*.5f):0.f;}};
inline bool suppressDirect(int32_t* axes,int x,int y,const Axis& horizontal,const Axis& vertical) {
    if(x<2 || x>5 || y<2 || y>5 || x==y || !horizontal.valid() || !vertical.valid())return false;
    axes[x]=horizontal.center();axes[y]=vertical.center();return true;
}
}
