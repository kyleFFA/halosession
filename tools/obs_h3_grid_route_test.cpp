#include <windows.h>
#include <cassert>
#include <cstdio>
#include "../HCMInternal/SessionOBSBypass.h"
#include "../HCMInternal/WorldLinesCaptureRoute.h"
struct Harness {
 worldlines::CaptureRoute route;uint64_t seen=0,now=1000;int title=0;bool wire=false,noBloom=false,xray=false,ready=true;int early=0,late=0,capture=0;bool warming=false;
 Harness(){sessionobs::request(false);sessionobs::earlyGridSequence=0;sessionobs::gridPostUntil=0;sessionobs::frame=nullptr;}
 void view(bool available=true){early=late=capture=0;if(title==0&&available&&worldlines::CaptureRoute::engineAllowed(wire,noBloom,xray,(sessionobs::lateGridLease(now)&&ready))){route.engineDrew();++sessionobs::earlyGridSequence;++early;}}
 void present(bool supported,bool available=true,int uiToggle=-1){auto frame=sessionobs::beginFrame(this,seen);frame.post=supported&&sessionobs::requested.load();sessionobs::frame=&frame;
  auto render=[&](){if(uiToggle>=0)sessionobs::request(uiToggle!=0);const bool engine=route.consumeEarlyDraw();if(engine)frame.earlyGrid=true;if(available&&!engine)++late;};
  if(!frame.post){frame.drawn=true;render();capture=early+late;}else{capture=early;assert(frame.drawPost(this,true));render();}
  warming=frame.post&&frame.earlyGrid;sessionobs::finishFrame(frame,now);sessionobs::frame=nullptr;++now;if(available)assert(early+late==1);
 }
 void tick(bool supported,bool available=true,int uiToggle=-1){view(available);present(supported,available,uiToggle);}
};
int main(){
 {Harness h;h.tick(false);assert(h.early==1&&h.capture==1);h.tick(false,true,1);assert(h.early==1&&h.capture==1);h.tick(true);assert(h.warming&&h.early==1&&h.late==0&&h.capture==1);h.tick(true);assert(!h.warming&&h.early==0&&h.late==1&&h.capture==0);
  h.tick(true,true,0);assert(h.early==0&&h.late==1&&h.capture==0);h.tick(false);assert(h.early==1&&h.capture==1);}
 {Harness h;sessionobs::request(true);h.tick(true);h.tick(true);assert(h.late==1&&h.capture==0);h.tick(false);assert(h.early==0&&h.late==1&&h.capture==1);h.tick(false);assert(h.early==1&&h.late==0&&h.capture==1);h.tick(true);assert(h.warming);h.tick(true);assert(h.capture==0);h.now+=1000;h.tick(true);assert(h.warming&&h.capture==1);h.tick(true);assert(!h.warming&&h.capture==0);}
 {Harness h;h.ready=false;sessionobs::request(true);h.tick(true);h.tick(true);assert(h.early==1&&h.warming&&h.capture==1);h.ready=true;h.tick(true);assert(h.early==0&&h.late==1&&h.capture==0);}
 {Harness h;sessionobs::request(true);h.tick(false);assert(h.early==1);h.view();h.noBloom=true;h.present(false);assert(h.early==1&&h.late==0);h.tick(false);assert(h.early==0&&h.late==1);h.noBloom=false;h.xray=true;h.tick(false);assert(h.late==1);h.xray=false;h.wire=true;h.tick(true);assert(h.late==1&&h.wire&&!h.noBloom&&!h.xray);}
 {Harness h;sessionobs::request(true);h.tick(true);h.tick(true);for(int title:{1,2,0,2,1,0}){h.title=title;h.tick(true);assert(h.early==0&&h.late==1&&h.capture==0);h.tick(false);assert(h.early==0&&h.late==1&&h.capture==1);h.tick(false);assert(title==0?h.early==1:h.late==1);h.tick(true);h.tick(true);assert(h.capture==0);}}
 {Harness h;h.view();h.present(false,false);h.title=1;h.tick(false);assert(h.late==1);h.title=0;h.tick(false);assert(h.early==1);}
 {Harness h;sessionobs::request(true);h.tick(true);h.tick(true);for(int i=0;i<1000;++i){h.title=(i/17)%3;h.wire=i%23==0;h.noBloom=i%29==0;h.xray=i%31==0;sessionobs::request(i%11!=0);h.tick(i%13!=0);assert(h.early+h.late==1);if(h.title!=0)assert(h.early==0);if(sessionobs::lateGridLease(h.now)&&h.title!=0)assert(h.capture==0);}}
 std::puts("PASS: production lease/frame and grid draw accounting; startup/enable warmup; off and unavailable fallback without duplicate/lost local draws; fresh/stale engine history; H3/ODST/Reach transitions; unchanged wire/no-bloom/X-ray values; loading consumption; 1000 mixed routing frames. Synthetic, no game/OBS accessed.");
}
