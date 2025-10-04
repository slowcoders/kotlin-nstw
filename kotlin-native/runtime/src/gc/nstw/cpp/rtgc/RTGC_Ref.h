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
    RT_RAMIFIED_with_ANCHOR = 0x03,
    RT_TRIBUTARY_with_SHORTCUT = 0x04,
    RT_TRIBUTARY_with_OFFSET = 0x05,
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

struct RtTributary_Offset {
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

struct RtCircuit {
    COMMON_BITS()
    uint64_t rc:  8;  
    uint64_t circuit_id:  32;
};

#define ENABLE_OPT_TRIBUTARY_MARK    false

struct GCNodeRef {
    friend struct GCNode;
protected:    
    union {
        ref_count_t refCount_flags_;
        RTGCRefBits refBits_;
        RtYoung youngBits_;
        RtCircuit circuitBits_;
        RtAcyclic acyclicBits_;
        RtRamified_Anchor ramifiedAnchorBits_;        
        RtRamified_FullRc ramifiedFullRcBits_;        
        RtTributary_Shorcut tributaryShorcutBits_;
        RtTributary_Offset tributaryOffsetBits_;
        RtTributary_FullRc tributaryFullRcBits_;
    };
    GCNode* anchor_;

    inline int refType() const {
        return refCount_flags_ & REF_TYPE_MASK;
    }

    inline void setRefType(int type) {
        refCount_flags_ = (refCount_flags_ & ~REF_TYPE_MASK) | type;
    }

    inline bool isYoung() const {
        return false;// refType() == RT_YOUNG;
    }

    inline bool isTributary() const {
        return refType() >= RT_TRIBUTARY_with_SHORTCUT;
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
        rtgc_assert(!isYoung());
        rtgc_assert(!isDestroyed() && !isEnquedToScan());
        return getExternalRefCount() == 0;
    }

    bool isEnquedToScan() const {
        return (refCount_flags_ & FLAG_SUSPECTED) != 0;
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
        rtgc_assert(refType() == RT_RAMIFIED_with_ANCHOR);
        return (GCNode*)((uint64_t*)node + ramifiedAnchorBits_.addr);
    }

    inline bool canAssignAnchor() const {
        return refType() == RT_RAMIFIED_with_ANCHOR;
    }

    inline void setAnchor_unsafe(GCNode* node) {
        rtgc_assert(refType() == RT_RAMIFIED_with_ANCHOR);
        ramifiedAnchorBits_.addr = (uint64_t*)node - (uint64_t*)anchor_;
    }

    inline void setAnchor(GCNode* node) {
        rtgc_assert(refType() == RT_RAMIFIED_with_ANCHOR);
        ramifiedAnchorBits_.addr = (uint64_t*)node - (uint64_t*)anchor_;
        rtgc_assert(node == getAnchorOf(node));
    }

    inline signed_ref_count_t refCount() const {
        rtgc_assert(!isYoung());
        switch (refType()) {
            case RT_ACYCLIC:
                return acyclicBits_.ext_rc;
            case RT_RAMIFIED_FULL_RC:
                return ramifiedAnchorBits_.ext_rc;
            case RT_RAMIFIED_with_ANCHOR:
                return ramifiedFullRcBits_.ext_rc;
            case RT_TRIBUTARY_with_SHORTCUT:
                return tributaryShorcutBits_.ext_rc;
            case RT_TRIBUTARY_with_OFFSET:
                return tributaryOffsetBits_.rc;
            case RT_TRIBUTARY_FULL_RC:
                return tributaryFullRcBits_.rc;
            case RT_CIRCUIT:
                return circuitBits_.rc;
            default:
                rtgc_assert(false);
                return 0;
        }
    }

    ref_count_t refCountBitFlags() const {
        return refCount_flags_;
    }


    template <bool Atomic>
    inline ref_count_t increaseAcyclicRefCount() {
        rtgc_assert(isAcyclic());
        pal::bit_add<Atomic>(&refCount_flags_, (ref_count_t)+0x100);
        return acyclicBits_.ext_rc;
    }

    template <bool Atomic>
    inline ref_count_t decreaseAcyclicRefCount() {
        rtgc_assert(isAcyclic());
        ref_count_t delta = -0x100;
        pal::bit_add<Atomic>(&refCount_flags_, delta);
        return acyclicBits_.ext_rc;
    }


    inline unsigned getExternalRefCount() const {
        rtgc_assert(!isYoung());
        switch (refType()) {
            case RT_ACYCLIC:
                return acyclicBits_.ext_rc;
            case RT_RAMIFIED_FULL_RC:
                return ramifiedAnchorBits_.ext_rc;
            case RT_RAMIFIED_with_ANCHOR:
                return ramifiedFullRcBits_.ext_rc;
            case RT_TRIBUTARY_with_SHORTCUT:
                return tributaryShorcutBits_.ext_rc;
            case RT_TRIBUTARY_with_OFFSET:
                return tributaryOffsetBits_.ext_rc;
            case RT_TRIBUTARY_FULL_RC:
                return tributaryFullRcBits_.ext_rc;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
                return circuitBits_.rc;
            default:
                rtgc_assert("should not be here" == 0);
                return 0;
        }
    }


    inline bool tryDecreaseExternalRefCount() {
        rtgc_assert(!isYoung());
        switch (refType()) {
            case RT_ACYCLIC:
                if (acyclicBits_.ext_rc == 0) return false;
                acyclicBits_.ext_rc --;
                break;          
            case RT_RAMIFIED_FULL_RC:
                if (ramifiedAnchorBits_.ext_rc == 0) return false;
                ramifiedAnchorBits_.ext_rc --;
                break;
            case RT_RAMIFIED_with_ANCHOR:
                if (ramifiedFullRcBits_.ext_rc == 0) return false;
                ramifiedFullRcBits_.ext_rc --;
                break;
            case RT_TRIBUTARY_with_SHORTCUT:
                if (tributaryShorcutBits_.ext_rc == 0) return false;
                tributaryShorcutBits_.ext_rc --;
                break;
            case RT_TRIBUTARY_with_OFFSET:
                if (tributaryOffsetBits_.ext_rc == 0) return false;
                tributaryOffsetBits_.ext_rc --;
                break;
            case RT_TRIBUTARY_FULL_RC:
                if (tributaryFullRcBits_.ext_rc == 0) return false;
                tributaryFullRcBits_.ext_rc --;
                break;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
                return 0;
        }
        return true;
    }


    inline void saveExternalRefCount() {
        switch (refType()) {
            case RT_ACYCLIC:
            case RT_RAMIFIED_FULL_RC:
            case RT_RAMIFIED_with_ANCHOR:
                rtgc_assert("should not be here" == 0);

            case RT_TRIBUTARY_with_SHORTCUT:
                // ignore
                break;
            case RT_TRIBUTARY_with_OFFSET:
                tributaryOffsetBits_.ext_rc = tributaryOffsetBits_.rc;
                break;
            case RT_TRIBUTARY_FULL_RC:
                tributaryFullRcBits_.ext_rc = tributaryFullRcBits_.rc;
                break;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
        }        
    }

    bool increaseObjectRef() {
        int erc;
        int rc2;
        switch (refType()) {
            case RT_ACYCLIC:
                rtgc_assert("should not be here" == 0);
                break;
            case RT_RAMIFIED_with_ANCHOR:
                erc = ramifiedAnchorBits_.ext_rc;
                if (~erc != 0) {
                    ramifiedAnchorBits_.ext_rc ++;
                    break;
                }
                setRefType(RT_RAMIFIED_FULL_RC);
                ramifiedFullRcBits_.ext_rc = erc + 1;
                break;

                // no break;
            case RT_RAMIFIED_FULL_RC:
                ramifiedFullRcBits_.ext_rc ++;
                rtgc_assert(ramifiedFullRcBits_.ext_rc != 0);
                break;

            case RT_TRIBUTARY_with_SHORTCUT:
                erc = tributaryShorcutBits_.ext_rc;
                setRefType(RT_TRIBUTARY_with_OFFSET);
                tributaryOffsetBits_.ext_rc = erc;
                tributaryOffsetBits_.rc = erc + 1;
                break;
            case RT_TRIBUTARY_with_OFFSET:
                rc2 = tributaryOffsetBits_.rc;
                if (~rc2 != 0) {
                    tributaryOffsetBits_.rc ++;
                    break;
                }
                erc = tributaryOffsetBits_.ext_rc;
                setRefType(RT_TRIBUTARY_FULL_RC);
                tributaryFullRcBits_.ext_rc = erc;
                tributaryFullRcBits_.rc = rc2;
                break;
            case RT_TRIBUTARY_FULL_RC:
                tributaryFullRcBits_.rc ++;
                rtgc_assert(tributaryFullRcBits_.rc != 0);
                break;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
        }        
        return true;
    }

    bool hasUnsafeReferrer() const {
        return false;        
    }

    bool decreaseObjectRef() {
        switch (refType()) {
            case RT_ACYCLIC:
                rtgc_assert("should not be here" == 0);
                break;
            case RT_RAMIFIED_with_ANCHOR:
                ramifiedAnchorBits_.ext_rc--;
                rtgc_assert(ramifiedAnchorBits_.ext_rc >= 0);
                break;
            case RT_RAMIFIED_FULL_RC:
                ramifiedFullRcBits_.ext_rc --;
                rtgc_assert(ramifiedFullRcBits_.ext_rc >= 0);
                break;

            case RT_TRIBUTARY_with_SHORTCUT:
                tributaryShorcutBits_.ext_rc --;
                rtgc_assert(tributaryShorcutBits_.ext_rc >= 0);
                break;

            case RT_TRIBUTARY_with_OFFSET:
                if (tributaryOffsetBits_.ext_rc != 0) {
                    tributaryOffsetBits_.ext_rc --;
                } 
                tributaryOffsetBits_.rc --;
                rtgc_assert(tributaryOffsetBits_.rc >= 0);
                break;
            case RT_TRIBUTARY_FULL_RC:
                tributaryFullRcBits_.ext_rc --;
                rtgc_assert(tributaryFullRcBits_.rc >= 0);
                break;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
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