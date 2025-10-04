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
    RT_YOUNG = 0x00,
    RT_ACYCLIC = 0x01,
    RT_RAMIFIED_FULL_RC = 0x02,
    RT_RAMIFIED_WITH_ANCHOR = 0x03,
    RT_TRIBUTARY_WITH_SHORTCUT = 0x04,
    RT_TRIBUTARY_WITH_OFFSET = 0x05,
    RT_TRIBUTARY_FULL_RC = 0x06,
    RT_CIRCUIT = 0x07,

    REF_TYPE_MASK = 0x7,



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


#define COMMON_BITS()   \
    uint64_t color: 4;  \
    uint64_t type: 4;   \
    uint64_t node: 8;   \


struct RTGCRefBits {
    COMMON_BITS()
    uint64_t ext_rc: 16;
    uint64_t rc:  32;  
};

struct RtYoung {
    COMMON_BITS()
    uint64_t reserved:  16;  
    uint64_t idx:  32;  
};

struct RtAcyclic {
    COMMON_BITS()
    uint64_t ext_rc:  48;  
};

struct RtRamified_FullRc {
    COMMON_BITS()
    uint64_t ext_rc: 48;  
};

struct RtRamified_Anchor {
    COMMON_BITS()
    uint64_t ext_rc: 8;  
    uint64_t addr:  40;  
};

// Long Shorcut 사용 시 반드시 Anchor 필요. --> Dest 가 Deallocation 될 수 있음.
struct RtTributary_Shorcut {
    COMMON_BITS()
    uint64_t ext_rc: 8;  
    uint64_t shortcut:  40;  
};

struct RtTributary {
    COMMON_BITS()
    uint64_t ext_rc: 8;  
    uint64_t rc:  16;
    uint64_t offset:  16;  
};

struct RtTributary_FullRc {
    COMMON_BITS()
    uint64_t ext_rc: 8;  
    uint64_t rc:  32;
};

#define ENABLE_OPT_TRIBUTARY_MARK    false

struct GCNodeRef {
    friend struct GCNode;
protected:    
    union {
        ref_count_t refCount_flags_;
        RTGCRefBits refBits_;
        RtAcyclic acyclicBits_;
        RtRamified_Anchor ramifiedBits_;
    };
    GCNode* anchor_;

    inline int refType() const {
        return refCount_flags_ & REF_TYPE_MASK;
    }


    inline bool isYoung() const {
        return false;// refType() == RT_YOUNG;
    }

    inline bool isTributary() const {
        return refType() >= RT_TRIBUTARY_WITH_SHORTCUT;
    }

    inline bool isThreadLocal() const {
        return false; //(refCount_flags_ & T_MASK) == 0;
    }

    inline bool isImmutable() const {
        return false; // (refCount_flags_ & T_IMMUTABLE) != 0;
    }

    inline bool isImmutableRoot() const {
        return false; // (refCount_flags_ & T_MASK) == T_IMMUTABLE;
    }

    inline bool isAcyclic() const {
        return refType() == RT_ACYCLIC;
    }

    inline bool isDestroyed() const {
        return (refCount_flags_ & FLAG_DESTROYED) != 0;
    }

    inline bool isUnstable() const {
        ref_count_t check_unstable_mask = RTGC_ROOT_REF_MASK | RTGC_SAFE_REF_MASK | FLAG_ACYCLIC | T_IMMUTABLE;
        return (refCount_flags_ & check_unstable_mask) == 0;
        // return get_color() == S_UNSTABLE;
    }

    bool isEnquedToScan() const {
        return (refCount_flags_ & FLAG_SUSPECTED) != 0;
    }

    bool isStableAnchored() const {
        return get_color() == S_STABLE_ANCHORED;
    }

    //============================================//

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


    // GCNode* replaceAnchorIfNull(GCNode* ref, GCNode* anchor) {
    //     assert(!isTributary());
    //     uint64_t new_addr = (uint64_t*)anchor - (uint64_t*)ref;
    //     if (node.addr == 0) {
    //         node.addr = new_addr;
    //         return NULL;
    //     }
    //     return node.addr == new_addr ? NULL : getAnchorOf(ref);
    // }


    inline GCNode* getAnchorOf(const GCNode* node) const {
        rtgc_assert(!isTributary());
        return (GCNode*)(uint64_t*)node + refBits_.addr;
    }

    inline void setAnchor_unsafe(GCNode* node) {
        rtgc_assert(!isTributary());
        refBits_.root = (uint64_t*)node - (uint64_t*)anchor_;
        rtgc_assert(node == getAnchorOf(node));
    }

    inline signed_ref_count_t refCount() const {
        return refBits()->rc;
    }

    inline unsigned getExternalRefCount() const {
        return refBits()->ext_rc;
    }

    inline void setExternalRefCount(int refCount) {
        (reinterpret_cast<RTGCRefBits*>(&refCount_flags_))->ext_rc = refCount;
    }

    ref_count_t refCountBitFlags() const {
        return refCount_flags_;
    }


    inline bool hasExternalRef() const {
        return refBits()->ext_rc != 0;
    }


    inline void setRefCount(signed_ref_count_t refCount) {
        refCount_flags_ = (refCount_flags_ & (RTGC_ROOT_REF_INCREMENT-1)) + (refCount * RTGC_ROOT_REF_INCREMENT);
    }

    inline void setRefCountAndFlags(uint32_t refCount, uint16_t flags) {
        refCount_flags_ = (refCount * RTGC_ROOT_REF_INCREMENT) + flags;
    }

    inline void setObjectRefCount(unsigned size) {
        ((RTGCRefBits*)&refCount_flags_)->rc = size;
    }

    template <bool Atomic>
    inline ref_count_t increaseAcyclicRefCount() {
        rtgc_assert(isAcyclic());
        pal::bit_add<Atomic>(&refCount_flags_, 0x100);
        return acyclicBits_.rc;
    }

    template <bool Atomic>
    inline ref_count_t decreaseAcyclicRefCount() {
        rtgc_assert(isAcyclic());
        pal::bit_add<Atomic>(&refCount_flags_, -0x100);
        return acyclicBits_.rc;
    }

    bool increaseExternalRef() {
        bool tributary = isTributary();
        if (~refBits_.ext_rc == 0) {
            if (tributary) {
                refBits_.rc ++; 
            } else {
                refBits_.rc = refBits_.ext_rc + 1;
                setTributary(true);
                return false;
            }
        } else {
            refBits_.ext_rc ++;
            if (tributary) {
                refBits_.rc ++; 
            }
        }
        return true;
    }

    bool hasUnsafeReferrer() const {
        return false;        
    }

    bool decreaseExternalRef() {
        bool unstable = hasUnsafeReferrer();
        if (unstable) {
            rtgc_assert(refBits_.rc > 0); 
            refBits_.rc --;
            if (refBits_.ext_rc != 0) {
                refBits_.ext_rc --; 
            }
        } else {
            rtgc_assert(refBits_.ext_rc > 0); 
            refBits_.ext_rc --; 
        }
        return true;
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