#pragma once

#include "ConcurrentMark.hpp"

namespace kotlin::gc::internal {

struct NstwGCTraits {
    static constexpr auto kName = "No-stop-the-world mark & sweep + ref counting GC";
    static constexpr bool kConcurrentSweep = true;
    using Mark = mark::ConcurrentMark;
};

}