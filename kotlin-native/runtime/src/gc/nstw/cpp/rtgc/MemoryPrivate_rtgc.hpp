/*
 * Copyright 2010-2018 JetBrains s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RUNTIME_MEMORYPRIVATE_HPP
#define RUNTIME_MEMORYPRIVATE_HPP

#include "RTGC.h"


typedef enum {
  // Individual state bits used during GC and freezing.
  CONTAINER_TAG_GC_SEEN     = 0x8000,
} ContainerTag;


struct ContainerHeader : public rtgc::GCNode {

  inline unsigned containerSize() const {
    rtgc_assert(this->isGarbageMarked());
    rtgc_assert("Not impl" == 0);
    return 0;//GCNode::refCount();
  }

  inline void setContainerSize(unsigned size) {
    rtgc_assert(this->isGarbageMarked());
    rtgc_assert("Not impl" == 0);
    // GCNode::setObjectRefCount(size);
  }

  inline bool hasContainerSize() {
    return this->isGarbageMarked();
  }


//   #define buffered_flags _rcBits
//   inline bool buffered() const {
// #if USE_RTGC
//     rtgc_assert(!rtgc::ENABLE_BK_GC);
//     return isEnquedToScan();
// #else    
//     return (buffered_flags & CONTAINER_TAG_GC_BUFFERED) != 0;
// #endif
//   }

//   inline void setBuffered() {
// #if USE_RTGC
//     rtgc_assert(!USE_RTGC);
// #else
//     buffered_flags |= CONTAINER_TAG_GC_BUFFERED;
// #endif
//   }

//   inline void resetBuffered() {
// #if USE_RTGC
//     // already cleared in RTGC
//     // rtgc_assert(!isEnquedToScan());
//     // rtgc_assert(isThreadLocal() || isImmutable());
// #else
//     buffered_flags &= ~CONTAINER_TAG_GC_BUFFERED;
// #endif
//   }

  // inline bool seen() const {
  //   rtgc_assert(!isGarbageMarked());
  //   return (buffered_flags & CONTAINER_TAG_GC_SEEN) != 0;
  // }

  // inline void setSeen() {
  //   rtgc_assert_ref(this, !isGarbageMarked());
  //   buffered_flags |= CONTAINER_TAG_GC_SEEN;
  // }

  // inline void resetSeen() {
  //   rtgc_assert_ref(this, !isGarbageMarked());
  //   ref_.resetSeen
  //   buffered_flags &= ~CONTAINER_TAG_GC_SEEN;
  // }

  // Following operations only work on freed container which is in finalization queue.
  // We cannot use 'this' here, as it conflicts with aliasing analysis in clang.
  inline void setNextLink(ContainerHeader* next) {
    *reinterpret_cast<intptr_t*>(this + 1) = ~reinterpret_cast<intptr_t>(next);
  }

  inline ContainerHeader* nextLink() {
    return reinterpret_cast<ContainerHeader*>(~*reinterpret_cast<intptr_t*>(this + 1));
  }
};

ALWAYS_INLINE ContainerHeader* containerFor(const ObjHeader* obj);

// Header for the meta-object.
struct MetaObjHeader {
  // Pointer to the type info. Must be first, to match ArrayHeader and ObjHeader layout.
  const TypeInfo* typeInfo_;
  // Container pointer.
  ContainerHeader* container_;

#ifdef KONAN_OBJC_INTEROP
  void* associatedObject_;
#endif

  // Flags for the object state.
  int32_t flags_;

  struct {
    // Strong reference to the counter object.
    ObjHeader* counter_;
  } WeakReference;
};

extern "C" {

#define MODEL_VARIANTS(returnType, name, ...)            \
   returnType name##Strict(__VA_ARGS__) RUNTIME_NOTHROW; \
   returnType name##Relaxed(__VA_ARGS__) RUNTIME_NOTHROW;

OBJ_GETTER(AllocInstanceStrict, const TypeInfo* type_info) RUNTIME_NOTHROW;
OBJ_GETTER(AllocInstanceRelaxed, const TypeInfo* type_info) RUNTIME_NOTHROW;

OBJ_GETTER(AllocArrayInstanceStrict, const TypeInfo* type_info, int32_t elements);
OBJ_GETTER(AllocArrayInstanceRelaxed, const TypeInfo* type_info, int32_t elements);

OBJ_GETTER(InitThreadLocalSingletonStrict, ObjHeader** location, const TypeInfo* typeInfo, void (*ctor)(ObjHeader*));
OBJ_GETTER(InitThreadLocalSingletonRelaxed, ObjHeader** location, const TypeInfo* typeInfo, void (*ctor)(ObjHeader*));

OBJ_GETTER(InitSingletonStrict, ObjHeader** location, const TypeInfo* typeInfo, void (*ctor)(ObjHeader*));
OBJ_GETTER(InitSingletonRelaxed, ObjHeader** location, const TypeInfo* typeInfo, void (*ctor)(ObjHeader*));

MODEL_VARIANTS(void, SetStackRef, ObjHeader** location, const ObjHeader* object);
MODEL_VARIANTS(void, SetHeapRef, ObjHeader** location, const ObjHeader* object);
MODEL_VARIANTS(void, ZeroStackRef, ObjHeader** location);
MODEL_VARIANTS(void, UpdateStackRef, ObjHeader** location, const ObjHeader* object);
MODEL_VARIANTS(void, UpdateHeapRef, ObjHeader** location, const ObjHeader* object);
MODEL_VARIANTS(void, rtgc_UpdateObjectRef, ObjHeader** location, const ObjHeader* object, const ObjHeader* owner);
MODEL_VARIANTS(void, UpdateHeapRefIfNull, ObjHeader** location, const ObjHeader* object);
MODEL_VARIANTS(void, UpdateReturnRef, ObjHeader** returnSlot, const ObjHeader* object);
MODEL_VARIANTS(void, UpdateHeapRefsInsideOneArray, const ArrayHeader* array, int fromIndex, int toIndex,
               int count); // zzz
MODEL_VARIANTS(void, EnterFrame, ObjHeader** start, int parameters, int count);
MODEL_VARIANTS(void, LeaveFrame, ObjHeader** start, int parameters, int count);
MODEL_VARIANTS(void, rtgc_LeaveFrameAndReturnRef, ObjHeader** frameStart, ObjHeader** returnSlot, const ObjHeader* returnRef);
MODEL_VARIANTS(void, SetCurrentFrame, ObjHeader** start); // zzz

void ReleaseHeapRef(const ObjHeader* object) RUNTIME_NOTHROW;
MODEL_VARIANTS(void, ReleaseHeapRef, const ObjHeader* object);
MODEL_VARIANTS(void, ReleaseHeapRefNoCollect, const ObjHeader* object);

}  // extern "C"

#endif // RUNTIME_MEMORYPRIVATE_HPP
