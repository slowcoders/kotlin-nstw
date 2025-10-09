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

    static void enqueUnstable(GCNode* node, bool isGarbage);

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

    volatile GCNodeRef ref_;

    const GCNodeRef getRef() const {
        GCNodeRef ref;
        ref.refCount_flags_ = pal::bit_read<true>(&ref_.refCount_flags_);
        return ref;
    }


public:

    inline void init(GCContext* context, bool immutable, bool acyclic) {
        int flags;
        if (immutable) {
            flags = RT2_PRIMITIVE_IMMUTABLE | T_SHARED | RT2_PRIMITIVE_ACYCLIC;
            pal::markPublished(this);
            // TODO Lock context!!!
            if (GCPolicy::canSuspendGC()) {
                context = GCContext::getSharedContext();
            }
        } else {
            flags = acyclic ? RT2_PRIMITIVE_ACYCLIC : 0;
        }
        ref_.refCount_flags_ = flags;
        context->onCreateInstance(this);
    }

    inline bool isThreadLocal() const {
        return getRef().isThreadLocal();
    }

    inline bool isYoung() const {
        return getRef().isYoung();
    }

    inline bool isImmutable() const {
        return getRef().isImmutable();
    }

    inline bool isImmutableRoot() const {
        return getRef().isImmutableRoot();
    }

    inline bool isTributary() const {
        return getRef().isTributary();
    }

    inline bool isAcyclic() const {
        return getRef().isAcyclic();
    }

    inline bool isPrimitiveRef() const {
        return getRef().isPrimitiveRef();
    }

    inline bool isGarbageMarked() const {
        return getRef().isGarbageMarked();
    }

    inline bool isUnstable() const {
        return getRef().isUnstable();
    }

    bool isEnquedToScan() const {
        return getRef().isEnquedToScan();
    }

#define BEGIN_ATOMIC_REF()  \
    while (true) { \
        GCNodeRef oldRef = getRef();    \
        GCNodeRef newRef = oldRef;  \

#define REF_COMP_SET(Atomic, old_, new_)  \
    pal::comp_set<Atomic>(&ref_.refCount_flags_, old_.refCount_flags_, new_.refCount_flags_)

#define END_ATOMIC_REF(Atomic) \
        if (REF_COMP_SET(Atomic, oldRef, newRef)) break; \
    }

    GCNode* getNextGarbage() const {
        return getRef().nextGarbageOf(this);
    }

    void setNextGarbage(GCNode* nextGarbage) {
        ref_.setNextGarbage(this, nextGarbage);
    }

    void markGarbage(GCNode* prevGarbage) {
        rtgc_assert_ref(this, !this->isGarbageMarked());
        ref_.markGarbage();
        ref_.setNextGarbage(this, prevGarbage);
        rtgc_trace_ref(RTGC_TRACE_FREE, this, "markGarbage");
    }

    template <bool noGarbage=true>
    void unmarkEnquedToScan() {
        rtgc_assert_ref(this, !noGarbage || !this->isGarbageMarked());
        BEGIN_ATOMIC_REF()
            newRef.setEnquedToScan(false);
        END_ATOMIC_REF(noGarbage)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "unmarkEnquedToScan");
    }


    inline GCNode* getAnchor() const {
        return getRef().getAnchorOf(this);
    }

    inline void setAnchor_unsafe(GCNode* node) {
        BEGIN_ATOMIC_REF()
            newRef.setAnchor_unsafe(node);
        END_ATOMIC_REF(true)
    }

    inline signed_ref_count_t refCount() const {
        return getRef().refCount();
    }

    inline ref_count_t refCountBitFlags() const {
        return getRef().refCountBitFlags();
    }

    inline unsigned getExternalRefCount() const {
        return getRef().getExternalRefCount();
    }

    inline void saveExternalRefCount() {
        BEGIN_ATOMIC_REF()
            newRef.saveExternalRefCount();
        END_ATOMIC_REF(true)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "saveExternalRefCount");
    }

    inline bool tryDecreaseExternalRefCount(int min_external_rc) {
        BEGIN_ATOMIC_REF()
            if (!newRef.tryDecreaseExternalRefCount(min_external_rc)) return false;
        END_ATOMIC_REF(true)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "tryDecreaseExternalRefCount");
        return true;
    }

    inline int tryAssignAnchor(GCNode* anchor) {
        int res;
        BEGIN_ATOMIC_REF()
        res = newRef.tryAssignAnchor(this, anchor);
        if (res >= 0) break;
        END_ATOMIC_REF(true);
        return res;
    }

    template <bool Atomic>
    inline bool increasePrimitiveRefCount_andCheckAcyclic() {
        rtgc_assert(!isYoung());
        rtgc_assert(isPrimitiveRef());
        ref_count_t delta = 0;
        ((RtPrimtive*)&delta)->common_rc = 1;
        ref_count_t res_rc = pal::bit_add<Atomic>(&this->ref_.refCount_flags_, +delta);
        rtgc_assert(((RtPrimtive*)&res_rc)->common_rc != 0);
        return ((GCNodeRef*)&res_rc)->isAcyclic();
    }

    template <bool Atomic>
    inline ReachableStatus decreasePrimitiveRefCount() {
        rtgc_assert(!isYoung());
        rtgc_assert(isPrimitiveRef());
        rtgc_assert(this->ref_.primitiveBits_.common_rc != 0);
        ref_count_t delta = 0;
        ((RtPrimtive*)&delta)->common_rc = 1;
        ref_count_t res_rc = pal::bit_add<Atomic>(&this->ref_.refCount_flags_, -delta);
        return res_rc < delta ? ReachableStatus::Unreachable : ReachableStatus::Reachable;
    }

    template<bool Atomic>
    RTGC_INLINE void addReferrer(GCNode* referrer) {
        retainRC<Atomic, false>(referrer);
    }

    RTGC_INLINE void retainRoot() { 
        this->isThreadLocal() ? retainRoot<false>() : retainRoot<true>();
    }

    template <bool Atomic>
    inline void retainRoot() { 
        retainRC<Atomic, true>(NULL);
        // markTouched<Atomic, false>(rc);
    }

    template<bool Atomic, bool isRootRef>
    RTGC_INLINE void retainRC(GCNode* referrer) {
        rtgc_assert_ref(this, !isGarbageMarked());
        rtgc_assert_ref(this, isThreadLocal() != Atomic);
        if (!isRootRef) {
            rtgc_assert_ref(this, referrer != this);
            rtgc_assert_ref(referrer, !referrer->isYoung());
            rtgc_trace_ref(RTGC_TRACE_GC, referrer, "add-as-referrer");
        }
        
        if (this->isPrimitiveRef()) {
            if (this->increasePrimitiveRefCount_andCheckAcyclic<Atomic>()) {
                markTributaryPath(referrer);
            }
            return;
        }

        while (true) {
            GCNodeRef oldMain = this->getRef();
            GCNodeRef newMain = oldMain;
            GCNode* erasedAnchor = newMain.increaseRef_andGetErasedAnchor(this, isRootRef);
            if (!isRootRef && erasedAnchor == NULL && newMain.tryAssignAnchor(this, referrer)) {
                if (!REF_COMP_SET(Atomic, oldMain, newMain)) continue;
                break;
            } 
            else {
                if (!REF_COMP_SET(Atomic, oldMain, newMain)) continue;
            }


            //=====================================================//
            #if 0
            add_second_anchor:
            AuxRef oldAux = _auxRef;
            newAux newAux = oldAux;
            if (newAux.tryAssignAnchor(this, referrer)) {
                // multi anchor 를 사용하면, tributary marking 횟수를 줄일 수 있다. 
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

            if (erasedAnchor != NULL) {
                /** TODO.
                erasedAnchor->eraseShotcut();
                */
                markTributaryPath(erasedAnchor);
            }
            if (!isRootRef) {
                markTributaryPath(referrer);
            }
            break;
        }
    }

    RTGC_INLINE void removeReferrer(GCNode* referrer, bool calledInGc) {
        if (this->isThreadLocal()) releaseRC<false, false>(referrer, calledInGc);
                              else releaseRC<true, false>(referrer, calledInGc);
    }

    RTGC_INLINE void releaseRoot() { 
        if (this->isThreadLocal()) this->releaseRC<false, true>(NULL, false);
                              else releaseRC<true, true>(NULL, false);
    }

    template<bool Atomic, bool isRootRef>
    RTGC_INLINE void releaseRC(GCNode* referrer, bool calledInGc) {
        rtgc_assert_ref(this, !isGarbageMarked());
        rtgc_trace_ref(RTGC_TRACE_REF, this, "releaseRC");
        if (!isRootRef) {
            rtgc_assert_ref(this, referrer != this);
            rtgc_assert(referrer != NULL);
            rtgc_assert_ref(referrer, !referrer->isYoung());
        }

        ReachableStatus rt;
        if (this->isPrimitiveRef()) {
            rt = this->decreasePrimitiveRefCount<Atomic>();
        }
        else 
        while (true) {
            GCNodeRef oldMain = this->getRef();
            GCNodeRef newMain = oldMain;
            rt = newMain.decreaseRef_andCheckReachable(false);
            bool markTributaryReferrer = !isRootRef
                && newMain.tryEraseAnchor(this, referrer)
                && rt != ReachableStatus::Unreachable;

            if (!REF_COMP_SET(true, oldMain, newMain)) 
                continue;

            if (markTributaryReferrer) {
                /**
                 * 동일 anchor 에 의한 다수의 참조 중 일부가 단절되면, 
                 * 실제 연결되어 있음에도 anchor 가 다른 Node 로 변경될 수 있는데, 이 때 단절된 경로가 tributary 로 변경되지 않는 문제가 있다.
                 * 이에, anchor_path 단절 시 obj-ref-count > 0 이면, 일단 TributaryPath 로 marking 한다.
                 * 또는, 별도의 brokenRamifiedNodes 에 추가한 후, 별도 검사 수행 후 Tributary Marking 여부를 결정한다.
                 */
                markTributaryPath(referrer);
            }
        }
        if (rt != ReachableStatus::Reachable) {
            GCContext::enqueUnstable(this, calledInGc && rt == ReachableStatus::Unreachable);
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
                    if (anchor->isGarbageMarked()) break;
                    node = anchor;
                }
                return context;
            }
            case FLAG_ENQUED_TO_SCAN:
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

    void eraseShortcut() {
        BEGIN_ATOMIC_REF()
            newRef.eraseShortcut();
        END_ATOMIC_REF(true)
    }

    static RTGC_INLINE void markTributaryPath(GCNode* node) {
        rtgc_assert(node != NULL);

        if (node->isTributary()) {
            node->eraseShortcut();
            return;
        }

        while (!node->isThreadLocal()) {
            rtgc_assert_ref(node, !node->isPrimitiveRef());
            rtgc_assert_ref(node, !node->isGarbageMarked());
            if (node->isTributary()) return;
            node->setTributary<true>(true);
            if ((node = node->getAnchor()) == NULL) return;
        }

        while (true) {
            rtgc_assert_ref(node, node->isThreadLocal());
            rtgc_assert_ref(node, !node->isPrimitiveRef());
            rtgc_assert_ref(node, !node->isGarbageMarked());
            if (node->isTributary()) return;
            node->setTributary<false>(true);
            if ((node = node->getAnchor()) == NULL) return;
        }
    }

    int get_color() const {
        return getRef().get_color();
    }

    bool onCircuit() const {
        return getRef().onCircuit();
    }

    void set_color(uint8_t color) {
        rtgc_assert_ref(this, !isGarbageMarked());
        rtgc_assert((color & S_MASK) == color);
        // TODO read and write is not thread-safe
        BEGIN_ATOMIC_REF()
            newRef.set_color(color);
        END_ATOMIC_REF(true)
        rtgc_trace_ref(RTGC_TRACE_GC, this, "set_color");
    }

    inline bool marked() const {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isGarbageMarked());
        return getRef().marked();
    }

    inline void mark() {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isGarbageMarked());
        BEGIN_ATOMIC_REF()
            newRef.mark();
        END_ATOMIC_REF(true)
    }

    inline void unMark() {
        // rtgc_assert_ref(this, isThreadLocal());
        rtgc_assert_ref(this, !isGarbageMarked());
        BEGIN_ATOMIC_REF()
            newRef.unMark();
        END_ATOMIC_REF(true)
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


};

inline void GCContext::onCreateInstance(GCNode* node) {
    if (1 && GCPolicy::LAZY_GC) {
        // node->markEnquedToScan(0);
        // _unstableNodes.push_back(node);
        // // _suspectedNodes->push_back(node);
    }
}

}; // namespace RTGC

#endif // RTGC_H

// RTGC RTGC_ROOT_REF_INCREMENT   100000000
// RTGC RTGC_OBJECT_REF_INCREMENT 1000000000000