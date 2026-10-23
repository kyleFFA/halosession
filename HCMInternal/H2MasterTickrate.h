#pragma once
#include "pch.h"
#include "IOptionalCheat.h"
#include "GameState.h"
#include "DIContainer.h"

// Selective H2 port of Harc's MasterTickrate: timer + collision scalar + game
// thread body rebuild. No Season7Physics, custom rates, TLS or other-game path.
class H2MasterTickrate : public IOptionalCheat
{
    class Impl;
    std::unique_ptr<Impl> pimpl;
public:
    H2MasterTickrate(GameState game, IDIContainer& dicon);
    ~H2MasterTickrate();
    static std::string status();
    static bool beginShutdown();
    static bool shutdownReady();
    std::string_view getName() override { return nameof(H2MasterTickrate); }
};
