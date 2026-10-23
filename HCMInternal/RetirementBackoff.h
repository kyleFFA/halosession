#pragma once
#include <chrono>
#include <algorithm>
class RetirementBackoff {
    using Clock=std::chrono::steady_clock;
    Clock::time_point next{};
    unsigned delay=0;
public:
    bool ready(Clock::time_point now) const { return now>=next; }
    unsigned failed(Clock::time_point now) {
        delay=delay?std::min(delay*2,2000u):50u;
        next=now+std::chrono::milliseconds(delay);return delay;
    }
    void clear() { next={};delay=0; }
};
