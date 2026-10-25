#pragma once
#include <cstdint>
namespace worldlines {
// Callers serialize engine and frame callbacks with the existing destruction/render mutex.
class CaptureRoute {
 uint64_t drawn=0,presented=0;
public:
 static bool engineAllowed(bool wire,bool noBloom,bool xray,bool verifiedLateLease){return !wire&&!noBloom&&!xray&&!verifiedLateLease;}
 void engineDrew(){++drawn;}
 bool consumeEarlyDraw(){const bool any=drawn!=presented;presented=drawn;return any;}
};
}
