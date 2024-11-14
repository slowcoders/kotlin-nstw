/*
 * Copyright 2010-2023 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "ReferenceOps.hpp"
#include "ThreadData.hpp"
#include "ThreadRegistry.hpp"

using namespace kotlin;

// on stack
template<> ALWAYS_INLINE void mm::RefAccessor<true>::beforeStore(ObjHeader*) noexcept {}
template<> ALWAYS_INLINE void mm::RefAccessor<true>::afterStore(ObjHeader*) noexcept {}
template<> ALWAYS_INLINE void mm::RefAccessor<true>::beforeLoad() noexcept {}
template<> ALWAYS_INLINE void mm::RefAccessor<true>::afterLoad() noexcept {}

// on heap
template<> ALWAYS_INLINE void mm::RefAccessor<false>::beforeStore(ObjHeader* value) noexcept {
    gc::beforeHeapRefUpdate(direct(), value, false);
}
template<> ALWAYS_INLINE void mm::RefAccessor<false>::afterStore(ObjHeader* value) noexcept {}
template<> ALWAYS_INLINE void mm::RefAccessor<false>::beforeLoad() noexcept {}
template<> ALWAYS_INLINE void mm::RefAccessor<false>::afterLoad() noexcept {}

template<> PERFORMANCE_INLINE void mm::RefAccessor<true>::store(ObjHeader* desired) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    beforeStore(desired);
    direct_.store(desired);
    afterStore(desired);
}

template<> PERFORMANCE_INLINE void mm::RefAccessor<false>::store(ObjHeader* desired) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    if (gc::isNSTW) {
        gc::nstw_heapRefUpdate(direct_, desired);
    } else {
        beforeStore(desired);
        direct_.store(desired);
        afterStore(desired);
    }
}

template<> PERFORMANCE_INLINE void mm::RefAccessor<true>::storeAtomic(ObjHeader* desired, std::memory_order order) noexcept {
    beforeStore(desired);
    direct_.storeAtomic(desired, order);
    afterStore(desired);
}

template<> PERFORMANCE_INLINE void mm::RefAccessor<false>::storeAtomic(ObjHeader* desired, std::memory_order order) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    beforeStore(desired);
    if (gc::isNSTW) {
        auto result = direct_.exchange(desired, order);
        gc::nstw_afterHeapRefUpdate(result, desired);
    } else {
        direct_.storeAtomic(desired, order);
    }
    afterStore(desired);
}

template<> PERFORMANCE_INLINE ObjHeader* mm::RefAccessor<true>::exchange(ObjHeader* desired, std::memory_order order) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    beforeLoad();
    beforeStore(desired);
    auto result = direct_.exchange(desired, order);
    afterStore(desired);
    afterLoad();
    return result;
}

template<> PERFORMANCE_INLINE ObjHeader* mm::RefAccessor<false>::exchange(ObjHeader* desired, std::memory_order order) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    beforeLoad();
    beforeStore(desired);
    auto result = direct_.exchange(desired, order);
    if (gc::isNSTW) {
        gc::nstw_afterHeapRefUpdate(result, desired);
    }
    afterStore(desired);
    afterLoad();
    return result;
}

template<> PERFORMANCE_INLINE bool mm::RefAccessor<true>::compareAndExchange(ObjHeader*& expected, ObjHeader* desired, std::memory_order order) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    beforeLoad();
    beforeStore(desired);
    bool result = direct_.compareAndExchange(expected, desired, order);
    afterStore(desired);
    afterLoad();
    return result;
}

template<> PERFORMANCE_INLINE bool mm::RefAccessor<false>::compareAndExchange(ObjHeader*& expected, ObjHeader* desired, std::memory_order order) noexcept {
    AssertThreadState(ThreadState::kRunnable);
    beforeLoad();
    beforeStore(desired);
    bool result = direct_.compareAndExchange(expected, desired, order);
    if (result && gc::isNSTW) {
        gc::nstw_afterHeapRefUpdate(expected, desired);
    }
    afterStore(desired);
    afterLoad();
    return result;
}

ALWAYS_INLINE OBJ_GETTER(mm::weakRefReadBarrier, std_support::atomic_ref<ObjHeader*> weakReferee) noexcept {
    RETURN_RESULT_OF(gc::weakRefReadBarrier, weakReferee);
}
