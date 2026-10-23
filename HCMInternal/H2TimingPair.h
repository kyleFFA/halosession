#pragma once
#include <cmath>
namespace h2timing
{
    struct Pair { short rate; float dt; };
    enum class WriteResult { Applied, Refused, RolledBack, Uncertain };
    inline bool valid(Pair pair) {
        return pair.rate>=3 && pair.rate<=32766 && std::isfinite(pair.dt) &&
            std::fabs(pair.dt-1.f/pair.rate)<=0.001f/pair.rate;
    }
    inline bool matches(Pair pair, short rate) { return pair.rate==rate && valid(pair); }
    template<class Read, class Write>
    WriteResult writePair(short rate, Read read, Write write)
    {
        Pair old{};
        if (rate<3 || rate>32766 || !read(old) || !valid(old)) return WriteResult::Refused;
        Pair observed{};
        if (write(Pair{rate,1.f/rate}) && read(observed) && matches(observed,rate)) return WriteResult::Applied;
        // A field write may have partially succeeded. Never report restoration
        // or application without reading back both fields. A successful store
        // alone cannot prove another writer did not immediately replace them.
        return write(old) && read(observed) && matches(observed,old.rate) &&
            std::fabs(observed.dt-old.dt)<=0.001f/old.rate ? WriteResult::RolledBack : WriteResult::Uncertain;
    }
}
