#ifndef RTGC_H
#define RTGC_H

#include "RTGC_def.h"
#include "RTGC_PAL.h"
#include "RTGC_Ref.h"

#define RTGC_NO_FRAME   0
#define RTGC_PER_FRAME  1
#define RTGC_TRIGGER    2

#define RTGC_MODE     RTGC_PER_FRAME

namespace rtgc {

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


struct GCNode {
    friend class RTCollector;

    GCNodeRef ref_;
    
public:
    inline void init(GCContext* context, bool immutable, bool acyclic) {
        int flags;
        if (immutable) {
            flags = T_IMMUTABLE | T_SHARED | RT_ACYCLIC;
            pal::markPublished(this);
            // TODO Lock context!!!
            if (GCPolicy::canSuspendGC()) {
                context = GCContext::getSharedContext();
            }
        } else {
            flags = acyclic ? RT_ACYCLIC : 0;
        }
        ref_.refCount_flags_ = flags;
        context->onCreateInstance(this);
    }

    uint8_t& byteFlags() {
        return ref_.byteFlags();
    }

    uint8_t byteFlags() const {
        return ref_.byteFlags();
    }

    inline bool isThreadLocal() const {
        return ref_.isThreadLocal();
    }

    inline bool isImmutable() const {
        return ref_.isImmutable();
    }

    inline bool isImmutableRoot() const {
        return ref_.isImmutableRoot();
    }

    inline bool isTributary() const {
        return ref_.isTributary();
    }


    inline bool isAcyclic() const {
        return ref_.isAcyclic();
    }

    inline bool isDestroyed() const {
        return ref_.isDestroyed();
    }

    inline bool isUnstable() const {
        return ref_.isUnstable();
    }

    bool isEnquedToScan() const {
        return ref_.isEnquedToScan();
    }

#define BEGIN_ATOMIC_REF()  \
    while (true) { \
        GCNodeRef oldRef = ref_;    \
        GCNodeRef newRef = oldRef;  \

#define END_ATOMIC_REF(Atomic) \
        if (pal::comp_set<Atomic>(&ref_.refCount_flags_, \
            oldRef.refCount_flags_, newRef.refCount_flags_)) break; \
    }

    inline void markImmutable(bool isRoot=false) {
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isRoot ? isThreadLocal() : !isImmutable());

        BEGIN_ATOMIC_REF()
            newRef.refCount_flags_ |= isRoot ? T_IMMUTABLE : (T_IMMUTABLE | T_SHARED);
            newRef.set_color(S_WHITE);
        END_ATOMIC_REF(false)

        rtgc_trace_ref(RTGC_TRACE_GC, this, "markImmutable");
        if (!isRoot) {
            pal::markPublished(this);
        }
    }

    inline void markShared() {
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal());
        BEGIN_ATOMIC_REF()
            newRef.refCount_flags_ |= T_SHARED;
        END_ATOMIC_REF(false)
        pal::markPublished(this);
        rtgc_trace_ref(RTGC_TRACE_GC, this, "markShared");
    }

    void markDestroyed(GCNode* prevGarbage) {
        rtgc_assert_ref(this, !this->isDestroyed());
        BEGIN_ATOMIC_REF()
            newRef.refCount_flags_ |= FLAG_DESTROYED;
            newRef.anchor_ = prevGarbage;
        END_ATOMIC_REF(false)
        rtgc_trace_ref(RTGC_TRACE_FREE, this, "markDestroyed");
    }

    bool markEnquedToScan(uint8_t additionalFlags) {        
        rtgc_assert_ref(this, !this->isDestroyed());
        // rtgc_assert_ref(this, get_color() == S_WHITE || get_color() == S_BLACK);
        //  || get_color() == S_UNSTABLE || get_color() == S_UNSTABLE_TAIL);
        const bool SUSPECTED_FLAG_IN_BYTE_FLAGS = FLAG_SUSPECTED < 0x100;
        /* SUSPECTED_FLAG_IN_BYTE_FLAGS:
           FLAG_SUSPECTED 가 s_color 와 동일 byte 범위 내에 있고,
           본 함수가 항상 shared_context 가 lock 된 상태에서 호출되는 경우 */
        if (SUSPECTED_FLAG_IN_BYTE_FLAGS || !GCPolicy::canSuspendGC() || this->isThreadLocal()) {
            if (this->isEnquedToScan()) return false;
            BEGIN_ATOMIC_REF()
                newRef.byteFlags() |= FLAG_SUSPECTED | additionalFlags;
            END_ATOMIC_REF(false)
            return true;
        } else {
            additionalFlags |= FLAG_SUSPECTED;
            auto rc = ref_.refCount_flags_;
            while ((rc & FLAG_SUSPECTED) == 0) {
                auto new_v = (rc & ~S_MASK) | additionalFlags;
                ref_count_t old_rc = pal::comp_xchg<true>(&ref_.refCount_flags_, rc, new_v);
                if (old_rc == rc) {
                    rtgc_assert_ref(this, (signed_ref_count_t)ref_.refCount_flags_ >= 0);
                    rtgc_trace_ref(RTGC_TRACE_GC, this, "markEnquedToScan shared");
                    return true;
                }
                if (old_rc & FLAG_DESTROYED) return false;
                rc = old_rc;
            } 
            return false;
        }
    }

    template <bool noGarbage=true>
    void unmarkEnquedToScan() {
        rtgc_assert_ref(this, !noGarbage || !this->isDestroyed());
        BEGIN_ATOMIC_REF()
            newRef.byteFlags() &= ~FLAG_SUSPECTED;
        END_ATOMIC_REF(false)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "unmarkEnquedToScan");
    }


    inline GCNode* getAnchor() const {
        return ref_.getAnchorOf(this);
    }

    inline void setAnchor_unsafe(GCNode* node) {
        ref_.setAnchor_unsafe(node);
    }

    inline signed_ref_count_t refCount() const {
        return ref_.refCount();
    }

    inline ref_count_t refCountBitFlags() const {
        return ref_.refCountBitFlags();
    }

    inline unsigned getExternalRefCount() const {
        return ref_.getExternalRefCount();
    }

    inline void saveExternalRefCount() {
        BEGIN_ATOMIC_REF()
            newRef.saveExternalRefCount();
        END_ATOMIC_REF(false)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "saveExternalRefCount");
    }

    inline void decreaseObjectRefCount() {
        BEGIN_ATOMIC_REF()
            if (!newRef.tryDecreaseExternalRefCount()) break;
        END_ATOMIC_REF(false)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "decreaseObjectRefCount");
    }

    template <bool Atomic>
    inline bool tryIncRootRefCount() { 
        ref_count_t survival_mask = RTGC_OBJECT_REF_MASK | RTGC_ROOT_REF_MASK;
        if (GCPolicy::LAZY_GC) survival_mask |=  FLAG_SUSPECTED;
        
        ref_count_t rc = ref_.refCount_flags_; 
        if (rc & FLAG_DESTROYED) return false;
        if ((rc & survival_mask) == 0) return false;
        if (!Atomic) {
            retainRoot<false>();
            return true;
        } 
        
        while (true) {
            ref_count_t old_rc = pal::comp_xchg<true>(&ref_.refCount_flags_, rc, rc + RTGC_ROOT_REF_INCREMENT);
            if (old_rc == rc) {
                markTouched</*Atomic=*/true, false>(rc);
                return true;
            }
            rc = old_rc;
            if (rc & FLAG_DESTROYED) return false;
            if ((rc & survival_mask) == 0) return false;
        } 
    }

    inline bool isYoung() const {
        return ref_.isYoung();
    }

    template<bool Atomic>
    RTGC_INLINE void addReferrer(GCNode* referrer) {
        rtgc_assert_ref(this, referrer != this);
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        rtgc_assert_ref(referrer, !referrer->isYoung());
        rtgc_trace_ref(RTGC_TRACE_GC, referrer, "add-as-referrer");

        
        if (this->isAcyclic()) {
            this->ref_.increaseAcyclicRefCount<Atomic>();    
            return;
        }

        while (true) {
            GCNodeRef oldMain = this->ref_;
            GCNodeRef newMain = oldMain;
            bool ext_rc_overflow = !newMain.increaseObjectRef();
            if (ext_rc_overflow) {
                markTributaryPath(oldMain.getAnchorOf(this));
                goto increase_ext_rc_only;
            } 
            else if (newMain.canAssignAnchor()) {
                newMain.setAnchor_unsafe(referrer);
                if (!pal::comp_set<true>(&ref_.refCount_flags_, oldMain.refCount_flags_, newMain.refCount_flags_)) continue;
                break;
            } 
            else {
                increase_ext_rc_only:
                if (!pal::comp_set<true>(&ref_.refCount_flags_, oldMain.refCount_flags_, newMain.refCount_flags_)) continue;
            }


            //=====================================================//
            #if 0
            add_second_anchor:
            AuxRef oldAux = _auxRef;
            if (oldAux.canAssignAnchor()) {
                // multi anchor 를 사용하면, tributary marking 횟수를 줄일 수 있다. 
                newAux newAux = oldAux;
                newAux.setAnchor(referrer);
                if (cmp_set(&_newAux, oldAux, newAux)) break;
            }


            if (!newMain.isDirty() && !referrer->_mainRef.isDirty()) {
                GCNode* oldAnchor = newMain.replaceAnchorIfNull(referrer);
                if (oldAnchor != NULL && oldAnchor->isDirty()) {
                    if (!cmp_set(&_mainRef, oldMain, newMain)) continue;
                    markTributaryPath(oldAnchor);
                    break;
                }
                newAux newAux = oldAux;
                oldAnchor = newAux.replaceAnchorIfNull(referrer);
                if (oldAnchor != NULL && oldAnchor->isDirty()) {
                    if (!cmp_set(&_auxRef, oldAux, newAux)) goto add_second_anchor;
                    markTributaryPath(oldAnchor);
                    break;
                }
            }
            #endif

            markTributaryPath(referrer);
            break;
        }
    }

    RTGC_INLINE ReachableState removeReferrer(GCNode* referrer) {
        return (this->isThreadLocal()) ? removeReferrer<false>(referrer) : removeReferrer<true>(referrer);
    }

    template<bool Atomic>
    RTGC_INLINE ReachableState removeReferrer(GCNode* referrer) {
        rtgc_assert_ref(this, referrer != this);
        rtgc_assert(referrer != NULL);
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(referrer, !referrer->isYoung());

        rtgc_trace_ref(RTGC_TRACE_REF, this, "removeReferrer");

        if (this->isAcyclic()) {
            if (!this->ref_.decreaseAcyclicRefCount<Atomic>()) {
                return ReachableState::Unreachable;
            } else {
                return ReachableState::Reachable;
            }
        }

        while (true) {
            GCNodeRef oldMain = this->ref_;
            GCNodeRef newMain = oldMain;
            bool ext_rc_overflow = !newMain.increaseObjectRef();
            if (ext_rc_overflow) {
                markTributaryPath(oldMain.getAnchorOf(this));
                goto increase_ext_rc_only;
            } 
            else if (newMain.canAssignAnchor()) {
                newMain.setAnchor_unsafe(referrer);
                if (!pal::comp_set<true>(&ref_.refCount_flags_, oldMain.refCount_flags_, newMain.refCount_flags_)) continue;
                break;
            } 
            else {
                increase_ext_rc_only:
                if (!pal::comp_set<true>(&ref_.refCount_flags_, oldMain.refCount_flags_, newMain.refCount_flags_)) continue;
            }


            //=====================================================//
            #if 0
            add_second_anchor:
            AuxRef oldAux = _auxRef;
            if (oldAux.canAssignAnchor()) {
                // multi anchor 를 사용하면, tributary marking 횟수를 줄일 수 있다. 
                newAux newAux = oldAux;
                newAux.setAnchor(referrer);
                if (cmp_set(&_newAux, oldAux, newAux)) break;
            }


            if (!newMain.isDirty() && !referrer->_mainRef.isDirty()) {
                GCNode* oldAnchor = newMain.replaceAnchorIfNull(referrer);
                if (oldAnchor != NULL && oldAnchor->isDirty()) {
                    if (!cmp_set(&_mainRef, oldMain, newMain)) continue;
                    markTributaryPath(oldAnchor);
                    break;
                }
                newAux newAux = oldAux;
                oldAnchor = newAux.replaceAnchorIfNull(referrer);
                if (oldAnchor != NULL && oldAnchor->isDirty()) {
                    if (!cmp_set(&_auxRef, oldAux, newAux)) goto add_second_anchor;
                    markTributaryPath(oldAnchor);
                    break;
                }
            }
            #endif

            markTributaryPath(referrer);
            break;
        }
        
        bool disconneted = !this->isAcyclic() && pal::comp_set<Atomic>(&ref_.anchor_, referrer, (GCNode*)nullptr);
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
        ref_count_t rc = decreaseRoot ? update_root_ref_count<Atomic>(false) : ref_.refCount_flags_;
        ref_count_t check_unstable_mask = S_MASK | RTGC_ROOT_REF_MASK | RTGC_SAFE_REF_MASK | FLAG_ACYCLIC | T_IMMUTABLE | FLAG_SUSPECTED;
        if (rc < RTGC_ROOT_REF_INCREMENT) {
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
        rtgc_assert (!this->isEnquedToScan());
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

    template<bool Atomic>
    inline void setTributary(bool isTributary) {
        BEGIN_ATOMIC_REF()
            newRef.setTributary(isTributary);
        END_ATOMIC_REF(Atomic)
    }

    static RTGC_INLINE void markTributaryPath(GCNode* node) {
        rtgc_assert(node != NULL);

        while (!node->isThreadLocal()) {
            rtgc_assert_ref(node, !node->isAcyclic());
            rtgc_assert_ref(node, !node->isDestroyed());
            if (node->isTributary()) return;
            node->setTributary<true>(true);
            if ((node = node->getAnchor()) == NULL) return;
        }

        while (true) {
            rtgc_assert_ref(node, node->isThreadLocal());
            rtgc_assert_ref(node, !node->isAcyclic());
            rtgc_assert_ref(node, !node->isDestroyed());
            if (node->isTributary()) return;
            node->setTributary<false>(true);
            if ((node = node->getAnchor()) == NULL) return;
        }
    }

    int get_color() const {
        return ref_.get_color();
    }

    bool onCircuit() {
        return ref_.onCircuit();
    }

    void set_color(uint8_t color) {
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert((color & S_MASK) == color);
        // TODO read and write is not thread-safe
        BEGIN_ATOMIC_REF()
            newRef.set_color(color);
        END_ATOMIC_REF(false)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "set_color");
    }

    inline bool marked() const {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isDestroyed());
        return ref_.marked();
    }

    inline void mark() {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isDestroyed());
        BEGIN_ATOMIC_REF()
            newRef.mark();
        END_ATOMIC_REF(false)
    }

    inline void unMark() {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isDestroyed());
        BEGIN_ATOMIC_REF()
            newRef.unMark();
        END_ATOMIC_REF(false)
    }


    static ref_count_t getExternalRefCountOfThreadLocalSubGraph(GCNode* node, NodeVector* nodes);
    
    static void unmarkEnquedToScanNode(GCNode* node, GCContext* context);

    static void replaceGlobalRef(GCRef* location, GCRef object);

    template <bool _volatile>
    static void replaceObjectRef_inline(GCRef* location, GCRef object, GCRef owner);

    static void replaceObjectRef_slow(GCRef* location, GCRef object, GCNode* owner);


    static void releaseObjectRef(GCNode* objContainer, GCNode* ownerContainer);

    static void retainObjectRef(GCNode* objContainer, GCNode* ownerContainer);

    static void updateStackRefCount(GCRef old, GCRef assigned);

    static void replaceStackRef(GCRef* location, GCRef obj);

    static int inspectUnstables(GCNode* node);


private:

    template <bool Atomic>
    inline ref_count_t decrease_object_ref_count(bool safeRefOnly) {
        signed_ref_count_t INCREMENT = RTGC_OBJECT_REF_INCREMENT;
        rtgc_assert_ref(this, !isDestroyed());
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        rtgc_assert_ref(this, (signed_ref_count_t)ref_.refCount_flags_ >= INCREMENT);
        unsigned_ref_count_t old_rc = ref_.refCount_flags_;
        unsigned_ref_count_t new_rc;
        while (true) {
            /**
             * @zee ref_count_t type 과 관계없이 value 를 int64_t 로 설정하면 오류 발생. llvm 버그???
             */
            new_rc = old_rc - INCREMENT;
            if (ref_.refBits_.ext_rc > 0) {
                new_rc -= RTGC_SAFE_REF_INCREMENT;
            }
            if (!Atomic) {
                ref_.refCount_flags_ = new_rc;
                break;
            }
            unsigned_ref_count_t res = pal::comp_xchg<true>(&ref_.refCount_flags_, old_rc, new_rc);
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
        unsigned_ref_count_t old_rc = ref_.refCount_flags_;
        unsigned_ref_count_t new_rc;
        while (true) {
            /**
             * @zee ref_count_t type 과 관계없이 value 를 int64_t 로 설정하면 오류 발생. llvm 버그???
             */
            new_rc = doIncrease ? old_rc + INCREMENT : old_rc - INCREMENT;
            if (!Atomic) {
                ref_.refCount_flags_ = new_rc;
                break;
            }
            unsigned_ref_count_t res = pal::comp_xchg<true>(&ref_.refCount_flags_, old_rc, new_rc);
            if (old_rc == res) break;
            old_rc = res;
        }
        rtgc_trace_ref(RTGC_TRACE_REF, this, "ref-count-changed");
        rtgc_assert_ref(this, (signed_ref_count_t)new_rc >= 0);
        return new_rc;
    }
    
};

inline void GCContext::onCreateInstance(GCNode* node) {
    if (1 && GCPolicy::LAZY_GC) {
        node->markEnquedToScan(0);
        _unstableNodes.push_back(node);
        // _suspectedNodes->push_back(node);
    }
}

}; // namespace RTGC

#endif // RTGC_H

// RTGC RTGC_ROOT_REF_INCREMENT   100000000
// RTGC RTGC_OBJECT_REF_INCREMENT 1000000000000