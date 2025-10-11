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

constexpr ref_count_t PRIMITIVE_INCREMENT = 0x1000;

enum ReachableStatus {
    Reachable,
    Unreachable,
    Unstable,
    EnquedToScan,
};

enum GCFlags {
    NT_PRIMITIVE = 0x00,

    NT_RAMIFIED_with_ANCHOR   = 0x01,
    NT_TRIBUTARY_with_SHORTCUT = 0x02,
    
    NT_TRIBUTARY_with_OFFSET  = 0x03,
    NT_TRIBUTARY_FULL_RC      = 0x04,
    NT_CIRCUIT                = 0x07,

    NT2_PRIMITIVE_YOUNG       = (0 << 3) | NT_PRIMITIVE,
    NT2_PRIMITIVE_ACYCLIC     = (1 << 3) | NT_PRIMITIVE,
    NT2_PRIMITIVE_RAMIFIED    = (2 << 3) | NT_PRIMITIVE,
    NT2_PRIMITIVE_IMMUTABLE   = (3 << 3) | NT2_PRIMITIVE_ACYCLIC,
 
    NT2_PRIMITIVE_GARBAGE     = (7 << 3) | NT_PRIMITIVE,



    REF_TYPE_MASK  = 0x07,
    REF_TYPE2_MASK = (7 << 3) | REF_TYPE_MASK,

    FLAG_ENQUED_TO_SCAN      = 0x40,
    FLAG_REMEMBERED          = 0x80,


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
    uint64_t type: 3;   \
    uint64_t color: 3;  \
    uint64_t enque: 2;  \
    uint64_t node: 8;   \


struct NtYoungOrDestroyed {
    COMMON_BITS()
    uint64_t addr:  40;  
};

struct NtPrimtive {
    COMMON_BITS()
    uint64_t idx:  16;  
    uint64_t common_rc:  32;  
};

struct NtRamified_Anchor {
    COMMON_BITS()
    uint64_t common_rc: 8;  
    int64_t addr:  40;  
};

// Long Shorcut 사용 시 반드시 Anchor 필요. --> Dest 가 Deallocation 될 수 있음.
struct NtTributary_Shorcut {
    COMMON_BITS()
    uint64_t common_rc: 8;  
    uint64_t shortcut:  40;  
};

struct NtTributary_Offset {
    COMMON_BITS()
    uint64_t ext_rc: 8;  
    uint64_t rc:  16;
    uint64_t offset:  16;  
};

struct NtTributary_Rc32 {
    COMMON_BITS()
    uint64_t ext_rc: 8;  
    uint64_t rc:  32;
};

struct NtCircuit {
    COMMON_BITS()
    uint64_t common_rc:  8;  
    uint64_t circuit_id:  32;
};

#define ENABLE_OPT_TRIBUTARY_MARK    false

struct GCNodeRef {
    friend struct GCNode;
protected:    
    union {
        ref_count_t _rcBits;
        NtCircuit _ntCircuit;
        NtPrimtive _ntPrimitive;
        NtYoungOrDestroyed _ntYoungOrDestroyed;
        NtRamified_Anchor _ntRamified_anchor;        
        NtTributary_Shorcut _ntTibutary_shorcut;
        NtTributary_Offset _ntTributary_offset;
        NtTributary_Rc32 _ntTributary_rc32;
    };
    // GCNode* anchor_;

// public:
    inline int nodeType() const {
        return _rcBits & REF_TYPE_MASK;
    }

    inline int nodeType2() volatile const {
        return _rcBits & REF_TYPE2_MASK;
    }

    inline void setNodeType(int type) {
        rtgc_assert((type & REF_TYPE_MASK) == type);
        rtgc_assert(type != NT_PRIMITIVE);
        _rcBits = (_rcBits & ~REF_TYPE_MASK) | type;
    }

    inline void setNodeType2(int type) {
        rtgc_assert((type & REF_TYPE2_MASK) == type);
        _rcBits = (_rcBits & ~REF_TYPE_MASK) | type;
    }

    inline bool isYoung() const {
        return false;// nodeType() == NT2_PRIMITIVE_YOUNG;
    }

    inline bool isTributary() const {
        return nodeType() >= NT_TRIBUTARY_with_SHORTCUT;
    }

    inline bool isThreadLocal() const {
        return false; //(_rcBits & T_MASK) == 0;
    }

    inline bool isImmutable() const {
        return false; // (_rcBits & T_IMMUTABLE) != 0;
    }

    inline bool isImmutableRoot() const {
        return false; // (_rcBits & T_MASK) == T_IMMUTABLE;
    }

    inline bool isAcyclic() const {
        return (nodeType2() & NT2_PRIMITIVE_ACYCLIC) != 0;
    }

    inline bool isPrimitiveRef() const {
        return nodeType() == NT_PRIMITIVE;
    }

    inline bool isGarbageMarked() volatile const {
        return nodeType2() == NT2_PRIMITIVE_GARBAGE;
    }

    inline void markGarbage() volatile {
        _rcBits = (_rcBits & ~REF_TYPE2_MASK) | NT2_PRIMITIVE_GARBAGE;
    }

    inline GCNode* nextGarbageOf(const GCNode* node) const {
        rtgc_assert(isGarbageMarked());
        return (GCNode*)((uint64_t*)node + _ntYoungOrDestroyed.addr);
    }

    inline void setNextGarbage(GCNode* node, GCNode* nextGarbage) volatile {
        rtgc_assert(isGarbageMarked());
        _ntYoungOrDestroyed.addr = getOffset(node, nextGarbage);
    }

    inline bool isUnstable() const {
        rtgc_assert(!isYoung());
        rtgc_assert(!isGarbageMarked() && !isEnquedToScan());
        return getExternalRefCount() == 0;
    }

    bool isRemembered() const {
        return (_rcBits & FLAG_REMEMBERED) != 0;
    }

    bool isEnquedToScan() const {
        return (_rcBits & (FLAG_ENQUED_TO_SCAN | FLAG_REMEMBERED)) != 0;
    }

    void setEnquedToScan(bool isEnqued) {
        if (isEnqued) {
            rtgc_assert(!isEnquedToScan());
            _rcBits |= FLAG_ENQUED_TO_SCAN;
        } else {
            _rcBits &= ~(FLAG_ENQUED_TO_SCAN | FLAG_REMEMBERED);
        }
    }

    static int64_t getOffset(GCNode* node, GCNode* anchor) {
        return (ref_count_t*)node - (ref_count_t*)anchor;
    }
    //============================================//

    inline void setTributary(bool isTributary, GCNode* node, GCNode* shortcut) {
        rtgc_assert(!this->isTributary());
        switch (nodeType()) {
            case NT_PRIMITIVE:
                rtgc_assert(nodeType2() == NT2_PRIMITIVE_RAMIFIED);
                setNodeType2(NT_TRIBUTARY_FULL_RC);
                _ntTributary_rc32.rc = _ntPrimitive.common_rc;
                _ntTributary_rc32.ext_rc = _ntTributary_rc32.rc;
                break;
            case NT_RAMIFIED_with_ANCHOR:
                setNodeType2(NT_TRIBUTARY_with_SHORTCUT);
                _ntTibutary_shorcut.common_rc = _ntRamified_anchor.common_rc;
                _ntTibutary_shorcut.shortcut = getOffset(node, shortcut);
                break;
            case NT_TRIBUTARY_with_SHORTCUT:
            case NT_TRIBUTARY_with_OFFSET:
            case NT_TRIBUTARY_FULL_RC:
            default:
                rtgc_assert("Should not be here!" == 0);
                break;
            case NT_CIRCUIT:
                rtgc_assert("Not impl" == 0);
        }
    }
    
    inline bool tryEraseTributraryShortcut() {
        switch (nodeType()) {
            case NT_TRIBUTARY_with_SHORTCUT:
                if (_ntTibutary_shorcut.shortcut == 0) return false;
                _ntTibutary_shorcut.shortcut = 0;
                return true;
            case NT_TRIBUTARY_with_OFFSET:
                if (_ntTributary_offset.offset == 0) return false;
                _ntTributary_offset.offset = 0;
                return true;
            case NT_TRIBUTARY_FULL_RC:
                return false;
            case NT_CIRCUIT:
                rtgc_assert("Not impl" == 0);
                return false;
            default:
                throw "Should not be here";
        }
    }


    inline GCNode* getAnchorOf(const GCNode* node) const {
        rtgc_assert(nodeType() == NT_RAMIFIED_with_ANCHOR);
        return (GCNode*)((uint64_t*)node + _ntRamified_anchor.addr);
    }

    inline int tryAssignAnchor(GCNode* node, GCNode* anchor) {
        if (nodeType() != NT_RAMIFIED_with_ANCHOR) return false;
        intptr_t offset = getOffset(node, anchor);
        if (_ntRamified_anchor.addr == 0) {
            _ntRamified_anchor.addr = offset;
            return -1;
        }
        return (_ntRamified_anchor.addr == offset);
    }

    inline bool tryEraseAnchor(GCNode* node, GCNode* anchor) {
        if (nodeType() != NT_RAMIFIED_with_ANCHOR) return false;
        if (_ntRamified_anchor.addr == getOffset(node, anchor)) {
            _ntRamified_anchor.addr = 0;
            return true;
        };
        return false;
    }


    inline void setAnchor_unsafe(GCNode* node, GCNode* anchor) {
        rtgc_assert(nodeType() == NT_RAMIFIED_with_ANCHOR);
        rtgc_assert(_ntRamified_anchor.addr == 0);
        _ntRamified_anchor.addr = getOffset(node, anchor);
    }

    inline signed_ref_count_t refCount() const {
        rtgc_assert(!isYoung());
        switch (nodeType()) {
            case NT_PRIMITIVE:
                return _ntPrimitive.common_rc;
            case NT_TRIBUTARY_with_SHORTCUT:
                return _ntTibutary_shorcut.common_rc;
            case NT_TRIBUTARY_with_OFFSET:
                return _ntTributary_offset.rc;
            case NT_TRIBUTARY_FULL_RC:
                return _ntTributary_rc32.rc;
            case NT_CIRCUIT:
                return _ntCircuit.common_rc;
            default:
                rtgc_assert(false);
                return 0;
        }
    }

    ref_count_t refCountBitFlags() const {
        return _rcBits;
    }


    inline unsigned getExternalRefCount() const {
        rtgc_assert(!isYoung());
        switch (nodeType()) {
            case NT_PRIMITIVE:
                return _ntPrimitive.common_rc;
            case NT_TRIBUTARY_with_SHORTCUT:
                return _ntTibutary_shorcut.common_rc;
            case NT_TRIBUTARY_with_OFFSET:
                return _ntTributary_offset.ext_rc;
            case NT_TRIBUTARY_FULL_RC:
                return _ntTributary_rc32.ext_rc;
            case NT_CIRCUIT:
                rtgc_assert("not impl" == 0);
                return _ntCircuit.common_rc;
            default:
                rtgc_assert("should not be here" == 0);
                return 0;
        }
    }


    inline bool tryDecreaseExternalRefCount(int min_external_rc) {
        rtgc_assert(!isYoung());
        switch (nodeType()) {
            case NT_PRIMITIVE:
                rtgc_assert("should not be here" == 0);
                break;

            case NT_TRIBUTARY_with_SHORTCUT:
                this->setNodeType(NT_TRIBUTARY_with_OFFSET);
                _ntTributary_offset.rc = _ntTibutary_shorcut.common_rc;
                _ntTributary_offset.ext_rc = _ntTributary_offset.rc - 1;
                return (_ntTributary_offset.ext_rc >= min_external_rc);
            case NT_TRIBUTARY_with_OFFSET:
                if (_ntTributary_offset.ext_rc == min_external_rc) return false;
                _ntTributary_offset.ext_rc --;
                break;
            case NT_TRIBUTARY_FULL_RC:
                if (_ntTributary_rc32.ext_rc == min_external_rc) return false;
                _ntTributary_rc32.ext_rc --;
                break;
            case NT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
                return 0;
        }
        return true;
    }


    inline void saveExternalRefCount() {
        switch (nodeType()) {
            case NT2_PRIMITIVE_ACYCLIC:
            case NT2_PRIMITIVE_RAMIFIED:
            case NT_RAMIFIED_with_ANCHOR:
                rtgc_assert("should not be here" == 0);

            case NT_TRIBUTARY_with_SHORTCUT:
                rtgc_assert("should not be here" == 0);
                break;

            case NT_TRIBUTARY_with_OFFSET:
                _ntTributary_offset.ext_rc = _ntTributary_offset.rc;
                if (_ntTributary_offset.ext_rc != _ntTributary_offset.rc) {
                    _ntTributary_offset.ext_rc = -1;
                }
                break;
            case NT_TRIBUTARY_FULL_RC:
                _ntTributary_rc32.ext_rc = _ntTributary_rc32.rc;
                if (_ntTributary_rc32.ext_rc != _ntTributary_rc32.rc) {
                    _ntTributary_rc32.ext_rc = -1;
                }
                break;
            case NT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
        }        
    }

    GCNode* increaseRef_andGetErasedAnchor(GCNode* ref, bool isRootRef) {
        int erc;
        GCNode* anchor;
        switch (nodeType()) {
            case NT2_PRIMITIVE_ACYCLIC:
                rtgc_assert("should not be here" == 0);
                break;
            case NT_RAMIFIED_with_ANCHOR:
                if (++_ntRamified_anchor.common_rc != 0) break;

                anchor = getAnchorOf(ref);
                setNodeType(NT2_PRIMITIVE_RAMIFIED);
                _ntPrimitive.common_rc = (uint)~_ntRamified_anchor.common_rc + 1;
                return anchor;

            case NT_TRIBUTARY_with_SHORTCUT:
                erc = _ntTibutary_shorcut.common_rc;
                if (isRootRef) {
                    if (++_ntTibutary_shorcut.common_rc != 0) break;
                }
                setNodeType(NT_TRIBUTARY_with_OFFSET);
                _ntTributary_offset.ext_rc = erc;
                _ntTributary_offset.rc = erc + 1;
                break;
            case NT_TRIBUTARY_with_OFFSET:
                erc = _ntTributary_offset.ext_rc;
                if (++_ntTributary_offset.rc != 0) {
                    if (isRootRef && ~erc != 0) {
                        _ntTributary_offset.ext_rc ++;
                    }
                    break;
                }
                if (isRootRef) erc ++;
                setNodeType(NT_TRIBUTARY_FULL_RC);
                _ntTributary_rc32.ext_rc = erc;
                _ntTributary_rc32.rc = (uint)~_ntTributary_offset.rc + 1;
                break;

            case NT_TRIBUTARY_FULL_RC:
                if (isRootRef && ++_ntTributary_rc32.ext_rc == 0) {
                    _ntTributary_rc32.ext_rc = -1;
                }
                _ntTributary_rc32.rc ++;
                rtgc_assert(_ntTributary_rc32.rc != 0);
                break;
            case NT_CIRCUIT:
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
        switch (nodeType()) {
            case NT_PRIMITIVE:
                rtgc_assert(isRootRef);
                rtgc_assert(_ntPrimitive.common_rc > 0);
                return --_ntPrimitive.common_rc == 0 ? Unreachable : Reachable;

            case NT_RAMIFIED_with_ANCHOR:
                rtgc_assert(_ntRamified_anchor.common_rc > 0);
                return --_ntRamified_anchor.common_rc == 0 ? Unreachable : Reachable;

            case NT_TRIBUTARY_with_SHORTCUT:
                rtgc_assert(_ntTibutary_shorcut.common_rc > 0);
                return --_ntTibutary_shorcut.common_rc == 0 ? Unreachable : Reachable;

            case NT_TRIBUTARY_with_OFFSET:
                rtgc_assert(_ntTributary_offset.rc > 0);
                if (--_ntTributary_offset.rc == 0) {
                    rtgc_assert(_ntTributary_offset.ext_rc <= 1);
                    _ntTributary_offset.ext_rc = 0;
                    return Unreachable;
                }

                if (_ntTributary_offset.ext_rc == 0) {
                    rtgc_assert(isEnquedToScan());
                    return EnquedToScan;
                }
                if (--_ntTributary_offset.ext_rc == 0) {
                    return Unstable;
                } 
                return Reachable;
                
            case NT_TRIBUTARY_FULL_RC:
                rtgc_assert(_ntTributary_rc32.rc > 0);
                if (--_ntTributary_rc32.rc == 0) {
                    rtgc_assert(_ntTributary_rc32.ext_rc <= 1);
                    _ntTributary_rc32.ext_rc = 0;
                    return Unreachable;
                }

                if (_ntTributary_rc32.ext_rc == 0) {
                    rtgc_assert(isEnquedToScan());
                    return EnquedToScan;
                }
                if (--_ntTributary_rc32.ext_rc == 0) {
                    return Unstable;
                } 
                return Reachable;

            case NT_CIRCUIT:
                rtgc_assert("not impl" == 0);
            default:
                rtgc_assert("should not be here" == 0);
                return Reachable;
        }        
    }    

    //////

    uint8_t& byteFlags() {
        return *(uint8_t*)(&_rcBits);
    }

    uint8_t byteFlags() const {
        return *(uint8_t*)(&_rcBits);
    }


    int get_color() const {
        return *(uint8_t*)&_rcBits & S_MASK; 
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
        return (_rcBits & FLAG_MARKED) != 0;
    }

    inline void mark() {
        _rcBits |= FLAG_MARKED;
    }

    inline void unMark() {
        _rcBits &= ~FLAG_MARKED;
    }

};

}; // namespace RTGC

#endif // RTGC_REF_H

// RTGC RTGC_ROOT_REF_INCREMENT   100000000
// RTGC RTGC_OBJECT_REF_INCREMENT 1000000000000