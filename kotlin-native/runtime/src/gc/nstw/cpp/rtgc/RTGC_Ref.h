#ifndef RTGC_REF_H
#define RTGC_REF_H

#include "RTGC_def.h"
#include "RTGC_PAL.h"

namespace rtgc {

typedef uint64_t ref_count_t;
typedef int64_t  signed_ref_count_t;
typedef uint64_t unsigned_ref_count_t;

constexpr int  RTGC_SAFE_REF_BITS   = 16;   // 12  // 4K
constexpr int  RTGC_ROOT_REF_BITS   = 16;   // 12  // 4K
constexpr int  RTGC_OBJECT_REF_BITS = 16;   // 28  // 256M
constexpr int  RTGC_NODE_FLAGS_BITS = 16;

constexpr ref_count_t RTGC_SAFE_REF_MASK = (((ref_count_t)1 << RTGC_SAFE_REF_BITS) - 1) << (RTGC_NODE_FLAGS_BITS);
constexpr ref_count_t RTGC_ROOT_REF_MASK = (((ref_count_t)1 << RTGC_ROOT_REF_BITS) - 1) << (RTGC_NODE_FLAGS_BITS + RTGC_SAFE_REF_BITS);
constexpr ref_count_t RTGC_OBJECT_REF_MASK = (ref_count_t)-1 << (RTGC_ROOT_REF_BITS + RTGC_SAFE_REF_BITS + RTGC_NODE_FLAGS_BITS);
constexpr ref_count_t RTGC_NODE_FLAGS_MASK = ((ref_count_t)1 << RTGC_NODE_FLAGS_BITS) - 1;

#define RTGC_SAFE_REF_INCREMENT     ((rtgc::ref_count_t)1 << rtgc::RTGC_NODE_FLAGS_BITS)
#define RTGC_ROOT_REF_INCREMENT     ((rtgc::ref_count_t)RTGC_SAFE_REF_INCREMENT << rtgc::RTGC_SAFE_REF_BITS)
#define RTGC_OBJECT_REF_INCREMENT   ((rtgc::ref_count_t)RTGC_ROOT_REF_INCREMENT << rtgc::RTGC_ROOT_REF_BITS)

enum GCFlags {
    T_IMMUTABLE     = 0x0400,
    T_SHARED        = 0x0800,
    T_MASK          = T_IMMUTABLE | T_SHARED,

    FLAG_ACYCLIC    = 0x0100,
    FLAG_TRIBUTARY  = 0x0200,
    FLAG_MARKED     = 0x2000,
    FLAG_DESTROYED  = 0x1000,

    S_WHITE             = 0,
    S_PINK              = 0x01,  // Small Circuit 의 진입점.
    S_RED               = 0x02,  // Scan 중인 Circuit;
    S_BROWN             = 0x03,  // Scan 이 끝난 Circuit
    
    S_GRAY              = 0x08,
    S_BLACK             = 0x09,
    S_CYCLIC_BLACK      = 0x0A,
    S_TRIBUTARY_BLACK      = 0x0B,

    S_STABLE_ANCHORED       = 0x0C,

    // S_UNSTABLE_TAIL     = 0x0E,
    // S_UNSTABLE          = 0x0F,
    S_UNSTABLE_0 = 0,
    S_MASK              = 0x0F,

    FLAG_SUSPECTED      = 0x10,
};

struct RTGCRefBits {
    uint64_t color: 4;
    uint64_t type: 4;
    uint64_t node: RTGC_NODE_FLAGS_BITS - 8;
    uint64_t ext_rc: RTGC_SAFE_REF_BITS;
    uint64_t root: RTGC_ROOT_REF_BITS;
    uint64_t rc:  RTGC_OBJECT_REF_BITS;  
};

#define ENABLE_OPT_TRIBUTARY_MARK    false

struct GCNodeRef {
    friend struct GCNode;
protected:    
    union {
        ref_count_t refCount_flags_;
        RTGCRefBits refBits_;
    };
    GCNode* anchor_;

    inline bool isTributary() const {
        return (refCount_flags_ & FLAG_TRIBUTARY) != 0;
    }

    inline void setTributary(bool isTributary) {
        if (isTributary) {
            refCount_flags_ |= FLAG_TRIBUTARY;
        } else {
            refCount_flags_ &= ~FLAG_TRIBUTARY;
        }
    }
    
    // bool hasNodeRef() {
    //     assert(!isTributary());
    // }


    // GCNode* getAnchorOf(GCNode* ref) {
    //     assert(!isTributary());
    //     return (GCNode*)(uint64_t*)ref + node.addr;
    // }


    // GCNode* replaceAnchorIfNull(GCNode* ref, GCNode* anchor) {
    //     assert(!isTributary());
    //     uint64_t new_addr = (uint64_t*)anchor - (uint64_t*)ref;
    //     if (node.addr == 0) {
    //         node.addr = new_addr;
    //         return NULL;
    //     }
    //     return node.addr == new_addr ? NULL : getAnchorOf(ref);
    // }


    // void setAnchor(GCNode* ref, GCNode* anchor) {
    //     assert(!isTributary());
    //     node.addr = (uint64_t*)anchor - (uint64_t*)ref;
    // }

    inline bool isThreadLocal() const {
        return (refCount_flags_ & T_MASK) == 0;
    }

    inline bool isImmutable() const {
        return (refCount_flags_ & T_IMMUTABLE) != 0;
    }

    inline bool isImmutableRoot() const {
        return (refCount_flags_ & T_MASK) == T_IMMUTABLE;
    }

    // inline bool isTributary() const {
    //     return (refCount_flags_ & FLAG_TRIBUTARY) != 0;
    // }


    inline bool isAcyclic() const {
        return (refCount_flags_ & (FLAG_ACYCLIC|T_IMMUTABLE)) != 0;
    }

    inline bool isDestroyed() const {
        return (refCount_flags_ & FLAG_DESTROYED) != 0;
    }

    inline bool isUnstable() const {
        ref_count_t check_unstable_mask = RTGC_ROOT_REF_MASK | RTGC_SAFE_REF_MASK | FLAG_ACYCLIC | T_IMMUTABLE;
        return (refCount_flags_ & check_unstable_mask) == 0;
        // return get_color() == S_UNSTABLE;
    }

    bool isSuspected() const {
        return (refCount_flags_ & FLAG_SUSPECTED) != 0;
    }

    bool isStableAnchored() const {
        return get_color() == S_STABLE_ANCHORED;
    }

    //============================================//

    inline GCNode* getAnchor() const {
        return anchor_;
    }

    inline void setAnchor_unsafe(GCNode* node) {
        anchor_ = node;
    }

    inline signed_ref_count_t refCount() const {
        return refCount_flags_ / RTGC_ROOT_REF_INCREMENT;
    }

    ref_count_t refCountBitFlags() const {
        return refCount_flags_;
    }

    inline ref_count_t getRootRefCount() const {
        return refBits()->root;
    }

    inline unsigned getObjectRefCount() const {
        return refBits()->rc;
    }

    inline unsigned getSafeRefCount() const {
        return refBits()->ext_rc;
    }

    inline void setSafeRefCount(int refCount) {
        (reinterpret_cast<RTGCRefBits*>(&refCount_flags_))->ext_rc = refCount;
    }


    inline bool hasRootRef() const {
        return refBits()->root != 0;
    }

    inline int hasObjectRef() const {
        return refBits()->rc != 0;
    }


    inline void setRefCount(signed_ref_count_t refCount) {
        refCount_flags_ = (refCount_flags_ & (RTGC_ROOT_REF_INCREMENT-1)) + (refCount * RTGC_ROOT_REF_INCREMENT);
    }

    inline void setRefCountAndFlags(uint32_t refCount, uint16_t flags) {
        refCount_flags_ = (refCount * RTGC_ROOT_REF_INCREMENT) + flags;
    }


    inline void setRootRefCount(unsigned size) {
        ((RTGCRefBits*)&refCount_flags_)->root = size;
    }

    inline void setObjectRefCount(unsigned size) {
        ((RTGCRefBits*)&refCount_flags_)->rc = size;
    }

    inline bool isYoung() const {
        return false;// (refCount_flags_ & T_MASK) == T_SHARED;
    }


    void increaseExternalRef(bool _isTributary) {
        assert(_isTributary == isTributary());
        if (~refBits_.ext_rc == 0) {
            if (_isTributary) {
                refBits_.rc ++; 
            } else {
                refBits_.rc = refBits_.ext_rc + 1;
                setTributary(true);
            }
        } else {
            refBits_.ext_rc ++;
            if (_isTributary) {
                refBits_.rc ++; 
            }
        }
    }

    //////

    uint8_t& byteFlags() {
        return *(uint8_t*)(&refCount_flags_);
    }

    uint8_t byteFlags() const {
        return *(uint8_t*)(&refCount_flags_);
    }

    inline const RTGCRefBits* refBits() const {
        return reinterpret_cast<const RTGCRefBits*>(&refCount_flags_);
    }


    int get_color() const {
        return *(uint8_t*)&refCount_flags_ & S_MASK; 
    }

    bool onCircuit() {
        uint8_t color = get_color();
        return color == S_RED || color == S_PINK || color == S_BROWN;
    }

    void set_color(uint8_t color) {
        rtgc_assert((color & S_MASK) == color);
        // TODO read and write is not thread-safe
        byteFlags() = (byteFlags() & ~S_MASK) | color; 
    }

    inline bool marked() const {
        return (refCount_flags_ & FLAG_MARKED) != 0;
    }

    inline void mark() {
        refCount_flags_ |= FLAG_MARKED;
    }

    inline void unMark() {
        refCount_flags_ &= ~FLAG_MARKED;
    }

};

}; // namespace RTGC

#endif // RTGC_REF_H

// RTGC RTGC_ROOT_REF_INCREMENT   100000000
// RTGC RTGC_OBJECT_REF_INCREMENT 1000000000000