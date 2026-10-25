#include "imgui.h"
#include "imgui_internal.h"
#include "../HCMInternal/SessionOBSControls.h"
#include <cassert>
#include <iostream>
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.DisplaySize={1280,720};io.DeltaTime=1.f/60;io.IniFilename=nullptr;unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);io.Fonts->SetTexID((ImTextureID)1);bool wanted=false;ImVec2 button{};int changes=0;
 auto frame=[&](){ImGui::NewFrame();ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({500,270});ImGui::Begin("Control fixture",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);button=ImGui::GetCursorScreenPos();button.x+=10;button.y+=10;if(sessionobs::controls(wanted))++changes;assert(ImGui::GetCursorPosY()<270);ImGui::End();ImGui::Render();assert(ImGui::GetDrawData()->TotalVtxCount>0);};
 frame();frame();assert(!wanted);io.AddMousePosEvent(button.x,button.y);io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();assert(wanted&&changes==1);
 for(auto s:{sessionobs::Status::Waiting,sessionobs::Status::Unsupported,sessionobs::Status::CaptureOverlays,sessionobs::Status::Ready,sessionobs::Status::Active,sessionobs::Status::Restoring,sessionobs::Status::GridTransition}){sessionobs::status=s;frame();assert(wanted);}
 io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();assert(!wanted&&changes==2);ImGui::DestroyContext();std::cout<<"PASS: production Control checkbox, pointer on/off events, default OFF, independent request/status, all honest state captions and wrapped bounds.\n";
}
