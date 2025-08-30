#ifndef RTGC_PAL_CONFIG_H
#define RTGC_PAL_CONFIG_H

#include "std_support/CStdlib.hpp"
#include <vector>
#include <deque>

namespace rtgc::pal {
#ifdef KONAN_NO_THREADS
    constexpr bool NO_THREADS = true;
#else 
    constexpr bool NO_THREADS = false;
#endif

    typedef struct ObjHeader    _GCObject;
    typedef struct FrameOverlay _GCFrame;

    template <typename T>
    using _Vector = std::vector<T>;

    template <typename T>
    using _Deque = std::deque<T>;
};

#endif // RTGC_PAL_CONFIG_H