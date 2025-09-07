#ifndef RTGC_PAL_IMPL_H
#define RTGC_PAL_IMPL_H

// #include "Memory.h"
#include "ObjectTraversal.hpp"
#include "Allocator.hpp"
#include <std_support/Atomic.hpp>
#include "RTGC_PAL.h"
#include "../GCImpl.hpp"


namespace rtgc::pal {

constexpr int kFrameOverlaySlots = sizeof(FrameOverlay) / sizeof(ObjHeader**);

inline GCRef toObject(const GCNode* node) {
    rtgc_assert_ref(node, !node->isImmutableRoot());
    return reinterpret_cast<GCRef>(node + 1);
}


template <bool notNull>
inline GCNode* toNode(GCRef obj) {
    if (notNull) {
        rtgc_assert(obj != NULL);
    } else if (obj == NULL) {
        return NULL;
    }

    kotlin::gc::GC::ObjectData& objectData = kotlin::alloc::objectDataForObject((ObjHeader*)obj);
    void* node = &objectData;
    return static_cast<GCNode*>(node);
}
 
template <bool _volatile>
inline void replaceYoungRef(GCRef* location, GCRef object) {
    if (!_volatile) {
        kotlin::mm::RefAccessor<false>{(ObjHeader**)location}.store(const_cast<ObjHeader*>(object));
    } else {
        kotlin::mm::RefAccessor<false>{(ObjHeader**)location}.storeAtomic(const_cast<ObjHeader*>(object), std::memory_order_seq_cst);
    }
}


inline bool isPublished(GCRef obj) {
#if ENABLE_RTGC_MM    
    rtgc_assert(obj != NULL);
    rtgc_assert((reinterpret_cast<uintptr_t>(obj) & OBJECT_TAG_MASK) == 0);
    uintptr_t bits = reinterpret_cast<uintptr_t>(obj->typeInfoOrMeta_);
    return (bits & OBJECT_TAG_PUBLISHED) != 0;
#else 
    return true;
#endif
}

inline void markPublished(GCNode* node) {
#if ENABLE_RTGC_MM    
    GCRef obj = toObject(node);
    rtgc_assert(!isPublished(obj));
    *(uintptr_t*)&obj->typeInfoOrMeta_ |= OBJECT_TAG_PUBLISHED;
#endif
}

inline GCRef* getRefSlotStart(GCFrame* frame) {
    return reinterpret_cast<GCRef*>(frame) + frame->parameters + kFrameOverlaySlots;  
}

inline GCRef* getEndOfFrameSlot(GCFrame* frame) {
    return reinterpret_cast<GCRef*>(frame) + frame->count;  
}

inline int getRefSlotLength(GCFrame* frame) {
    return frame->count - kFrameOverlaySlots - frame->parameters;  
}

inline GCFrame* getPreviousFrame(GCFrame* frame) {
    return frame->previous;  
}

inline void setPreviousFrame(GCFrame* frame, GCFrame* prevFrame) {
    frame->previous = prevFrame;  
}

inline bool insideFrame(GCFrame* frame, void* location) {
  return frame != NULL && ((uint32_t)((ObjHeader**)location - (ObjHeader**)frame) < (uint32_t)frame->count);
}


template <bool includeNull=false, typename Func>
static inline void foreachFrameRefs(FrameOverlay* frame, Func process) {
    GCRef* end   = pal::getEndOfFrameSlot(frame);  
    for (GCRef* stack = pal::getRefSlotStart(frame); stack < end; stack++) {
        GCNode* node = pal::toNode(*stack);
        if (includeNull || (node != nullptr)) process(node);
    }
}


class RefFieldIterator {
    uintptr_t _baseAddr;
    int  _count;
    union {
        GCRef* _pItem;
        const int32_t* _pOffset;
    };

public:    
    RefFieldIterator(GCNode* anchor) {
        const ObjHeader* object =  toObject(anchor);
        const TypeInfo* typeInfo = object->type_info();
        if (typeInfo == theArrayTypeInfo) {
            const ArrayHeader* array = object->array();
            _baseAddr = 0;
            _count = array->count_;
            _pItem = (GCRef*)ArrayAddressOfElementAt(array, 0);
        } else {
            _baseAddr = reinterpret_cast<uintptr_t>(object);
            _count = typeInfo->objOffsetsCount_;
            _pOffset = &typeInfo->objOffsets_[0];
        }
    }    

    bool isArray() const {
        return _baseAddr == 0;
    }

    GCRef* nextField() {
        if (--_count < 0) return NULL;

        if (isArray()) {
            return _pItem ++;
        } else {
            return reinterpret_cast<GCRef*>(_baseAddr + *_pOffset ++);
        }
    }    

};


class ChildNodeIterator : public RefFieldIterator {

public:    
    ChildNodeIterator(GCNode* anchor) : RefFieldIterator(anchor) {}

    GCNode* nextNode() {        
        GCRef* field;
        while ((field = nextField()) != NULL) {
            GCNode* node = toNode(*field);
            if (node != NULL) return node;
        }
        return NULL;
    }    
};

}; // namespace rtgc::pal

#endif // RTGC_PAL_IMPL_H