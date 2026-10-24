// Included inside SessionSketch's private renderer namespace.
ComPtr<ID3D11ShaderResourceView> centerTexture;
std::filesystem::path loadedCenterPath;
bool centerFailed=false;
std::string presetStatus;
std::filesystem::path centerPath(const State& s){return s.centerAsset.empty()?appearanceDirectory/"backgrounds"/(s.centerBackground==1?"bright-sky.png":"pastel-clouds.png"):std::filesystem::path(s.centerAsset);}
void centerBackdrop(ImDrawList* draw,const State& s,const Layout& l){
 if(!s.centerBackground)return;
 const float radius=l.diameter*selectedSkin.radius;
 const double seconds=s.clock.visible(ImGui::GetTime(),ImGui::GetFrameCount(),s.centerAnimated,s.centerSpeed);
 const ImU32 tint=IM_COL32(255,255,255,int(s.centerOpacity*255+.5f));
 if(s.centerBackground<=2){
  auto path=centerPath(s);if(path!=loadedCenterPath){loadedCenterPath=path;centerTexture.Reset();uint64_t budget=0;centerFailed=!fileTexture(path,centerTexture,nullptr,budget);}
  if(!centerTexture){draw->AddCircleFilled(l.center,radius,IM_COL32(208,225,235,255),96);return;}
  auto crop=SessionCenter::cloudCrop(seconds);constexpr float extent=320.f/420.f;
  draw->PushTextureID((ImTextureID)centerTexture.Get());constexpr int segments=128;draw->PrimReserve(segments*3,segments+1);auto base=draw->_VtxCurrentIdx;
  draw->PrimWriteVtx(l.center,{crop.x+extent*.5f,crop.y+extent*.5f},tint);
  for(int i=0;i<segments;++i){float angle=float(i*SessionCenter::tau/segments),x=std::cos(angle),y=std::sin(angle);draw->PrimWriteVtx({l.center.x+radius*x,l.center.y+radius*y},{crop.x+extent*(x*.5f+.5f),crop.y+extent*(y*.5f+.5f)},tint);}
  for(int i=0;i<segments;++i){draw->PrimWriteIdx(ImDrawIdx(base));draw->PrimWriteIdx(ImDrawIdx(base+1+i));draw->PrimWriteIdx(ImDrawIdx(base+1+(i+1)%segments));}draw->PopTextureID();return;
 }
 draw->AddCircleFilled(l.center,radius,IM_COL32(0,1,7,int(s.centerOpacity*255+.5f)),128);
 const double t=SessionCenter::starPhase(seconds);const bool retro=s.centerBackground==4;const float pixel=radius/160.f;
 auto field=[&](const auto& stars){for(const auto& star:stars){
  double p=std::pow((std::sin(t*star.frequency+star.phase)+1)*.5,retro?3:5);float strength=float(retro?.42+.58*p:.2+.8*p);
  ImU32 color=IM_COL32(int(star.r*strength),int(star.g*strength),int(star.b*strength),int(s.centerOpacity*255+.5f));ImVec2 c{l.center.x+(star.x-160)*pixel,l.center.y+(star.y-160)*pixel};
  float dx=c.x-l.center.x,dy=c.y-l.center.y;if(std::hypot(dx,dy)+pixel*.75f<radius)draw->AddRectFilled({c.x-pixel*.5f,c.y-pixel*.5f},{c.x+pixel*.5f,c.y+pixel*.5f},color);
  auto line=[&](float x0,float y0,float x1,float y1,ImU32 col){ImVec2 a{c.x+x0*pixel,c.y+y0*pixel},b{c.x+x1*pixel,c.y+y1*pixel};if(SessionCenter::clipLine(a,b,l.center,std::max(0.f,radius-2*pixel)))draw->AddLine(a,b,col,std::max(.5f,pixel));};
  if(star.large){float extension=float(1+int((retro?7:5)*p));line(-extension,0,extension,0,color);line(0,-extension,0,extension,color);if(retro&&p>.7){line(-2,-2,2,2,color);line(-2,2,2,-2,color);if(std::hypot(dx,dy)+pixel<radius)draw->AddRectFilled({c.x-pixel*.5f,c.y-pixel*.5f},{c.x+pixel*.5f,c.y+pixel*.5f},IM_COL32(255,255,255,int(s.centerOpacity*255+.5f)));}}
 }};
 if(retro)field(SessionCenter::retro);else field(SessionCenter::quiet);
}
