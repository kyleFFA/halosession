#pragma once
#include "pch.h"
#include "IOptionalCheat.h"
#include "GameState.h"
#include "DIContainer.h"

// PRIVATE (collision-viewer-private branch - do not push to public).
// Halo 2 (MCC 1.3528) collision wireframe + live Havok TIM overlay. Port of the standalone H2H2CollisionViewer
// (upstream H2CollisionViewer); the H2CV_* files are generated from it by port_to_hcm.py.
// Lines are drawn inside the game's frame at the first-person pass (sub_1807E0C60) against the game's own world
// depth and projection, or after bloom (sub_180951EC0), or at Present against the viewer's own collision depth.
class H2CollisionViewer : public IOptionalCheat
{
private:
	class Impl;
	std::unique_ptr<Impl> pimpl;

public:
	H2CollisionViewer(GameState gameImpl, IDIContainer& dicon);
	~H2CollisionViewer();
    static std::string status();

	std::string_view getName() override { return nameof(H2CollisionViewer); }
};
