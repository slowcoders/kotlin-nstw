#ifndef RTGC_PAL_H
#define RTGC_PAL_H

#include "RTGC_PAL_config.h"

namespace rtgc {

    typedef const pal::_GCObject*   GCRef;
    typedef pal::_GCFrame           GCFrame;

    struct GCContext;
    struct GCNode;

    namespace pal {

        class ChildNodeIterator;

        template <bool notNull=false>
        inline GCNode* toNode(GCRef obj);

        inline GCRef toObject(const GCNode* node);

        void markPublished(GCNode* node);

        GCContext* getCurrentThreadGCContext();
        bool finalizeObject(GCNode* node);
        void deallocNode(GCContext* context, GCNode* node);
        void processPendingRefCountUpdate(GCContext* context);
        bool insideFrame(GCFrame* frame, void* location);
        bool isPublished(GCRef obj);

        template<bool Atomic, typename T>
        static inline T bit_or(volatile T* where, T bits) {
            if (Atomic && !NO_THREADS) {
                return __sync_fetch_and_or(where, bits);
            } else {
                return (*where |= bits);
            }
        }

        template<bool Atomic, typename T>
        static inline T bit_and(volatile T* where, T bits) {
            if (Atomic && !NO_THREADS) {
                return __sync_fetch_and_and(where, bits);
            } else {
                return (*where &= bits);
            }
        }

        template<bool Atomic, typename T>
        static inline T bit_xor(volatile T* where, T bits) {
            if (Atomic && !NO_THREADS) {
                return __sync_fetch_and_xor(where, bits);
            } else {
                return (*where ^= bits);
            }
        }

        template<bool Atomic, typename T>
        static inline T bit_add(volatile T* where, T increment) {
            if (Atomic && !NO_THREADS) {
                return __sync_add_and_fetch(where, (T)increment);
            } else {
                return (*where += increment);
            }
        }

        template<bool Atomic, typename T>
        static inline bool comp_set(volatile T* where, T expectedValue, T newValue) {
            T oldValue = *where;
            if (oldValue != expectedValue) return false;
            if (Atomic && !NO_THREADS) {
                return __sync_bool_compare_and_swap(where, expectedValue, newValue);
            } else {
                *where = newValue;
                return true;
            }
        }

        template<bool Atomic, typename T>
        static inline T comp_xchg(volatile T* where, T expectedValue, T newValue) {
            if (Atomic && !NO_THREADS) {
                return __sync_val_compare_and_swap(where, expectedValue, newValue);
            }
            T oldValue = *where;
            if (oldValue == expectedValue) {
                *where = newValue;
            }
            return oldValue;
        }
    };
};

#endif // RTGC_PAL_H