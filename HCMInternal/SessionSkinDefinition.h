#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <optional>
#include <cmath>
#include <cwctype>
#include <pugixml.hpp>
namespace SessionSkin {
struct Definition {
 std::string id="default",name="Session V1";
 std::filesystem::path folder,ring,clouds,node,plaque;
 float cloudRimOpacity=100.f/255.f;
 float centerX=.500179154f,centerY=.475353879f,radius=.311131824f,insetRatio=1.f-.289f/.311131824f;
};
inline bool contained(const std::filesystem::path& root,const std::filesystem::path& candidate){
 auto a=root.begin(),b=candidate.begin();
 for(;a!=root.end();++a,++b){
  if(b==candidate.end())return false;auto x=a->wstring(),y=b->wstring();
  for(auto& c:x)c=wchar_t(std::towlower(c));for(auto& c:y)c=wchar_t(std::towlower(c));if(x!=y)return false;
 }
 return true;
}
inline std::optional<Definition> read(const std::filesystem::path& folder){
 try{
  Definition d;d.id=folder.filename().string();d.folder=std::filesystem::weakly_canonical(folder);
  if(d.id.empty()||d.id=="default"||d.id.size()>64||d.id.front()=='_')return {};
  for(auto c:d.id)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return {};
  auto manifest=std::filesystem::weakly_canonical(d.folder/"skin.xml");if(!contained(d.folder,manifest))return {};auto bytes=std::filesystem::file_size(manifest);if(!bytes||bytes>65536)return {};
  std::ifstream file(manifest,std::ios::binary);std::string xml(size_t(bytes),'\0');if(!file.read(xml.data(),std::streamsize(bytes)))return {};
  pugi::xml_document document;if(!document.load_buffer(xml.data(),xml.size()))return {};
  auto root=document.child("SessionSkin");if(!root||root.attribute("version").as_int()!=1)return {};
  d.name=root.attribute("name").as_string();if(d.name.empty()||d.name.size()>120)return {};
  auto opening=root.child("Opening");for(auto name:{"centerX","centerY","radius","wheelInsetRatio"})if(!opening.attribute(name))return {};
  d.centerX=opening.attribute("centerX").as_float(NAN);d.centerY=opening.attribute("centerY").as_float(NAN);
  d.radius=opening.attribute("radius").as_float(NAN);d.insetRatio=opening.attribute("wheelInsetRatio").as_float(NAN);
  if(!std::isfinite(d.centerX)||!std::isfinite(d.centerY)||!std::isfinite(d.radius)||!std::isfinite(d.insetRatio))return {};
  if(d.radius<.15f||d.radius>.44f||d.insetRatio<.03f||d.insetRatio>.25f)return {};
  if(d.centerX-d.radius<.02f||d.centerY-d.radius<.02f||d.centerX+d.radius>.98f||d.centerY+d.radius>.98f)return {};
  auto cloud=root.child("Clouds");if(cloud.attribute("rimOpacity")){d.cloudRimOpacity=cloud.attribute("rimOpacity").as_float(NAN);if(!std::isfinite(d.cloudRimOpacity)||d.cloudRimOpacity<0||d.cloudRimOpacity>1)return {};}
  auto textures=root.child("Textures");bool valid=true;
  auto asset=[&](const char* key,bool required){
   std::filesystem::path relative=textures.attribute(key).as_string();if(relative.empty()){if(required)valid=false;return std::filesystem::path{};}
   if(relative.is_absolute()||relative.has_root_path()){valid=false;return std::filesystem::path{};}
   for(auto part:relative)if(part==".."){valid=false;return std::filesystem::path{};}
   auto path=std::filesystem::weakly_canonical(d.folder/relative);auto extension=path.extension().wstring();for(auto& c:extension)c=wchar_t(std::towlower(c));
   if(extension!=L".png"||!contained(d.folder,path))valid=false;return path;
  };
  d.ring=asset("ring",true);d.clouds=asset("clouds",true);d.node=asset("node",false);d.plaque=asset("plaque",false);
  if(!valid)return {};return d;
 }catch(...){return {};}
}
}
