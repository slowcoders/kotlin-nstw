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

enum ReachableStatus {
    Reachable,
    Unreachable,
    Unstable,
    EnquedToScan,
};

enum GCFlags {
    RT_PRIMITIVE = 0x00,

    RT_RAMIFIED_with_ANCHOR   = 0x01,
    RT_TRIBUTARY_with_SHORTCUT = 0x02,
    
    RT_TRIBUTARY_with_OFFSET  = 0x03,
    RT_TRIBUTARY_FULL_RC      = 0x04,
    RT_CIRCUIT                = 0x07,

    RT2_PRIMITIVE_YOUNG       = 0x00 | RT_PRIMITIVE,
    RT2_PRIMITIVE_ACYCLIC     = 0x10 | RT_PRIMITIVE,
    RT2_PRIMITIVE_RAMIFIED    = 0x20 | RT_PRIMITIVE,
    RT2_PRIMITIVE_IMMUTABLE   = 0x30 | RT2_PRIMITIVE_ACYCLIC,


    REF_TYPE_MASK  = 0x07,
    REF_TYPE2_MASK = 0x37,

    FLAG_ENQUED_TO_SCAN      = 0x10,


    T_IMMUTABLE     = 0x0400,
    T_SHARED        = 0x0800,
    T_MASK          = T_IMMUTABLE | T_SHARED,

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

    S_UNSTABLE_0 = 0,
    S_MASK              = 0x0F,

};


#define COMMON_BITS()   \
    uint64_t color: 4;  \
    uint64_t type: 4;   \
    uint64_t node: 8;   \


struct RtPrimtive {
    COMMON_BITS()
    uint64_t idx:  16;  
    uint64_t common_rc:  32;  
};

struct RtRamified_Anchor {
    COMMON_BITS()
    uint64_t common_rc: 8;  
    int64_t addr:  40;  
};

// Long Shorcut 사용 시 반드시 Anchor 필요. --> Dest 가 Deallocation 될 수 있음.
struct RtTributary_Shorcut {
    COMMON_BITS()
    uint64_t common_rc: 8;  
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
    uint64_t common_rc:  8;  
    uint64_t circuit_id:  32;
};

#define ENABLE_OPT_TRIBUTARY_MARK    false

struct GCNodeRef {
    friend struct GCNode;
protected:    
    union {
        ref_count_t refCount_flags_;
        RtCircuit circuitBits_;
        RtPrimtive primitiveBits_;
        RtRamified_Anchor ramifiedAnchorBits_;        
        RtTributary_Shorcut tributaryShorcutBits_;
        RtTributary_Offset tributaryOffsetBits_;
        RtTributary_FullRc tributaryFullRcBits_;
    };
    GCNode* anchor_;

    inline int refType() const {
        return refCount_flags_ & REF_TYPE_MASK;
    }

    inline int refType2() const {
        return refCount_flags_ & REF_TYPE2_MASK;
    }

    inline void setRefType(int type) {
        refCount_flags_ = (refCount_flags_ & ~REF_TYPE_MASK) | type;
    }

    inline bool isYoung() const {
        return false;// refType() == RT2_PRIMITIVE_YOUNG;
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
        return (refType2() & RT2_PRIMITIVE_ACYCLIC) != 0;
    }

    inline bool isPrimitiveRef() const {
        return refType() == RT_PRIMITIVE;
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
        return (refCount_flags_ & FLAG_ENQUED_TO_SCAN) != 0;
    }

    void setEnquedToScan(bool isEnqued) {
        if (isEnqued) {
            refCount_flags_ |= FLAG_ENQUED_TO_SCAN;
        } else {
            refCount_flags_ &= ~FLAG_ENQUED_TO_SCAN;
        }
    }
    //============================================//

    inline void setTributary(bool isTributary) {
        // if (isTributary) {
        //     refCount_flags_ |= FLAG_TRIBUTARY;
        // } else {
        //     refCount_flags_ &= ~FLAG_TRIBUTARY;
        // }
    }
    
    inline void eraseShortcut() {
        rtgc_assert(isTributary());
        tributaryShorcutBits_.shortcut = 0;
    }


    inline GCNode* getAnchorOf(const GCNode* node) const {
        rtgc_assert(refType() == RT_RAMIFIED_with_ANCHOR);
        return (GCNode*)((uint64_t*)node + ramifiedAnchorBits_.addr);
    }

    inline int tryAssignAnchor(GCNode* node, GCNode* anchor) {
        if (refType() != RT_RAMIFIED_with_ANCHOR) return false;
        intptr_t offset = (uint64_t*)node - (uint64_t*)anchor;
        if (ramifiedAnchorBits_.addr == 0) {
            ramifiedAnchorBits_.addr = offset;
            return -1;
        }
        return (ramifiedAnchorBits_.addr == offset);
    }

    inline bool tryEraseAnchor(GCNode* node, GCNode* anchor) {
        if (refType() != RT_RAMIFIED_with_ANCHOR) return false;
        if (ramifiedAnchorBits_.addr == (uint64_t*)node - (uint64_t*)anchor) {
            ramifiedAnchorBits_.addr = 0;
            return true;
        };
        return false;
    }


    inline void setAnchor_unsafe(GCNode* node) {
        rtgc_assert(refType() == RT_RAMIFIED_with_ANCHOR);
        rtgc_assert(ramifiedAnchorBits_.addr == 0);
        ramifiedAnchorBits_.addr = (uint64_t*)node - (uint64_t*)anchor_;
    }

    inline signed_ref_count_t refCount() const {
        rtgc_assert(!isYoung());
        switch (refType()) {
            case RT_PRIMITIVE:
                return primitiveBits_.common_rc;
            case RT_TRIBUTARY_with_SHORTCUT:
                return tributaryShorcutBits_.common_rc;
            case RT_TRIBUTARY_with_OFFSET:
                return tributaryOffsetBits_.rc;
            case RT_TRIBUTARY_FULL_RC:
                return tributaryFullRcBits_.rc;
            case RT_CIRCUIT:
                return circuitBits_.common_rc;
            default:
                rtgc_assert(false);
                return 0;
        }
    }

    ref_count_t refCountBitFlags() const {
        return refCount_flags_;
    }


    inline unsigned getExternalRefCount() const {
        rtgc_assert(!isYoung());
        switch (refType()) {
            case RT_PRIMITIVE:
                return primitiveBits_.common_rc;
            case RT_TRIBUTARY_with_SHORTCUT:
                return tributaryShorcutBits_.common_rc;
            case RT_TRIBUTARY_with_OFFSET:
                return tributaryOffsetBits_.ext_rc;
            case RT_TRIBUTARY_FULL_RC:
                return tributaryFullRcBits_.ext_rc;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
                return circuitBits_.common_rc;
            default:
                rtgc_assert("should not be here" == 0);
                return 0;
        }
    }


    inline bool tryDecreaseExternalRefCount(int min_external_rc) {
        rtgc_assert(!isYoung());
        switch (refType()) {
            case RT_PRIMITIVE:
                rtgc_assert("should not be here" == 0);
                break;

            case RT_TRIBUTARY_with_SHORTCUT:
                this->setRefType(RT_TRIBUTARY_with_OFFSET);
                tributaryOffsetBits_.rc = tributaryShorcutBits_.common_rc;
                tributaryOffsetBits_.ext_rc = tributaryOffsetBits_.rc - 1;
                return (tributaryOffsetBits_.ext_rc >= min_external_rc);
            case RT_TRIBUTARY_with_OFFSET:
                if (tributaryOffsetBits_.ext_rc == min_external_rc) return false;
                tributaryOffsetBits_.ext_rc --;
                break;
            case RT_TRIBUTARY_FULL_RC:
                if (tributaryFullRcBits_.ext_rc == min_external_rc) return false;
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
            case RT2_PRIMITIVE_ACYCLIC:
            case RT2_PRIMITIVE_RAMIFIED:
            case RT_RAMIFIED_with_ANCHOR:
                rtgc_assert("should not be here" == 0);

            case RT_TRIBUTARY_with_SHORTCUT:
                rtgc_assert("should not be here" == 0);
                break;

            case RT_TRIBUTARY_with_OFFSET:
                tributaryOffsetBits_.ext_rc = tributaryOffsetBits_.rc;
                if (tributaryOffsetBits_.ext_rc != tributaryOffsetBits_.rc) {
                    tributaryOffsetBits_.ext_rc = -1;
                }
                break;
            case RT_TRIBUTARY_FULL_RC:
                tributaryFullRcBits_.ext_rc = tributaryFullRcBits_.rc;
                if (tributaryFullRcBits_.ext_rc != tributaryFullRcBits_.rc) {
                    tributaryFullRcBits_.ext_rc = -1;
                }
                break;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
        }        
    }

    GCNode* increaseRef_andGetErasedAnchor(GCNode* ref, bool isRootRef) {
        int erc;
        GCNode* anchor;
        switch (refType()) {
            case RT2_PRIMITIVE_ACYCLIC:
                rtgc_assert("should not be here" == 0);
                break;
            case RT_RAMIFIED_with_ANCHOR:
                if (++ramifiedAnchorBits_.common_rc != 0) break;

                anchor = getAnchorOf(ref);
                setRefType(RT2_PRIMITIVE_RAMIFIED);
                primitiveBits_.common_rc = (uint)~ramifiedAnchorBits_.common_rc + 1;
                return anchor;

            case RT_TRIBUTARY_with_SHORTCUT:
                erc = tributaryShorcutBits_.common_rc;
                if (isRootRef) {
                    if (++tributaryShorcutBits_.common_rc != 0) break;
                }
                setRefType(RT_TRIBUTARY_with_OFFSET);
                tributaryOffsetBits_.ext_rc = erc;
                tributaryOffsetBits_.rc = erc + 1;
                break;
            case RT_TRIBUTARY_with_OFFSET:
                erc = tributaryOffsetBits_.ext_rc;
                if (++tributaryOffsetBits_.rc != 0) {
                    if (isRootRef && ~erc != 0) {
                        tributaryOffsetBits_.ext_rc ++;
                    }
                    break;
                }
                if (isRootRef) erc ++;
                setRefType(RT_TRIBUTARY_FULL_RC);
                tributaryFullRcBits_.ext_rc = erc;
                tributaryFullRcBits_.rc = (uint)~tributaryOffsetBits_.rc + 1;
                break;

            case RT_TRIBUTARY_FULL_RC:
                if (isRootRef && ++tributaryFullRcBits_.ext_rc == 0) {
                    tributaryFullRcBits_.ext_rc = -1;
                }
                tributaryFullRcBits_.rc ++;
                rtgc_assert(tributaryFullRcBits_.rc != 0);
                break;
            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
        }        
        return NULL;
    }

    bool hasUnsafeReferrer() const {
        return false;        
    }

    ReachableStatus decreaseRef_andCheckReachable(bool isRootRef) {
        switch (refType()) {
            case RT_PRIMITIVE:
                rtgc_assert(isRootRef);
                rtgc_assert(primitiveBits_.common_rc > 0);
                return --primitiveBits_.common_rc == 0 ? Unreachable : Reachable;

            case RT_RAMIFIED_with_ANCHOR:
                rtgc_assert(ramifiedAnchorBits_.common_rc > 0);
                return --ramifiedAnchorBits_.common_rc == 0 ? Unreachable : Reachable;

            case RT_TRIBUTARY_with_SHORTCUT:
                rtgc_assert(tributaryShorcutBits_.common_rc > 0);
                return --tributaryShorcutBits_.common_rc == 0 ? Unreachable : Reachable;

            case RT_TRIBUTARY_with_OFFSET:
                rtgc_assert(tributaryOffsetBits_.rc > 0);
                if (--tributaryOffsetBits_.rc == 0) {
                    rtgc_assert(tributaryOffsetBits_.ext_rc <= 1);
                    tributaryOffsetBits_.ext_rc = 0;
                    return Unreachable;
                }

                if (tributaryOffsetBits_.ext_rc == 0) {
                    rtgc_assert(isEnquedToScan());
                    return EnquedToScan;
                }
                if (--tributaryOffsetBits_.ext_rc == 0) {
                    return Unstable;
                } 
                return Reachable;
                
            case RT_TRIBUTARY_FULL_RC:
                rtgc_assert(tributaryFullRcBits_.rc > 0);
                if (--tributaryFullRcBits_.rc == 0) {
                    rtgc_assert(tributaryFullRcBits_.ext_rc <= 1);
                    tributaryFullRcBits_.ext_rc = 0;
                    return Unreachable;
                }

                if (tributaryFullRcBits_.ext_rc == 0) {
                    rtgc_assert(isEnquedToScan());
                    return EnquedToScan;
                }
                if (--tributaryFullRcBits_.ext_rc == 0) {
                    return Unstable;
                } 
                return Reachable;

            case RT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
                return Reachable;
        }        
    }    

    //////

    uint8_t& byteFlags() {
        return *(uint8_t*)(&refCount_flags_);
    }

    uint8_t byteFlags() const {
        return *(uint8_t*)(&refCount_flags_);
    }


    int get_color() const {
        return *(uint8_t*)&refCount_flags_ & S_MASK; 
    }

    bool onCircuit() const {
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