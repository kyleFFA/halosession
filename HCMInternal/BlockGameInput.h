#pragma once
#include "SharedRequestProvider.h"
#include "PointerDataStore.h"

class BlockGameInput 
{
private:
	std::shared_ptr<TokenSharedRequestProvider> pimpl;
public:

	BlockGameInput(std::shared_ptr<PointerDataStore> ptr);
	~BlockGameInput();


	std::shared_ptr<SharedRequestToken> makeScopedRequest() {
		return pimpl->makeScopedRequest();
	}

	std::shared_ptr<SharedRequestToken> makeMenuMovementRequest(); // Block keyboard/mouse; controller passes only through the verified sanitizing hook.

	bool serviceIsRequested() {
		return pimpl->serviceIsRequested();
	}
};