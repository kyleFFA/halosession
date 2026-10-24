#pragma once
#include "SessionSketch.h"
#include <pugixml.hpp>
#include <fstream>
namespace SessionPreset {
inline bool safeId(const std::string& id){if(id.empty()||id.size()>64||id=="."||id=="..")return false;for(unsigned char c:id)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return false;return true;}
inline bool finite(float value,float low,float high){return std::isfinite(value)&&value>=low&&value<=high;}
inline void color(pugi::xml_node parent,const char* name,ImVec4 value){auto n=parent.append_child(name);n.append_attribute("r")=value.x;n.append_attribute("g")=value.y;n.append_attribute("b")=value.z;n.append_attribute("a")=value.w;}
inline bool color(pugi::xml_node parent,const char* name,ImVec4& value,bool){auto n=parent.child(name);if(!n)return false;float values[4];int i=0;for(auto key:{"r","g","b","a"}){if(!n.attribute(key))return false;values[i]=n.attribute(key).as_float(NAN);if(!finite(values[i++],0,1))return false;}value={values[0],values[1],values[2],values[3]};return true;}
inline bool valid(const SessionSketch::State& s){return finite(s.centerOpacity,0.f,1.f)&&finite(s.textScale,.75f,1.6f)&&finite(s.centerSpeed,.25f,2.f)&&finite(s.backgroundOpacity,.5f,1.f)&&finite(s.size.y,300,4000)&&s.centerBackground>=0&&s.centerBackground<=4&&finite(s.position.x,0,32000)&&finite(s.position.y,0,32000);}
inline void write(pugi::xml_document& doc,const SessionSketch::State& s,const std::string& name,bool externalSkin,bool centerAsset){
 auto root=doc.append_child("SessionPreset");root.append_attribute("version")=1;root.append_attribute("name")=name.c_str();
 root.append_attribute("skin")=externalSkin?"artwork":"embedded-v1";if(centerAsset)root.append_attribute("centerAsset")="center.png";
 auto a=root.append_child("Appearance");a.append_attribute("background")=s.centerBackground;a.append_attribute("animated")=s.centerAnimated;a.append_attribute("speed")=s.centerSpeed;a.append_attribute("cloudDrift")=s.animateClouds;a.append_attribute("opacity")=s.backgroundOpacity;a.append_attribute("centerOpacity")=s.centerOpacity;a.append_attribute("textScale")=s.textScale;a.append_attribute("centered")=s.centered;a.append_attribute("autoTextContrast")=s.autoTextContrast;
 a.append_attribute("x")=s.position.x;a.append_attribute("y")=s.position.y;a.append_attribute("diameter")=s.size.y;
 if(ImGui::GetCurrentContext()){auto viewport=ImGui::GetIO().DisplaySize;if(viewport.x>0&&viewport.y>0){a.append_attribute("referenceWidth")=viewport.x;a.append_attribute("referenceHeight")=viewport.y;}}
 color(a,"Wheel",s.wheel.background);color(a,"Segment",s.wheel.segment);color(a,"Selected",s.wheel.selected);color(a,"Text",s.textColor);
}
inline bool read(const std::filesystem::path& file,SessionSketch::State& state,std::string& name,bool& externalSkin,bool& centerAsset){
 try{if(std::filesystem::file_size(file)>65536)return false;pugi::xml_document doc;if(!doc.load_file(file.c_str()))return false;auto root=doc.child("SessionPreset");if(root.attribute("version").as_int()!=1)return false;name=root.attribute("name").as_string();if(name.empty()||name.size()>100)return false;
 std::string skin=root.attribute("skin").as_string();if(skin!="artwork"&&skin!="embedded-v1")return false;externalSkin=skin=="artwork";
 std::string asset=root.attribute("centerAsset").as_string();if(!asset.empty()&&asset!="center.png")return false;centerAsset=!asset.empty();
 auto a=root.child("Appearance");for(auto required:{"background","animated","speed","cloudDrift","opacity","centerOpacity","textScale","centered","autoTextContrast","x","y","diameter"})if(!a.attribute(required))return false;
 state.centerBackground=a.attribute("background").as_int(-1);state.centerAnimated=a.attribute("animated").as_bool();state.centerSpeed=a.attribute("speed").as_float(NAN);state.animateClouds=a.attribute("cloudDrift").as_bool();state.backgroundOpacity=a.attribute("opacity").as_float(NAN);state.centerOpacity=a.attribute("centerOpacity").as_float(NAN);state.textScale=a.attribute("textScale").as_float(NAN);state.centered=a.attribute("centered").as_bool();state.autoTextContrast=a.attribute("autoTextContrast").as_bool();state.position={a.attribute("x").as_float(NAN),a.attribute("y").as_float(NAN)};float d=a.attribute("diameter").as_float(NAN);state.size={d*1.5f,d};
 if(!valid(state)||!color(a,"Wheel",state.wheel.background,true)||!color(a,"Segment",state.wheel.segment,true)||!color(a,"Selected",state.wheel.selected,true)||!color(a,"Text",state.textColor,true))return false;
 if((state.centerBackground==1||state.centerBackground==2)!=centerAsset)return false;
 if(a.attribute("referenceWidth")||a.attribute("referenceHeight")){
  float width=a.attribute("referenceWidth").as_float(NAN),height=a.attribute("referenceHeight").as_float(NAN);if(!finite(width,100,32000)||!finite(height,100,32000))return false;
  if(ImGui::GetCurrentContext()){auto viewport=ImGui::GetIO().DisplaySize;if(viewport.x>24&&viewport.y>24){float cx=(state.position.x+state.size.x*.5f)/width,cy=(state.position.y+state.size.y*.5f)/height;float diameter=std::min({state.size.y,(viewport.x-16)/1.5f,viewport.y-16});state.size={diameter*1.5f,diameter};state.position={std::clamp(cx*viewport.x-state.size.x*.5f,0.f,std::max(0.f,viewport.x-state.size.x)),std::clamp(cy*viewport.y-state.size.y*.5f,0.f,std::max(0.f,viewport.y-state.size.y))};}}
 }
 state.initialized=true;return true;}catch(...){return false;}
}
}
