#pragma once
#include "pch.h"
#include "IOptionalCheat.h"
#include "GameState.h"
#include "DIContainer.h"
class H2NativeMovement : public IOptionalCheat {
 class Impl;
 std::unique_ptr<Impl> pimpl;
public:
 H2NativeMovement(GameState game,IDIContainer& dicon);
 ~H2NativeMovement();
 std::string_view getName() override {return nameof(H2NativeMovement);}
};
