#include "RTGC.h"

// ================ rtgc::pal ===========================

bool rtgc::pal::finalizeObject(rtgc::GCNode* garbage) {
//   auto container = static_cast<ContainerHeader*>(garbage);
//   if (isAggregatingFrozenContainer(container)) {
//     freeAggregatingFrozenContainer(container);
//     return false;
//   }

//   runDeallocationHooks(container);
    rtgc_assert("Not impl" == 0);
  return true;
}

void rtgc::pal::deallocNode(rtgc::GCContext* state, rtgc::GCNode* garbage) {
  rtgc_trace_ref(RTGC_TRACE_FREE, garbage, "deallocNode");
    rtgc_assert("Not impl" == 0);
//   auto container = static_cast<ContainerHeader*>(garbage);
//   rtgc_assert_ref(container, container->getRootRefCount() == 0 || ((int)container->getRootRefCount() == (uint16_t)0xfeee && container->isImmutable()));
// #if !RTGC_DEBUG_MEMORY_LEAKS  
//   freeInObjectPool(container);
// #endif
//   atomicAdd(&allocCount, -1);
  // scheduleDestroyContainer((MemoryState*)state, static_cast<ContainerHeader*>(garbage));
}

ALWAYS_INLINE rtgc::GCContext* rtgc::pal::getCurrentThreadGCContext() {
//   return memoryState;
    rtgc_assert("Not impl" == 0);
    return NULL;
}

void rtgc::pal::processPendingRefCountUpdate(rtgc::GCContext* context) {
//   processDecrements((MemoryState*)context);
    rtgc_assert("Not impl" == 0);
}
