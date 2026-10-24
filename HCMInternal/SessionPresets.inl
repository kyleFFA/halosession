// Included in SessionSketch namespace after the renderer's private helpers.
void setAppearanceDirectory(const std::filesystem::path& directory){appearanceDirectory=std::filesystem::weakly_canonical(directory);loadedCenterPath.clear();centerTexture.Reset();}
std::string presetMessage(){return presetStatus;}
std::vector<PresetChoice> savedPresets(){
 std::vector<PresetChoice> choices;try{auto root=appearanceDirectory/"SavedSkins";if(!std::filesystem::is_directory(root))return choices;
 for(const auto& entry:std::filesystem::directory_iterator(root)){if(choices.size()>=128)break;if(!entry.is_directory()||entry.is_symlink())continue;auto id=entry.path().filename().string();if(!SessionPreset::safeId(id))continue;State candidate;std::string name;bool external=false,asset=false;
 if(SessionPreset::read(entry.path()/"preset.xml",candidate,name,external,asset))choices.push_back({id,name});}
 std::sort(choices.begin(),choices.end(),[](auto& a,auto& b){return a.name<b.name;});}catch(...){}return choices;
}
bool savePreset(const State& s,const std::string& name,bool overwrite){
 namespace fs=std::filesystem;
 try{
  if(!SessionPreset::safeId(name)||!SessionPreset::valid(s)){presetStatus="Use 1-64 letters, digits, dashes or underscores for the preset name.";return false;}
  auto root=appearanceDirectory/"SavedSkins";fs::create_directories(root);auto dest=root/name;
  if(fs::exists(dest)&&!overwrite){presetStatus="That preset already exists. Choose Replace explicitly or use a new name.";return false;}
  if(fs::exists(dest)&&(!fs::is_directory(dest)||fs::is_symlink(dest))){presetStatus="Unsafe preset destination.";return false;}
  auto temp=root/(name+"-writing");if(fs::exists(temp)){presetStatus="A previous staging folder exists; choose another name.";return false;}fs::create_directory(temp);
  const bool external=selectedSkin.id!="default",asset=s.centerBackground==1||s.centerBackground==2;
  if(external){auto art=temp/"artwork";fs::create_directory(art);pugi::xml_document skin;auto r=skin.append_child("SessionSkin");r.append_attribute("version")=1;r.append_attribute("name")=selectedSkin.name.c_str();auto textures=r.append_child("Textures");
   for(auto item:{std::pair{"ring",selectedSkin.ring},std::pair{"clouds",selectedSkin.clouds},std::pair{"node",selectedSkin.node},std::pair{"plaque",selectedSkin.plaque}}){if(item.second.empty())continue;std::string file=std::string(item.first)+".png";fs::copy_file(item.second,art/file);textures.append_attribute(item.first)=file.c_str();}
   auto opening=r.append_child("Opening");opening.append_attribute("centerX")=selectedSkin.centerX;opening.append_attribute("centerY")=selectedSkin.centerY;opening.append_attribute("radius")=selectedSkin.radius;opening.append_attribute("wheelInsetRatio")=selectedSkin.insetRatio;r.append_child("Clouds").append_attribute("rimOpacity")=selectedSkin.cloudRimOpacity;if(!skin.save_file((art/"skin.xml").c_str()))throw std::runtime_error("Cannot save skin assets");
  }
  if(asset){ComPtr<ID3D11ShaderResourceView> test;uint64_t budget=0;if(!owner||!fileTexture(centerPath(s),test,nullptr,budget))throw std::runtime_error("Center artwork unavailable");fs::copy_file(centerPath(s),temp/"center.png");}
  pugi::xml_document doc;SessionPreset::write(doc,s,name,external,asset);if(!doc.save_file((temp/"preset.xml").c_str()))throw std::runtime_error("Cannot save preset");
  // No silent deletion/overwrite: an explicit replacement retains the old folder as a backup.
  if(fs::exists(dest)){auto backup=root/(name+"-backup-"+std::to_string(GetTickCount64()));fs::rename(dest,backup);}fs::rename(temp,dest);presetStatus="Saved skin: "+name;return true;
 }catch(const std::exception& e){presetStatus=std::string("Save failed: ")+e.what();return false;}
}
bool loadPreset(State& state,const std::string& id){
 namespace fs=std::filesystem;
 try{
  if(!SessionPreset::safeId(id))throw std::runtime_error("Invalid preset ID");auto root=fs::weakly_canonical(appearanceDirectory/"SavedSkins"),folder=fs::weakly_canonical(root/id);
  if(!SessionSkin::contained(root,folder)||fs::is_symlink(root/id))throw std::runtime_error("Preset path escapes SavedSkins");
  State candidate=state;std::string name;bool external=false,asset=false;if(!SessionPreset::read(folder/"preset.xml",candidate,name,external,asset))throw std::runtime_error("Invalid preset data");
  SessionSkin::Definition definition;ComPtr<ID3D11ShaderResourceView> nextRing,nextClouds,nextNode,nextPlaque,nextCenter;AlphaMask nextRingMask,nextCloudMask;uint64_t budget=0;
  if(external){auto parsed=SessionSkin::read(folder/"artwork");if(!parsed||!SessionSkin::contained(folder,parsed->folder))throw std::runtime_error("Invalid preset artwork");definition=*parsed;
   if(!fileTexture(definition.ring,nextRing,&nextRingMask,budget)||!fileTexture(definition.clouds,nextClouds,&nextCloudMask,budget)||(!definition.node.empty()&&!fileTexture(definition.node,nextNode,nullptr,budget))||(!definition.plaque.empty()&&!fileTexture(definition.plaque,nextPlaque,nullptr,budget)))throw std::runtime_error("Preset PNG cannot be decoded");
   if(nextRingMask.width!=nextRingMask.height)throw std::runtime_error("Preset ring must be square");
   for(auto mask:{&nextRingMask,&nextCloudMask})if(mask->hit(0,0)||mask->hit(.999f,0)||mask->hit(0,.999f)||mask->hit(.999f,.999f))throw std::runtime_error("Opaque artwork corners");
   for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)if(nextRingMask.hit(definition.centerX+x*definition.radius*.2f,definition.centerY+y*definition.radius*.2f))throw std::runtime_error("Covered ring opening");
  }
  if(asset){auto file=fs::weakly_canonical(folder/"center.png");if(!SessionSkin::contained(folder,file)||!fileTexture(file,nextCenter,nullptr,budget))throw std::runtime_error("Invalid center artwork");candidate.centerAsset=file.string();}else candidate.centerAsset.clear();
  if(external){ring=std::move(nextRing);clouds=std::move(nextClouds);nodeTexture=std::move(nextNode);plaqueTexture=std::move(nextPlaque);ringMask=std::move(nextRingMask);cloudMask=std::move(nextCloudMask);definition.id="preset-"+id;selectedSkin=definition;presetSkin=definition;}else {presetSkin.reset();defaultSkin();}
  pendingSkin.reset();centerTexture=std::move(nextCenter);loadedCenterPath=asset?std::filesystem::path(candidate.centerAsset):fs::path{};centerFailed=false;
  candidate.clock={};candidate.contentHeights.fill(0);candidate.contentWidths.fill(0);++candidate.contentRevision;state=std::move(candidate);presetStatus="Loaded skin: "+name;return true;
 }catch(const std::exception& e){presetStatus=std::string("Load refused; current appearance retained: ")+e.what();return false;}
}
