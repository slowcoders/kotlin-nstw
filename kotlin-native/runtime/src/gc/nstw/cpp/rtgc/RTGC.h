#ifndef RTGC_H
#define RTGC_H

#include "RTGC_def.h"
#include "RTGC_PAL.h"

#define RTGC_NO_FRAME   0
#define RTGC_PER_FRAME  1
#define RTGC_TRIGGER    2

#define RTGC_MODE     RTGC_PER_FRAME

namespace rtgc {

typedef uint64_t ref_count_t;
typedef int64_t  signed_ref_count_t;
typedef uint64_t unsigned_ref_count_t;

typedef pal::_Deque<GCNode*> NodeDeque;
typedef pal::_Vector<GCNode*> NodeVector;

enum ReachableState {
    Reachable,
    Unreachable,
    Unstable
};

enum SharedGCStatus {
    Suspended,
    Triggered,
    Collecting,
};

#if (RTGC_MODE != RTGC_TRIGGER)
constexpr SharedGCStatus _sharedGCStatus = SharedGCStatus::Collecting;
#else
extern volatile SharedGCStatus _sharedGCStatus;
#endif

constexpr bool ELASTIC_FRAME = (RTGC_MODE == RTGC_PER_FRAME) && true;
constexpr bool ENABLE_BK_GC = false;

struct GCContext {
    NodeDeque*  _suspectedNodes;
    NodeVector  _unstableNodes;
    NodeDeque   _nodeDeques[4];
    GCNode*     _garbageQ;
    GCNode*     _destroyedQ;

    GCFrame*    _currentFrame;
#if RTGC_DEBUG    
    GCRef*      _endOfFrame;
#endif

    NodeDeque   _exceptionStack;
    GCRef       _currentException;
    int32_t     _refStackSize;

    static volatile int _localCollectorCount;
    static volatile int _unscannedThreadCount;
    static volatile int _activeLocalCollectorCount;

    int         gcSuspendCount;
    size_t      gcThreshold;
    uint64_t    gcCollectCyclesThreshold;
    bool        _gcEnabled;
    bool        _gcInProgress;
    bool        _isLocalGcTriggered;

    void init(GCFrame* initialFrame);

    void enterFrame(GCFrame* frame);

    void leaveFrame(GCFrame* frame);

    void rewindFrame(GCFrame* frame);

    void setLocalGCStatus(bool enableGC);

    bool syncGCStatus();

    void terminateContext();

    void saveExceptionStack(GCRef exception);

    void clearExceptionStack(int count);

    void onCreateInstance(GCNode* node);
    
    void enqueGarbage(GCNode* node);

    void enqueSuspectedNode(GCNode* node);

    void clearGarbages(int collectCyclicThreshold);

    void destroyGarbages();

    void collectGarbagesInCurrentFrame(bool clearCyclicGarbage);

    void collectCyclicGarbage();

    static void enqueUnstable(GCNode* node, ReachableState state);

    static GCContext* getGCContext(GCNode* node);

    // static GCContext* getCurrentContext() {
    //     GCContext* context = pal::getCurrentThreadGCContext();
    //     if (RTGC_MODE == RTGC_TRIGGER && _sharedGCStatus == SharedGCStatus::Triggered && !context->_gcEnabled) {
    //         context->syncGCStatus();
    //     }
    //     return context;
    // }

    static void triggerGC();

    static void initRTGC();

    static void terminateRTGC();

    static GCContext* getSharedContext();

// private:
    int  deallocGarbages();
    void checkLocalGCThreashold();
    void retainCurrentFrame();

    void setCurrentFrame(GCFrame* frame);
    bool resumeFrame();
    bool checkStillSuspected(GCNode* unstable);
};

namespace GCPolicy {  

    constexpr bool LAZY_GC =     RTGC_MODE != RTGC_NO_FRAME; 

    inline bool canSuspendGC()   { return RTGC_MODE == RTGC_TRIGGER; }

    inline bool gcSuspended() {
      return canSuspendGC() && _sharedGCStatus == SharedGCStatus::Suspended;
    }

    inline void checkToResumeGC(GCContext* context) {
      if (GCPolicy::LAZY_GC) context->checkLocalGCThreashold();
    }

    template <bool Nullable>
    inline bool shouldRetainStackRef(GCRef obj) {
      if (Nullable && obj == NULL) return false;
      if (!GCPolicy::LAZY_GC) return true;
      return _sharedGCStatus != SharedGCStatus::Suspended && pal::isPublished(obj);
    }
};



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

struct RTGCRefBits {
    uint64_t color: 4;
    uint64_t type: 4;
    uint64_t node: RTGC_NODE_FLAGS_BITS - 8;
    uint64_t safe: RTGC_SAFE_REF_BITS;
    uint64_t root: RTGC_ROOT_REF_BITS;
    uint64_t obj:  RTGC_OBJECT_REF_BITS;  
};

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

#define ENABLE_OPT_TRIBUTARY_MARK    false

struct GCNode {
    friend class RTCollector;

public:
    inline void init(GCContext* context, bool immutable, bool acyclic) {
        int flags;
        if (immutable) {
            flags = T_IMMUTABLE | T_SHARED | FLAG_ACYCLIC;
            pal::markPublished(this);
            // TODO Lock context!!!
            if (GCPolicy::canSuspendGC()) {
                context = GCContext::getSharedContext();
            }
        } else {
            flags = acyclic ? FLAG_ACYCLIC : 0;
        }
        refCount_flags_ = flags;
        context->onCreateInstance(this);
    }

    uint8_t& byteFlags() {
        return *(uint8_t*)(&refCount_flags_);
    }

    uint8_t byteFlags() const {
        return *(uint8_t*)(&refCount_flags_);
    }

    inline bool isThreadLocal() const {
        return (refCount_flags_ & T_MASK) == 0;
    }

    inline bool isImmutable() const {
        return (refCount_flags_ & T_IMMUTABLE) != 0;
    }

    inline bool isImmutableRoot() const {
        return (refCount_flags_ & T_MASK) == T_IMMUTABLE;
    }

    inline bool isAcyclic() const {
        return (refCount_flags_ & (FLAG_ACYCLIC|T_IMMUTABLE)) != 0;
    }

    inline bool isDestroyed() const {
        return (refCount_flags_ & FLAG_DESTROYED) != 0;
    }

    inline bool isTributary() const {
        return (this->refCount_flags_ & FLAG_TRIBUTARY) != 0;
    }

    inline bool isUnstable() const {
        ref_count_t check_unstable_mask = RTGC_ROOT_REF_MASK | RTGC_SAFE_REF_MASK | FLAG_ACYCLIC | T_IMMUTABLE;
        return (refCount_flags_ & check_unstable_mask) == 0;
        // return get_color() == S_UNSTABLE;
    }

    // inline bool isUnstableTail() const {
    //     return get_color() == S_UNSTABLE_TAIL;
    // }

    // inline bool isUnstableOrTail() const {
    //     return get_color() >= S_UNSTABLE_TAIL;
    // }


    // inline bool isUnstableOrSuspected() const {
    //     auto color_an_suspected = this->refCount_flags_ & (S_MASK | FLAG_SUSPECTED);
    //     return color_an_suspected >= S_UNSTABLE_TAIL;
    // }

    bool isSuspected() const {
        return (refCount_flags_ & FLAG_SUSPECTED) != 0;
    }

    bool isStableAnchored() const {
        return get_color() == S_STABLE_ANCHORED;
    }

    inline void markImmutable(bool isRoot=false) {
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isRoot ? isThreadLocal() : !isImmutable());
        refCount_flags_ |= isRoot ? T_IMMUTABLE : (T_IMMUTABLE | T_SHARED);
        set_color(S_WHITE);
        rtgc_trace_ref(RTGC_TRACE_GC, this, "markImmutable");
        if (!isRoot) {
            pal::markPublished(this);
        }
    }

    inline void markShared() {
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal());
        refCount_flags_ |= T_SHARED;
        pal::markPublished(this);
        rtgc_trace_ref(RTGC_TRACE_GC, this, "markShared");
    }

    inline void markAcyclic() {
        rtgc_assert_ref(this, !isDestroyed());
        refCount_flags_ |= FLAG_ACYCLIC;
    }

    void markDestroyed(GCNode* prevGarbage) {
        rtgc_assert_ref(this, !this->isDestroyed());
        refCount_flags_ |= FLAG_DESTROYED;
        rtgc_trace_ref(RTGC_TRACE_FREE, this, "markDestroyed");
        anchor_ = prevGarbage;
    }

    bool markSuspected(uint8_t additionalFlags) {        
        rtgc_assert_ref(this, !this->isDestroyed());
        // rtgc_assert_ref(this, get_color() == S_WHITE || get_color() == S_BLACK);
        //  || get_color() == S_UNSTABLE || get_color() == S_UNSTABLE_TAIL);
        const bool SUSPECTED_FLAG_IN_BYTE_FLAGS = FLAG_SUSPECTED < 0x100;
        /* SUSPECTED_FLAG_IN_BYTE_FLAGS:
           FLAG_SUSPECTED 가 s_color 와 동일 byte 범위 내에 있고,
           본 함수가 항상 shared_context 가 lock 된 상태에서 호출되는 경우 */
        if (SUSPECTED_FLAG_IN_BYTE_FLAGS || !GCPolicy::canSuspendGC() || this->isThreadLocal()) {
            if (this->isSuspected()) return false;

            byteFlags() |= FLAG_SUSPECTED | additionalFlags;
            return true;
        } else {
            additionalFlags |= FLAG_SUSPECTED;
            auto rc = refCount_flags_;
            while ((rc & FLAG_SUSPECTED) == 0) {
                auto new_v = (rc & ~S_MASK) | additionalFlags;
                ref_count_t old_rc = pal::comp_xchg<true>(&refCount_flags_, rc, new_v);
                if (old_rc == rc) {
                    rtgc_assert_ref(this, (signed_ref_count_t)refCount_flags_ >= 0);
                    rtgc_trace_ref(RTGC_TRACE_GC, this, "markSuspected shared");
                    return true;
                }
                if (old_rc & FLAG_DESTROYED) return false;
                rc = old_rc;
            } 
            return false;
        }
    }

    template <bool noGarbage=true>
    void unmarkSuspected() {
        rtgc_assert_ref(this, !noGarbage || !this->isDestroyed());
        byteFlags() &= ~FLAG_SUSPECTED;
        rtgc_trace_ref(RTGC_TRACE_GC, this, "unmarkSuspected");
    }

    void unmarkStableAnchored() {
        if (this->isStableAnchored()) {
            set_color(0);
            rtgc_trace_ref(RTGC_TRACE_GC, this, "unmarkStableAnchored");
        }
    }
    // void markToFree() {
    //   rtgc_assert_ref(this, (this->refCount_flags_ & FLAG_TO_FREE) == 0);
    //   this->refCount_flags_ |= FLAG_TO_FREE;
    // }

    inline GCNode* getAnchor() const {
        return anchor_;
    }

    inline void setAnchor_unsafe(GCNode* node) {
        this->anchor_ = node;
    }

    inline signed_ref_count_t refCount() const {
        rtgc_assert_ref(this, !isDestroyed());
        return refCount_flags_ / RTGC_ROOT_REF_INCREMENT;
    }

    ref_count_t refCountBitFlags() const {
        return refCount_flags_;
    }

    inline ref_count_t getRootRefCount() const {
        return refBits()->root;
    }

    inline unsigned getObjectRefCount() const {
        return refBits()->obj;
    }

    inline unsigned getSafeRefCount() const {
        return refBits()->safe;
    }

    inline void setSafeRefCount(int refCount) {
        rtgc_assert_ref(this, (short)refCount >= 0);
        (reinterpret_cast<RTGCRefBits*>(&refCount_flags_))->safe = refCount;
        rtgc_trace_ref(RTGC_TRACE_GC, this, "setSafeRefCount");
    }


    inline bool hasRootRef() const {
        return refBits()->root != 0;
    }

    inline int hasObjectRef() const {
        return refBits()->obj != 0;
    }


    inline void setRefCount(signed_ref_count_t refCount) {
        rtgc_assert_ref(this, !isDestroyed());
        refCount_flags_ = (refCount_flags_ & (RTGC_ROOT_REF_INCREMENT-1)) + (refCount * RTGC_ROOT_REF_INCREMENT);
        rtgc_trace_ref(RTGC_TRACE_GC | RTGC_TRACE_REF, this, "setRefCount");
    }

    inline void setRefCountAndFlags(uint32_t refCount, uint16_t flags) {
        refCount_flags_ = (refCount * RTGC_ROOT_REF_INCREMENT) + flags;
    }

    template <bool Atomic>
    inline bool tryIncRootRefCount() { 
        ref_count_t survival_mask = RTGC_OBJECT_REF_MASK | RTGC_ROOT_REF_MASK;
        if (GCPolicy::LAZY_GC) survival_mask |=  FLAG_SUSPECTED;
        
        ref_count_t rc = refCount_flags_; 
        if (rc & FLAG_DESTROYED) return false;
        if ((rc & survival_mask) == 0) return false;
        if (!Atomic) {
            retainRoot<false>();
            return true;
        } 
        
        while (true) {
            ref_count_t old_rc = pal::comp_xchg<true>(&refCount_flags_, rc, rc + RTGC_ROOT_REF_INCREMENT);
            if (old_rc == rc) {
                markTouched</*Atomic=*/true, false>(rc);
                return true;
            }
            rc = old_rc;
            if (rc & FLAG_DESTROYED) return false;
            if ((rc & survival_mask) == 0) return false;
        } 
    }

    inline void setRootRefCount(unsigned size) {
        ((RTGCRefBits*)&refCount_flags_)->root = size;
        rtgc_trace_ref(RTGC_TRACE_GC | RTGC_TRACE_REF, this, "setRootRefCount");
    }

    inline void setObjectRefCount(unsigned size) {
        ((RTGCRefBits*)&refCount_flags_)->obj = size;
        rtgc_trace_ref(RTGC_TRACE_GC | RTGC_TRACE_REF, this, "setObjectRefCount");
    }

    inline bool isYoung() const {
        return false;// (refCount_flags_ & T_MASK) == T_SHARED;
    }

    template<bool Atomic>
    RTGC_INLINE void addReferrer(GCNode* referrer) {
        rtgc_assert_ref(this, referrer != this);
        rtgc_trace_ref(RTGC_TRACE_GC, referrer, "add-as-referrer");

        if (referrer->isYoung()) {
            return;
        }
        
        if (this->isAcyclic()) {
            this->increase_object_ref_count<Atomic>(true);    
            return;
        }

        /**
         * referrer 가 이미 suspectedNode 로 분류된 경우,
         * (즉, referrer 의 safeRefCount 가 0 이거나, 과거 잠시라도 0 이었던 경우)
         * 
         * referrer == this->_anchor 이면, 
         *    --> safeRefCount 증가. (referrer 스캔 시, Circuit 생성 여부도 확인)
         * else,
         *    1) this 가 (Circuit 검사가 필요한!) ramified node 인 경우, (즉, referrer 는 tributary)
         *       --> safeRefCount 증가. (Circuit 검사를 미룬다)
         *       참고) referrer 가 this 의 anchor-path 에서 파생된 것이고, anchor-path 가 순환경로를 형성하는 경우,
         *       markTributaryPath() 과정에서 순환경로가 검출되므로 추가적인  Circuit 검사가 필요하지 않다.
         *       즉, this 가 ramified 이고, referrer 가 tributary 라면, 그 참조는 safe 한다.
         *    2) this 가 tributary node 인 경우,
         *       --> safeRefCount 증가. (어차피 circuit scan 과정에서 다시 감소한다)
         */
        bool isSafeRef __attribute__((unused))  = referrer->isSuspected();
        if (!pal::comp_set<Atomic>(&anchor_, (GCNode*)NULL, referrer)) {
            markTributaryPath(referrer);
            isSafeRef |= !this->isTributary();
        } else {
            if (this->isTributary()) {
                // TODO: assign 전에 처리해야 한다(?)
                markTributaryPath(referrer);
            } else {
                isSafeRef |= referrer->isTributary();
                // markUnstable();
            }
        }

        // 참고) Compiler 최적화에 의해 this->getRootRefCount() == 0 일 수 있다.
        // ref_count_t rc = 
        this->increase_object_ref_count<Atomic>(false);    
        // rtgc_assert(!isStableAnchored());
        // rtgc_assert(!isUnsuspectedUnstable());
        // rtgc_assert(!isUnstableTail());
    }

    RTGC_INLINE ReachableState removeReferrer(GCNode* referrer) {
        return (this->isThreadLocal()) ? removeReferrer<false>(referrer) : removeReferrer<true>(referrer);
    }

    template<bool Atomic>
    RTGC_INLINE ReachableState removeReferrer(GCNode* referrer) {
        rtgc_assert_ref(this, referrer != this);
        rtgc_assert(referrer != NULL);
        rtgc_assert_ref(this, !isDestroyed());

        rtgc_trace_ref(RTGC_TRACE_REF, this, "removeReferrer");

        if (referrer->isYoung()) {
            return ReachableState::Reachable;
        }
        

        bool disconneted = !this->isAcyclic() && pal::comp_set<Atomic>(&anchor_, referrer, (GCNode*)nullptr);
        ref_count_t rc = this->decrease_object_ref_count<Atomic>(false);
        if (rc < RTGC_ROOT_REF_INCREMENT) {
            return ReachableState::Unreachable;
        } else if (this->isAcyclic()) {
            return ReachableState::Reachable;
        }


        if (disconneted) {
            if (ENABLE_OPT_TRIBUTARY_MARK && rc >= RTGC_OBJECT_REF_INCREMENT) {
                /**
                 * 동일 anchor 에 의한 다수의 참조 중 일부가 단절되면, 
                 * 실제 연결되어 있음에도 anchor 가 다른 Node 로 변경될 수 있는데, 이 때 단절된 경로가 tributary 로 변경되지 않은 문제가 있다.
                 * 이에, anchor_path 단절 시 obj-ref-count > 0 이면, 일단 TributaryPath 로 marking 한다.
                 * 또는, 별도의 brokenRamifiedNodes 에 추가한 후, 별도 검사 수행 후 Tributary Marking 여부를 결정한다.
                 */
                markTributaryPath(referrer);
            }
        } 

        ref_count_t check_unstable_mask = RTGC_ROOT_REF_MASK | RTGC_SAFE_REF_MASK | FLAG_ACYCLIC | T_IMMUTABLE;
        if ((rc & check_unstable_mask) == 0) {
            return ReachableState::Unstable;
        } else {
            return ReachableState::Reachable;
        }
    }  

    RTGC_INLINE void retainRoot() { 
        this->isThreadLocal() ? retainRoot<false>() : retainRoot<true>();
    }

    template <bool Atomic>
    inline void retainRoot() { 
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        ref_count_t rc = update_root_ref_count<Atomic>(true);
        markTouched<Atomic, false>(rc);
    }

    RTGC_INLINE void releaseRoot(bool decreaseRoot) { 
        if (isThreadLocal()) releaseRoot<false>(decreaseRoot); else releaseRoot<true>(decreaseRoot);
    }

    template <bool Atomic>
    RTGC_INLINE void releaseRoot(bool decreaseRoot) {
        rtgc_assert_ref(this, !isDestroyed());
        ref_count_t rc = decreaseRoot ? update_root_ref_count<Atomic>(false) : refCount_flags_;
        ref_count_t check_unstable_mask = S_MASK | RTGC_ROOT_REF_MASK | RTGC_SAFE_REF_MASK | FLAG_ACYCLIC | T_IMMUTABLE | FLAG_SUSPECTED;
        if (rc < RTGC_ROOT_REF_INCREMENT) {
            // rtgc_log("releaseRoot: %p refCount_flags_=%llx, rc=%llx, root=%llu\n", 
            //     this, (unsigned long long)refCount_flags_, (unsigned long long)rc, (unsigned long long)refBits()->root);
            GCContext::enqueUnstable(this, ReachableState::Unreachable);
        } else if ((rc & check_unstable_mask) == 0) {
            // tributary 여부와 관계없이 외부로부터의 진입 경로가 없는 순환 경로를 탐지한다.
            // 1) 외부 진입 불가 순환 경로의 생성
            // obj-ref-count=0 인 상태에서 anchor 가 설정되었고,
            // anchor->anchor 값이 null 이 아닌 경우, anchor 를 unsafe 로 설정한다.
            // 2) unsafe 해제.
            // unsafe 객체를 새로이 참조하는 경우, unsafe -> unstable 로 변경.
            // anchor->anchor 값이 null 이 아닌 경우, anchor 를 unsafe 로 설정한다.
            GCContext::enqueUnstable(this, ReachableState::Unstable);
        }
    }    

    RTGC_INLINE GCContext* resolveUnstable(GCContext* context) {
        rtgc_trace_ref(RTGC_TRACE_GC, this, "resolveUnstable");
        rtgc_assert (!this->isSuspected());
        switch (inspectUnstables(this)) {
            case 0:
                break;
            case FLAG_DESTROYED: {
                if (context == NULL) {
                    context = pal::getCurrentThreadGCContext();
                }
                for (GCNode* node = this;;) {
                    GCNode* anchor = node->getAnchor();
                    context->enqueGarbage(node);
                    if (anchor->isDestroyed()) break;
                    node = anchor;
                }
                return context;
            }
            case FLAG_SUSPECTED:
                if (context == NULL) {
                    context = pal::getCurrentThreadGCContext();
                }
                context->enqueSuspectedNode(this);
                break;
        }
        return NULL;
    }

    template<bool Atomic, bool clearUnstable>
    inline void markTouched(ref_count_t rc) {
        if (!ENABLE_BK_GC) return;
        rtgc_assert(ENABLE_BK_GC);
    }

    // inline void markUnstable() {
    //     rtgc_assert_ref(this, !isDestroyed());
    //     rtgc_assert_ref(this, GCPolicy::LAZY_GC || hasRootRef());
    //     /**
    //      * 다음 조건의 경우, obj-ref-count > 0 이더라도 root-ref-count 가 0이 될 때, garbage 검사를 수행한다.
    //      * 1) 객체 연결에 의해 anchor_path 의 시작점이 변경되었을 때. (root-circuit 중 하나는 반드시 Unstable 상태이어야 한다)
    //      * 2) 객체 연결 단절에 의해 anchor_path 가 단절되거나, anchor_path 가 없는 연결이 단절되었을 때,
    //      * TODO: state 변경 시 gc thread 와의 충돌 여부 점검.  
    //      */
    //     uint8_t color = get_color();
    //     if (color != S_UNSTABLE) { //} && (ignoreSuspected || (color & FLAG_SUSPECTED)) == 0) {
    //         set_color(S_UNSTABLE);
    //     }
    // }

    // inline void markUnstableTail() {
    //     set_color(S_UNSTABLE_TAIL);
    // }

    template<bool Atomic>
    inline void unmarkTributary() {
        pal::bit_and<Atomic>(&refCount_flags_, (ref_count_t)~FLAG_TRIBUTARY);
    }

    static RTGC_INLINE void markTributaryPath(GCNode* node) {
        rtgc_assert(node != NULL);

        while (!node->isThreadLocal()) {
            rtgc_assert_ref(node, !node->isAcyclic());
            rtgc_assert_ref(node, !node->isDestroyed());
            if (node->isTributary()) return;
            pal::bit_or<true>(&node->refCount_flags_, (ref_count_t)FLAG_TRIBUTARY);
            if ((node = node->anchor_) == NULL) return;
        }

        while (true) {
            rtgc_assert_ref(node, node->isThreadLocal());
            rtgc_assert_ref(node, !node->isAcyclic());
            rtgc_assert_ref(node, !node->isDestroyed());
            if (node->isTributary()) return;
            node->refCount_flags_ |= FLAG_TRIBUTARY;
            if ((node = node->anchor_) == NULL) return;
        }
    }

    int get_color() const {
        return *(uint8_t*)&this->refCount_flags_ & S_MASK; 
    }

    bool onCircuit() {
        uint8_t color = this->get_color();
        return color == S_RED || color == S_PINK || color == S_BROWN;
    }

    void set_color(uint8_t color) {
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert((color & S_MASK) == color);
        // TODO read and write is not thread-safe
        byteFlags() = (byteFlags() & ~S_MASK) | color; 
        rtgc_trace_ref(RTGC_TRACE_GC, this, "set_color");
    }

    inline bool marked() const {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isDestroyed());
        return (refCount_flags_ & FLAG_MARKED) != 0;
    }

    inline void mark() {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isDestroyed());
        refCount_flags_ |= FLAG_MARKED;
    }

    inline void unMark() {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isDestroyed());
        refCount_flags_ &= ~FLAG_MARKED;
    }


    static ref_count_t getExternalRefCountOfThreadLocalSubGraph(GCNode* node, NodeVector* nodes);
    
    static void unmarkSuspectedNode(GCNode* node, GCContext* context);

    static void replaceGlobalRef(GCRef* location, GCRef object);

    static void replaceObjectRef(GCRef* location, GCRef object, GCRef owner);

    static void releaseObjectRef(GCNode* objContainer, GCNode* ownerContainer);

    static void retainObjectRef(GCNode* objContainer, GCNode* ownerContainer);

    static void updateStackRefCount(GCRef old, GCRef assigned);

    static void replaceStackRef(GCRef* location, GCRef obj);

    static int inspectUnstables(GCNode* node);


private:

    template <bool Atomic>
    inline ref_count_t increase_object_ref_count(bool isSafe) {
        signed_ref_count_t INCREMENT = RTGC_OBJECT_REF_INCREMENT;
        if (isSafe) {
            INCREMENT += RTGC_SAFE_REF_INCREMENT;
        }
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        unsigned_ref_count_t old_rc = this->refCount_flags_;
        unsigned_ref_count_t new_rc;
        while (true) {
            /**
             * @zee ref_count_t type 과 관계없이 value 를 int64_t 로 설정하면 오류 발생. llvm 버그???
             */
            new_rc = old_rc + INCREMENT;
            if (!Atomic) {
                refCount_flags_ = new_rc;
                break;
            }
            unsigned_ref_count_t res = pal::comp_xchg<true>(&refCount_flags_, old_rc, new_rc);
            if (old_rc == res) break;
            old_rc = res;
        }
        rtgc_trace_ref(RTGC_TRACE_REF, this, "ref-count-changed");
        rtgc_assert_ref(this, (signed_ref_count_t)new_rc >= 0);
        rtgc_assert_ref(this, (signed_ref_count_t)new_rc >= INCREMENT);
        return new_rc;
    }

    template <bool Atomic>
    inline ref_count_t decrease_object_ref_count(bool safeRefOnly) {
        signed_ref_count_t INCREMENT = RTGC_OBJECT_REF_INCREMENT;
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        rtgc_assert_ref(this, (signed_ref_count_t)refCount_flags_ >= INCREMENT);
        unsigned_ref_count_t old_rc = this->refCount_flags_;
        unsigned_ref_count_t new_rc;
        while (true) {
            /**
             * @zee ref_count_t type 과 관계없이 value 를 int64_t 로 설정하면 오류 발생. llvm 버그???
             */
            new_rc = old_rc - INCREMENT;
            if (refBits_.safe > 0) {
                new_rc -= RTGC_SAFE_REF_INCREMENT;
            }
            if (!Atomic) {
                refCount_flags_ = new_rc;
                break;
            }
            unsigned_ref_count_t res = pal::comp_xchg<true>(&refCount_flags_, old_rc, new_rc);
            if (old_rc == res) break;
            old_rc = res;
        }
        rtgc_trace_ref(RTGC_TRACE_REF, this, "ref-count-changed");
        rtgc_assert_ref(this, (signed_ref_count_t)new_rc >= 0);
        return new_rc;
    }

    template <bool Atomic>
    inline ref_count_t update_root_ref_count(bool doIncrease) {
        signed_ref_count_t INCREMENT = RTGC_ROOT_REF_INCREMENT;
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        unsigned_ref_count_t old_rc = this->refCount_flags_;
        unsigned_ref_count_t new_rc;
        rtgc_assert_ref(this, doIncrease || (int16_t)reinterpret_cast<RTGCRefBits*>(&old_rc)->root > 0);
        while (true) {
            /**
             * @zee ref_count_t type 과 관계없이 value 를 int64_t 로 설정하면 오류 발생. llvm 버그???
             */
            new_rc = doIncrease ? old_rc + INCREMENT : old_rc - INCREMENT;
            if (!Atomic) {
                refCount_flags_ = new_rc;
                break;
            }
            unsigned_ref_count_t res = pal::comp_xchg<true>(&refCount_flags_, old_rc, new_rc);
            if (old_rc == res) break;
            old_rc = res;
        }
        rtgc_trace_ref(RTGC_TRACE_REF, this, "ref-count-changed");
        rtgc_assert_ref(this, (signed_ref_count_t)new_rc >= 0);
        rtgc_assert_ref(this, !doIncrease || (int16_t)reinterpret_cast<RTGCRefBits*>(&new_rc)->root > 0);
        return new_rc;
    }
    
    inline const RTGCRefBits* refBits() const {
        return reinterpret_cast<const RTGCRefBits*>(&refCount_flags_);
    }

protected:  
    union {
        ref_count_t refCount_flags_;
        RTGCRefBits refBits_;
    };
    GCNode* anchor_;
};

inline void GCContext::onCreateInstance(GCNode* node) {
    if (1 && GCPolicy::LAZY_GC) {
        node->markSuspected(0);
        _unstableNodes.push_back(node);
        // _suspectedNodes->push_back(node);
    }
}

}; // namespace RTGC

#endif // RTGC_H

// RTGC RTGC_ROOT_REF_INCREMENT   100000000
// RTGC RTGC_OBJECT_REF_INCREMENT 1000000000000