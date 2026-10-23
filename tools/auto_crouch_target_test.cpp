#include "../HCMInternal/AutoCrouchTarget.h"
#include <cassert>
#include <limits>
#include <iostream>

int main()
{
    std::array<float, 3> result;
    const float pi = 3.14159265358979323846f;
    const auto near = [](float a, float b) { return std::abs(a-b) < 0.00001f; };
    assert(autoCrouchTarget({1,2,3}, 0, 0, result));
    assert(near(result[0],1.4f) && near(result[1],2) && near(result[2],3));
    assert(autoCrouchTarget({1,2,3}, pi/2, 0, result));
    assert(near(result[0],1) && near(result[1],2.4f) && near(result[2],3));
    // Looking straight up/down must stay finite and move vertically by 0.4.
    for (float pitch : {pi/2, -pi/2}) {
        assert(autoCrouchTarget({1,2,3}, 0.9f, pitch, result));
        assert(near(result[0],1) && near(result[1],2));
        assert(near(result[2], pitch > 0 ? 3.4f : 2.6f));
    }
    // Distance stays fixed at oblique angles, not 0.4 per axis.
    assert(autoCrouchTarget({0,0,0}, 0.7f, -0.6f, result));
    assert(near(std::sqrt(result[0]*result[0]+result[1]*result[1]+result[2]*result[2]),0.4f));
    assert(!autoCrouchTarget({0,0,0}, std::numeric_limits<float>::quiet_NaN(), 0, result));
    assert(!autoCrouchTarget({0,0,0}, 0, std::numeric_limits<float>::infinity(), result));
    assert(!autoCrouchTarget({0,std::numeric_limits<float>::infinity(),0}, 0, 0, result));
    std::cout << "Auto-crouch target checks passed\n";
}
