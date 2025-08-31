/*
 * Copyright 2010-2021 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "GCImpl.hpp"

#include <memory>

#include "CompilerConstants.hpp"
#include "GC.hpp"
#include "GCStatistics.hpp"
#include "MarkAndSweepUtils.hpp"
#include "ObjectOps.hpp"

using namespace kotlin;

gc::GC::ThreadData::ThreadData(GC& gc, mm::ThreadData& threadData) noexcept :
    impl_(std::make_unique<Impl>(gc.impl().mark_, threadData)) {}

gc::GC::ThreadData::~ThreadData() = default;

void gc::GC::ThreadData::OnSuspendForGC() noexcept {
    impl_->mark_.onSuspendForGC();
}

void gc::GC::ThreadData::safePoint() noexcept {
    impl_->mark_.onSafePoint();
}

void gc::GC::ThreadData::onThreadRegistration() noexcept {
    impl_->barriers_.onThreadRegistration();
}

PERFORMANCE_INLINE void gc::GC::ThreadData::onAllocation(ObjHeader* object) noexcept {
    /* 
    AllocInstance(Memory.cpp)
        mm::AllocateObject(ObjectOps.cpp)
            auto* object = threadData->allocator().allocateObject(typeInfo);
            threadData->gc().onAllocation(object);
    */
    impl_->barriers_.onAllocation(object);
}

gc::GC::GC(alloc::Allocator& allocator, gcScheduler::GCScheduler& gcScheduler) noexcept :
    impl_(std::make_unique<Impl>(allocator, gcScheduler, compiler::gcMutatorsCooperate(), compiler::auxGCThreads())) {
    RuntimeLogInfo({kTagGC}, "%s GC initialized", internal::NstwGCTraits::kName);
}

gc::GC::~GC() {
    impl_->state_.shutdown();
}

void gc::GC::ClearForTests() noexcept {
    GCHandle::ClearForTests();
}

// static
PERFORMANCE_INLINE void gc::GC::processObjectInMark(void* state, ObjHeader* object) noexcept {
    gc::internal::processObjectInMark<gc::mark::ConcurrentMark::MarkTraits>(state, object);
}

// static
PERFORMANCE_INLINE void gc::GC::processArrayInMark(void* state, ArrayHeader* array) noexcept {
    gc::internal::processArrayInMark<gc::mark::ConcurrentMark::MarkTraits>(state, array);
}

int64_t gc::GC::Schedule() noexcept {
    return impl_->state_.schedule();
}

void gc::GC::WaitFinished(int64_t epoch) noexcept {
    impl_->state_.waitEpochFinished(epoch);
}

void gc::GC::WaitFinalizers(int64_t epoch) noexcept {
    impl_->state_.waitEpochFinalized(epoch);
}

PERFORMANCE_INLINE void gc::beforeHeapRefUpdate(mm::DirectRefAccessor ref, ObjHeader* value, bool loadAtomic) noexcept {
    barriers::beforeHeapRefUpdate(ref, value, loadAtomic);
}

PERFORMANCE_INLINE OBJ_GETTER(gc::weakRefReadBarrier, std_support::atomic_ref<ObjHeader*> weakReferee) noexcept {
    RETURN_OBJ(gc::barriers::weakRefReadBarrier(weakReferee));
}

PERFORMANCE_INLINE bool gc::isMarked(ObjHeader* object) noexcept {
    return alloc::objectDataForObject(object).marked();
}

PERFORMANCE_INLINE bool gc::tryResetMark(GC::ObjectData& objectData) noexcept {
    return objectData.tryResetMark();
}

ALWAYS_INLINE bool gc::barriers::ExternalRCRefReleaseGuard::isNoop() {
    return false;
}
PERFORMANCE_INLINE gc::barriers::ExternalRCRefReleaseGuard::ExternalRCRefReleaseGuard(mm::DirectRefAccessor ref) noexcept : impl_(ref) {}
PERFORMANCE_INLINE gc::barriers::ExternalRCRefReleaseGuard::ExternalRCRefReleaseGuard(ExternalRCRefReleaseGuard&& other) noexcept = default;
PERFORMANCE_INLINE gc::barriers::ExternalRCRefReleaseGuard::~ExternalRCRefReleaseGuard() noexcept = default;
PERFORMANCE_INLINE gc::barriers::ExternalRCRefReleaseGuard& gc::barriers::ExternalRCRefReleaseGuard::ExternalRCRefReleaseGuard::operator=(
        ExternalRCRefReleaseGuard&&) noexcept = default;

// static
ALWAYS_INLINE uint64_t type_layout::descriptor<gc::GC::ObjectData>::type::size() noexcept {
    return sizeof(gc::GC::ObjectData);
}

// static
ALWAYS_INLINE size_t type_layout::descriptor<gc::GC::ObjectData>::type::alignment() noexcept {
    return alignof(gc::GC::ObjectData);
}

// static
ALWAYS_INLINE gc::GC::ObjectData* type_layout::descriptor<gc::GC::ObjectData>::type::construct(uint8_t* ptr) noexcept {
    return new (ptr) gc::GC::ObjectData();
}

void gc::GC::onEpochFinalized(int64_t epoch) noexcept {
    GCHandle::getByEpoch(epoch).finalizersDone();
    impl_->state_.finalized(epoch);
}

extern "C" PERFORMANCE_INLINE RUNTIME_NOTHROW void rtgc_UpdateObjectRef(ObjHeader** location, const ObjHeader* object, const ObjHeader* owner) {
    mm::RefAccessor<false>{location}.store(const_cast<ObjHeader*>(object));
    /*
        AssertThreadState(ThreadState::kRunnable);
        beforeStore(desired);
        direct_.storeAtomic(desired, order);
        afterStore(desired);
        ----
        gc::beforeHeapRefUpdate(direct_, desired, false);
            barriers::beforeHeapRefUpdate(ref, value, loadAtomic);
                gc::barriers::beforeHeapRefUpdate()
                    if (__builtin_expect(phase == BarriersPhase::kMarkClosure, false)) {
                        beforeHeapRefUpdateSlowPath(ref, value, loadAtomic);
                            auto& markQueue = *threadData.gc().impl().mark_.markQueue();
                            gc::mark::ConcurrentMark::MarkTraits::tryEnqueue(markQueue, prev);
                    }
    */
}

extern "C" PERFORMANCE_INLINE RUNTIME_NOTHROW void rtgc_UpdateVolatileObjectRef(ObjHeader** location, const ObjHeader* object, const ObjHeader* owner) {
    mm::RefAccessor<false>{location}.storeAtomic(const_cast<ObjHeader*>(object), std::memory_order_seq_cst);
    /*
        AssertThreadState(ThreadState::kRunnable);
        beforeStore(desired);
        direct_.storeAtomic(desired, order);
        afterStore(desired);
    */
}

extern "C" PERFORMANCE_INLINE RUNTIME_NOTHROW void rtgc_UpdateStaticRef(ObjHeader** location, const ObjHeader* object) {
    mm::RefAccessor<false>{location}.storeAtomic(const_cast<ObjHeader*>(object), std::memory_order_seq_cst);
}


// extern "C" PERFORMANCE_INLINE RUNTIME_NOTHROW OBJ_GETTER(rtgc_CompareAndSwapVolatileHeapRef, ObjHeader** location, ObjHeader* expectedValue, ObjHeader* newValue) {
//     ObjHeader* actual = expectedValue;
//     mm::RefAccessor<false>{location}.compareAndExchange(actual, newValue, std::memory_order_seq_cst);
//     RETURN_OBJ(actual);
// }

// extern "C" PERFORMANCE_INLINE RUNTIME_NOTHROW bool rtgc_CompareAndSetVolatileHeapRef(ObjHeader** location, ObjHeader* expectedValue, ObjHeader* newValue) {
//     return mm::RefAccessor<false>{location}.compareAndExchange(expectedValue, newValue, std::memory_order_seq_cst);
// }

// extern "C" PERFORMANCE_INLINE RUNTIME_NOTHROW OBJ_GETTER(rtgc_GetAndSetVolatileHeapRef, ObjHeader** location, ObjHeader* newValue) {
//     RETURN_OBJ(mm::RefAccessor<false>{location}.exchange(newValue, std::memory_order_seq_cst));
// }
