#pragma once
#include <array>
#include <cmath>

// Matches Halo3Spot's fixed forward move, including pitch. No game access.
inline bool autoCrouchTarget(std::array<float, 3> origin, float yaw, float pitch,
    std::array<float, 3>& target)
{
    if (!std::isfinite(yaw) || !std::isfinite(pitch)) return false;
    for (float coordinate : origin) if (!std::isfinite(coordinate)) return false;
    const double x = std::cos(double(pitch)) * std::cos(double(yaw));
    const double y = std::cos(double(pitch)) * std::sin(double(yaw));
    const double z = std::sin(double(pitch));
    const double length = std::sqrt(x*x + y*y + z*z);
    target = {origin[0] + float(0.4*x/length), origin[1] + float(0.4*y/length), origin[2] + float(0.4*z/length)};
    for (float coordinate : target) if (!std::isfinite(coordinate)) return false;
    return true;
}
