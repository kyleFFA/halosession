#include "pch.h"
#include "GUIRequiredServices.h"
#include <iostream>
int main()
{
    GUIRequiredServices registry;
    for(const auto& [game,service]:registry.getAllRequiredServices())
        std::cout<<"SERVICE "<<game.toString()<<' '<<magic_enum::enum_name(service)<<'\n';
    for(const auto& [gui,game]:registry.getToplevelGUIElements())
        std::cout<<"TOP "<<game.toString()<<' '<<magic_enum::enum_name(gui)<<'\n';
}
