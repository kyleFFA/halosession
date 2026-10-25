#pragma once
#include "pch.h"
#include "IOptionalCheat.h"
#include "GameState.h"
#include "DIContainer.h"
#include "PointerDataStore.h"
#include "MultilevelPointer.h"

class OBSBypassCheck : public IOptionalCheat
{
private:



public:
	OBSBypassCheck(GameState gameImpl, IDIContainer& dicon)
	{
		// Compatibility is checked dynamically on the graphics owner thread.
		(void)gameImpl;(void)dicon;
	}
	

	std::string_view getName() override { return nameof(OBSBypassCheck); }

};