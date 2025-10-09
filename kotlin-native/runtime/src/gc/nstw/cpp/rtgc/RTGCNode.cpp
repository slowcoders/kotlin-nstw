#include "KString.h"
#include "Porting.h"
#include "std_support/CStdlib.hpp"
#include "Natives.h"
#include "StackTrace.hpp"

#include "MemoryPrivate_rtgc.hpp"
#include "RTGC_PAL_impl.h"
#include <thread>
#include <pthread.h>

#define L1_CACHE_SIZE 128

using namespace rtgc;

volatile int rtgc::GCContext::_localCollectorCount = 0;
volatile int rtgc::GCContext::_activeLocalCollectorCount = 0;
volatile int rtgc::GCContext::_unscannedThreadCount = 0;

#if (RTGC_MODE == RTGC_TRIGGER) 
volatile SharedGCStatus rtgc::_sharedGCStatus = rtgc::SharedGCStatus::Suspended;
#endif
static volatile int _triggerLock = 0;

class Locker {
  pthread_mutex_t* lock_;

 public:
  Locker(pthread_mutex_t* alock): lock_(alock) {
    pthread_mutex_lock(lock_);
  }

  ~Locker() {
    pthread_mutex_unlock(lock_);
  }
};

class SharedGCContext : public GCContext {
    pthread_mutex_t lock_;
    pthread_mutex_t timestampLock_;
    pthread_cond_t cond_;
    pthread_t gcThread_;
    bool _terminated;

public:
    void init();

    void terminate();

private:
    void wait() {
        pthread_mutex_lock(&lock_);
        pthread_cond_wait(&cond_, &lock_);
        pthread_mutex_unlock(&lock_);
    }

    void notify() {
        pthread_cond_signal(&cond_);
    }

    static void* mainLoop(void* argument);

};

SharedGCContext g_sharedContext;

struct SpinLock {
    volatile int* _lock;
    SpinLock(volatile int* lock_) {
        lock(lock_);
        _lock = lock_;
    }

    static void lock(volatile int* lock_) {
        while (!pal::comp_set<true>(lock_, 0, 1)) {}
    }

    static void unlock(volatile int* lock_) {
        rtgc_assert(*lock_ != 0);
        *lock_ = 0;
    }

    ~SpinLock() {
        unlock(_lock);
    }
};

inline static GCNode* _toPublishedNode(GCRef obj) {
    if (obj == NULL) return NULL;
    if (GCPolicy::LAZY_GC && !pal::isPublished(obj)) {
        return NULL;
    }
    return pal::toNode<true>(obj);
}

#if RTGC_MODE == RTGC_NO_FRAME
ALWAYS_INLINE 
#else
NO_INLINE
#endif
void GCNode::updateStackRefCount(GCRef old, GCRef assigned) {
    rtgc_assert(old != assigned);

    GCNode* node; // ????????
    if ((node = _toPublishedNode(assigned)) != NULL) {
        node->retainRoot();
    }

    if ((node = _toPublishedNode(old)) != NULL) {
        node->releaseRoot();
    }
}

void GCNode::replaceStackRef(GCRef* location, GCRef newObj) {
    GCRef old = *location;
    if (old == newObj) return;

    if (GCPolicy::canSuspendGC()) {
        GCContext* context = pal::getCurrentThreadGCContext();
        if (_sharedGCStatus == SharedGCStatus::Triggered) {
            *location = newObj;
            if (context->syncGCStatus()) {
                rtgc_assert(context->_gcEnabled);
                rtgc_trace_ref(RTGC_TRACE_GC, newObj, "skip on-sync reatain replaceStackRef");
                rtgc_trace_ref(RTGC_TRACE_GC, old, "skip on-sync release replaceStackRef");
                return;
            }
        } else {
            bool synced = context->syncGCStatus();
            *location = newObj;
            if (synced) {
                rtgc_assert(!context->_gcEnabled);
                rtgc_trace_ref(RTGC_TRACE_GC, newObj, "skip no-gc-sync reatain replaceStackRef");
                rtgc_trace_ref(RTGC_TRACE_GC, old, "skip no-gc-sync release replaceStackRef");
                return;
            }
        }
    } else {
        *location = newObj;
    }

    updateStackRefCount(old, newObj);
}


void GCContext::init(GCFrame* initialFrame) {
    _currentFrame = initialFrame;
    _suspectedNodes = &_nodeDeques[0];
    _garbageQ = NULL;
    _destroyedQ = NULL;
    _currentException = NULL;
    _refStackSize = 0;
    _gcInProgress = false;
    _isLocalGcTriggered = false;
    setCurrentFrame(initialFrame);
    if (GCPolicy::canSuspendGC()) {
        SpinLock lock(&_triggerLock);
        _gcEnabled = _sharedGCStatus == SharedGCStatus::Triggered;
        _localCollectorCount ++;
        if (_gcEnabled) {
            _activeLocalCollectorCount ++;
        }
    } else {
        _gcEnabled = false;
    }
}


// const static int MAX_UNSTABLE_OVERFLOW = 32;

// void GCContext::checkLocalGCThreashold() {
//     // if (!GCPolicy::LAZY_GC) return;
//     if (_currentFrame == NULL) {
//         clearGarbages(1);
//     } else if (_gcInProgress) {
//         return;
//     } else if (!_isLocalGcTriggered) {
//         GCFrame* prevFrame = _currentFrame->previous;
//         size_t threshold = _currentFrame->rtgc_threshold;
//         if (prevFrame != NULL && prevFrame->rtgc_isRetained) {
//             threshold += 8192;
//         }
//         _isLocalGcTriggered = _unstableNodes.size() > threshold;
//     } else if (++_currentFrame->rtgc_spSize > MAX_UNSTABLE_OVERFLOW) {
//         _isLocalGcTriggered = false;
//         collectGarbagesInCurrentFrame(false);
//         _currentFrame->rtgc_spSize = MAX_UNSTABLE_OVERFLOW / 2;
//     }
// }


void GCContext::enqueUnstable(GCNode* unstable, bool isGarbage) {
    GCContext* context;
    if (GCPolicy::LAZY_GC) {
        if (unstable->isEnquedToScan()) return;
        rtgc_assert_ref(unstable, unstable->isUnstable());
        rtgc_assert_ref(unstable, !unstable->isDestroyed());
        rtgc_assert_ref(unstable, isGarbage || !unstable->isPrimitiveRef());

        const bool isSharedContext = true; // nstw. GCPolicy::canSuspendGC() && !unstable->isThreadLocal();
        context = isSharedContext ? &g_sharedContext : pal::getCurrentThreadGCContext();

        if (isSharedContext) SpinLock::lock(&_triggerLock);
        if (isGarbage && context->_gcInProgress) {
            context->enqueGarbage(unstable);
        }
        else if (unstable->markEnquedToScan(isGarbage ? 0 : S_UNSTABLE_0)) {
            context->_unstableNodes.push_back(unstable);
            // context->_suspectedNodes->push_back(unstable);
        }
        if (isSharedContext) SpinLock::unlock(&_triggerLock);

    } else {
        context = pal::getCurrentThreadGCContext();
        if (isGarbage) {
            context->enqueGarbage(unstable);
        } else {
            context->enqueSuspectedNode(unstable);            
            // context = unstable->resolveUnstable(NULL);
            // if (context == NULL) return;
        }

        if (!context->_gcInProgress) {
            // NO_FRAME mode 에서는 모든 garbage 가 즉각 처리되어야 한다.
            // 그렇지 않은 경우, garbage anchor 가 남아, markTributary() 실행 시 오류가 발생한다.
            context->clearGarbages(-1);
        }
    }
}

ALWAYS_INLINE void GCContext::enqueGarbage(GCNode* garbage) {
    garbage->markDestroyed(_garbageQ);
    _garbageQ = garbage;
}


static const bool RESURRECTABLE_FINALIZER = false;

void GCContext::collectGarbagesInCurrentFrame(bool clearCyclicGarbage) {
    if (GCPolicy::LAZY_GC) {
        if (GCPolicy::canSuspendGC()) {
            rtgc::GCContext::triggerGC();
            syncGCStatus();
        } else {
            retainCurrentFrame();
        }
    }
    clearGarbages(clearCyclicGarbage);
    resumeFrame();
}

void GCContext::destroyGarbages() {
    rtgc_assert(_gcInProgress);

    GCNode* destroyedQ = _destroyedQ;
    const bool isSharedContext = GCPolicy::canSuspendGC() && this == &g_sharedContext;
    while (true) {
        for (GCNode* garbageQ = _garbageQ; garbageQ != NULL; garbageQ = _garbageQ) {
            _garbageQ = NULL;

            GCNode* lastNode = NULL;
            if (RESURRECTABLE_FINALIZER) {
                GCNode* nextNode = NULL;
                for (GCNode* garbage = garbageQ; garbage != NULL; garbage = nextNode) {
                    nextNode = garbage->getAnchor();
                    if (pal::finalizeObject(garbage)) {
                        lastNode = garbage;
                    } else if (lastNode == NULL) {
                        garbageQ = nextNode;
                    } else {
                        lastNode->setAnchor_unsafe(nextNode);
                    }
                }
            }

            for (GCNode* garbage = garbageQ; garbage != NULL; garbage = garbage->getAnchor()) {
                if (!RESURRECTABLE_FINALIZER) {
                    lastNode = garbage;
                    if (!pal::finalizeObject(garbage)) continue;
                }
                rtgc_assert(!garbage->isImmutableRoot());
                GCRef* field;
                for (pal::RefFieldIterator iter(garbage); (field = iter.nextField()) != nullptr;) {
                    auto node = pal::toNode(*field);
                    if (RTGC_DEBUG) *field = (GCRef)0xDDDDEEEEFFFF0000L; //   nullptr;
                    if (node == nullptr || node == garbage || node->isDestroyed()) continue;

                    // rtgc_trace_ref_2(RTGC_TRACE_REF, node, garbage, "rtgc_clearObjectRefField");
                    node->removeReferrer(garbage, true);
                    /* nstw
                    if (res != ReachableStatus::Reachable) {
                        if (!GCPolicy::canSuspendGC() || !isSharedContext || node->isThreadLocal()) {
                            if (res == ReachableStatus::Unreachable) {
                                this->enqueGarbage(node);
                            } else if (node->markEnquedToScan(S_UNSTABLE_0)) {
                                rtgc_assert_ref(node, !node->isPrimitiveRef());
                                _unstableNodes.push_back(node);
                            }
                        } else {
                            SpinLock lock(&_triggerLock);
                            if (node->markEnquedToScan(res == ReachableStatus::Unreachable ? 0 : S_UNSTABLE_0)) {
                                g_sharedContext._unstableNodes.push_back(node);
                            }
                        }
                    }
                    */
                }
            }

            if (lastNode != NULL) {
                lastNode->setAnchor_unsafe(destroyedQ);
                destroyedQ = garbageQ;
            } else {
                rtgc_assert(garbageQ == NULL);
            }
        }

        if (!isSharedContext) {
            pal::processPendingRefCountUpdate(this);
        }

        if (_unstableNodes.empty()) break;
        else {
            /* TODO. SharedContextLock 을 이용해 context 를 스위칭하고, GC 중인 SharedContext 는 Lock 없이 scan 한다.*/
            if (isSharedContext) SpinLock::lock(&_triggerLock);
            for (auto node : _unstableNodes) {
                if (node == NULL) continue;
                
                node->unmarkEnquedToScan<false>();
                // no nstw.
                // if (node->hasStackRef()) continue;

                if (node->refCount() == 0) {
                    if (GCPolicy::canSuspendGC() && this != &g_sharedContext) {
                        rtgc_assert_ref(node, node->isThreadLocal());
                    }
                    if (!node->isDestroyed()) {
                        this->enqueGarbage(node);
                    }
                } else if (node->isUnstable()) {
                    rtgc_assert_ref(node, !node->isPrimitiveRef());
                    node->resolveUnstable(this);
                }
            }
            _unstableNodes.resize(0);
            if (isSharedContext) SpinLock::unlock(&_triggerLock);
        }
    }
    _destroyedQ = destroyedQ;
}

void GCContext::clearGarbages(int collectCyclicThreashold) {

    rtgc_assert(!_gcInProgress);
    // rtgc_assert(!ELASTIC_FRAME || _currentFrame == NULL || _currentFrame->previous == NULL || _currentFrame->previous->rtgc_isRetained);
    _gcInProgress = true;
    destroyGarbages();

    const bool isSharedContext = GCPolicy::canSuspendGC() && this == &g_sharedContext;
    if (!isSharedContext) {
        size_t garbageInSuspected = deallocGarbages();        
        switch (collectCyclicThreashold)
        {
        case 0:
            if (garbageInSuspected <= _suspectedNodes->size() / 8) break;
            // no-break
        case 1:
            this->collectCyclicGarbage();
        default:
            break;
        }
    }
    _gcInProgress = false;
}

void GCContext::enqueSuspectedNode(GCNode* suspected) {
    const bool isSharedContext = GCPolicy::canSuspendGC() && this == &g_sharedContext;

    rtgc_assert_ref(suspected, !suspected->isPrimitiveRef());
    rtgc_assert_ref(suspected, !suspected->isDestroyed());
    rtgc_assert_ref(suspected, isSharedContext == !suspected->isThreadLocal());

    if (isSharedContext) SpinLock::lock(&_triggerLock);
    rtgc_assert_ref(suspected, !isSharedContext || !suspected->isEnquedToScan());

    if (suspected->markEnquedToScan(S_UNSTABLE_0)) {
        rtgc_trace_ref(RTGC_TRACE_GC, suspected, "enqueSuspectedNode");
        // rtgc_log("s %p %x\n", suspected, (int)suspected->refCountBitFlags());
        rtgc_assert_ref(suspected, !suspected->isPrimitiveRef());
        _suspectedNodes->push_back(suspected);
    }
    if (isSharedContext) SpinLock::unlock(&_triggerLock);
}

volatile int toatlGarbageInSuspected = 0;
int GCContext::deallocGarbages() {
    int garbageInSuspected = 0;
    for (GCNode* garbage = _destroyedQ; garbage != NULL;) {
        GCNode* next = garbage->getAnchor();
        if (garbage->isEnquedToScan()) {
            garbageInSuspected ++;
        } else {
            pal::deallocNode(this, garbage);
        }
        garbage = next;
    }
    if (garbageInSuspected > 0) {
        toatlGarbageInSuspected += garbageInSuspected;
        rtgc_trace(RTGC_TRACE_GC, "deallocGarbages: %d/%d suspected nodes\n", 
            garbageInSuspected, toatlGarbageInSuspected);
    }
    _destroyedQ = NULL;
    return garbageInSuspected;
}

int GCNode::inspectUnstables(GCNode* unstable) {
    return FLAG_SUSPECTED;
    if (false) {
        rtgc_assert(!unstable->isDestroyed());
        rtgc_assert(!unstable->isEnquedToScan());
        rtgc_assert_ref(unstable, !unstable->isPrimitiveRef());

        if (unstable->isThreadLocal()) {
            rtgc_trace_ref(RTGC_TRACE_GC, unstable, "check cyclic");
            rtgc_assert_ref(unstable, unstable->isUnstable());
            rtgc_assert_ref(unstable, unstable->refCount() != 0);
            rtgc_assert_ref(unstable, unstable->isThreadLocal());

            int max_repeat = 8; // 순환 경로 내 무한 루프 방지 용.
            int obj_rc = unstable->refCount();
            for (GCNode* node = unstable->getAnchor(); node != NULL && --max_repeat > 0; node = node->getAnchor()) {
                if (node->isDestroyed()) {
                    return FLAG_SUSPECTED;
                }
                // no nstw.
                // if (node->refCount() == 0) {
                //     // 경로 시작점에 다다른 경우, 검색을 멈춘다.
                //     rtgc_trace_ref(RTGC_TRACE_REF, node, "not garbage");
                //     goto skip_cyclic_check;
                // }
                if (node->isEnquedToScan()) {
                    rtgc_trace_ref(RTGC_TRACE_REF, node, "sibling suspected detected");
                    goto skip_cyclic_check;
                }

                if (node == unstable) {
                    if (obj_rc == 1) {
                        return FLAG_DESTROYED;
                    }
                    GCNode::markTributaryPath(unstable);
                    break;
                }
                // if (node->get_color() == S_UNSTABLE) {
                //     rtgc_trace_ref(RTGC_TRACE_REF, node, "sibling suspected detected");
                //     goto skip_cyclic_check;
                // }

                if (false/*no nstw. node->hasStackRef()*/) {
                    // 검사를 미룬다.
                    // if (!node->isStableAnchored()) {
                    //     node->markUnstable();
                    // }
                    rtgc_trace_ref(RTGC_TRACE_REF, node, "skip unstable!");
                skip_cyclic_check:
                    rtgc_assert(!unstable->isEnquedToScan());
                    // unstable->set_color(S_UNSTABLE_TAIL);
                    return 0;
                }

                obj_rc += node->refCount() - 1;
                rtgc_assert(obj_rc >= 0);
            }
        }
        return FLAG_SUSPECTED;
    }
}

void GCNode::retainObjectRef(GCNode* objContainer, GCNode* referrer) {
    rtgc_assert(referrer != NULL);
    if (objContainer != nullptr) {
        // rtgc_trace_ref_2(RTGC_TRACE_REF, objContainer, referrer, "GCNode::retainObjectRef");
        rtgc_assert_ref(objContainer, !objContainer->isDestroyed());
        if (objContainer != referrer) {
            if (objContainer->isThreadLocal()) {
                objContainer->addReferrer<false>(referrer);
            } else {
                objContainer->addReferrer<true>(referrer);
            }
        }
    }
}

void GCNode::releaseObjectRef(GCNode* node, GCNode* referrer) {
    rtgc_assert(referrer != NULL);

    if (node != nullptr) {
        // rtgc_trace_ref_2(RTGC_TRACE_REF, node, referrer, "GCNode::releaseObjectRef");
        rtgc_assert(!node->isDestroyed());
        if (node != referrer) {
            node->removeReferrer(referrer, false);
            // if (res != 0) {
            //     GCContext::enqueUnstable(node, res);
            // }
        }
    }
}

void GCNode::replaceGlobalRef(GCRef* location, GCRef object) {
    GCRef old = *location;
    *location = object;

    if (RTGC_TRACE_GC | RTGC_TRACE_REF) {
        if (RTGC_is_debug_pointer(object) || RTGC_is_debug_pointer(old)) {
            rtgc_log("replace global ref %p -> %p\n", pal::toNode(old), pal::toNode(object));
        }
    }

    GCNode* node;
    if ((node = pal::toNode(object)) != NULL) {
        node->retainRoot();
    }
    if ((node = pal::toNode(old)) != NULL) {
        node->releaseRoot();
    }
}

void GCNode::replaceObjectRef_slow(GCRef* location, GCRef object, GCNode* referrer) {
    GCRef old = *location;
    if (old != object) {

        if (referrer == nullptr) {
            rtgc_assert(referrer != nullptr);
            // permanenent 객체이거나 stack 객체이다.
            // "-g" 옵션없이 컴파일한 경우, 로컬 객체를 Stack 내에 allocation 하고, typeInfoOrMeta_에 OBJECT_TAG_PERMANENT_CONTAINER flag 를
            // 추가한다. stack 객체의 생성자 또는 (condition ? stack-obj : heap-obj) 같은 조건문 사용으로 인해 컴파일 단계에서 stack 객체와
            // heap 객체를 명확히 구분할 수 없다. stack 객체의 경우 old 가 null 이 아닐 수 있다.
            replaceGlobalRef(location, object);
            return;
        }

        if (RTGC_TRACE_GC | RTGC_TRACE_REF) {
            if (RTGC_is_debug_pointer(object) || RTGC_is_debug_pointer(old)) {
                rtgc_log("replace-obj-ref [%p] %p -> %p\n", referrer, pal::toNode(old), pal::toNode(object));
            }
        }

        // @Frozen 속성을 가진 객체(Lock.kt 참조)는 생성자 내에서 필드 변경이 가능하다.
        rtgc_assert(referrer->isThreadLocal() || old == NULL);
        if (object != nullptr) {
            // rtgc_assert(object == owner || pal::toNode(object) != referrer);
            GCNode::retainObjectRef(pal::toNode(object), referrer);
        }
        
        // retain 후에 filed 변경. (참고. object 는 stack 임)
        old = pal::xchg<true>(location, object);

        if (old != nullptr) {
            // rtgc_assert(old == owner || pal::toNode(old) != referrer);
            GCNode::releaseObjectRef(pal::toNode(old), referrer);
        }
    }
}

template <bool _volatile>
void GCNode::replaceObjectRef_inline(GCRef* location, GCRef object, GCRef owner) {
    rtgc_assert(owner != NULL);
    auto referrer = pal::toNode<true>(owner);
    if (referrer->isYoung()) {
        pal::replaceYoungRef<_volatile>(location, object);
    } else {
        replaceObjectRef_slow(location, object, referrer);
    }
}


// void GCContext::enterFrame(GCFrame* frame) {
//     rtgc_assert(_currentException == NULL);
//     rtgc_assert(_exceptionStack.size() == 0);

//     // USE_RTGC exception 처리를 위해 frame 보관
//     pal::setPreviousFrame(frame, _currentFrame);
//     if (_gcInProgress) {
//         // 참고) kotlin-native:backend.native:tests:interop_objc_kt42172
//         // Finalizer 실행 중.
//         // do nothing.
//     } else if (ELASTIC_FRAME) {
//         rtgc_assert(_currentFrame == NULL || !_currentFrame->rtgc_isRetained);
//         frame->rtgc_threshold = _unstableNodes.size() * 2 + 2;
//         frame->rtgc_spSize = 0;
//         frame->rtgc_isRetained = 0;
//     } else if (GCPolicy::LAZY_GC && (!GCPolicy::canSuspendGC() || _gcEnabled) && _currentFrame != NULL) {
//         pal::foreachFrameRefs(_currentFrame, [](GCNode* node) {
//             if (node->isThreadLocal()) node->retainRoot();
//         });
//     }
//     setCurrentFrame(frame);
// }

// // static int cc = 0;
// void GCContext::setCurrentFrame(GCFrame* frame) {
//     // if (false && ++cc > 115) {
//         // rtgc_log("set frame (%d) %p -> %p\n", ++cc, _currentFrame, frame);
//     // }
//     _currentFrame = frame;
// #if RTGC_DEBUG
//     if (frame == NULL) {
//         _endOfFrame = NULL;
//     } else {
//         _endOfFrame = pal::getEndOfFrameSlot(frame);
//     }
// #endif
// }

// void GCContext::leaveFrame(GCFrame* frame) {
//     if (_gcInProgress) {
//         // Finalize 함수 실행 중!
//         setCurrentFrame(pal::getPreviousFrame(frame));
//         return;
//     }
//     if (!GCPolicy::LAZY_GC || !GCPolicy::canSuspendGC() || _gcEnabled) {
//         pal::foreachFrameRefs(frame, [](GCNode* node) {
//             if (!GCPolicy::LAZY_GC) {
//                 node->releaseRoot();    
//             } else if (!node->isThreadLocal()) {
//                 node->releaseRoot();
//             }
//         });
//     }
//     setCurrentFrame(pal::getPreviousFrame(frame));
//     if (!resumeFrame()) {
//         checkLocalGCThreashold();
//     }
// }

// void GCContext::retainCurrentFrame() {
//     if (_currentFrame == NULL) {
//         return;
//     }

//     for (auto frame = _currentFrame; frame != NULL && !frame->rtgc_isRetained; frame = frame->previous) {
//         frame->rtgc_isRetained = true;
//         pal::foreachFrameRefs(frame, [](GCNode* node) { 
//             if (node->isThreadLocal()) node->retainRoot<false>();
//         });
//     }
// }


// bool GCContext::resumeFrame() {
//     rtgc_assert(!_gcInProgress);
//     if (!GCPolicy::LAZY_GC || (GCPolicy::canSuspendGC() && !_gcEnabled)) return true;    

//     if (_currentFrame == NULL) {
//         this->clearGarbages(+1);
//         this->collectCyclicGarbage();
//         return true;
//     }
    
//     if (ELASTIC_FRAME) {
//         if (!_currentFrame->rtgc_isRetained) return false;
//     }

//     this->clearGarbages(0);
//     if (ELASTIC_FRAME) {
//         _currentFrame->rtgc_isRetained = false;
//     }
    
//     pal::foreachFrameRefs(_currentFrame, [](GCNode* node) {
//         if (node->isThreadLocal()) node->releaseRoot<false>(true);
//     });
//     return true;
// }

// void GCContext::saveExceptionStack(GCRef exception) {
//     pal::toNode(exception)->retainRoot();
//     _currentException = exception;

//     FrameOverlay* frame = _currentFrame;
//     if (GCPolicy::canSuspendGC() && !this->_gcEnabled) {
//         for (; frame != NULL; frame = frame->previous) {
//             frame->rtgc_spSize = 0;
//         }
//         return;
//     }

//     {
//         frame->rtgc_spSize = 0;
//         pal::foreachFrameRefs(frame, [this](GCNode* node) {
//             if (!GCPolicy::LAZY_GC || !node->isThreadLocal()) _exceptionStack.push_back(node);
//         });
//     }

//     while ((frame = frame->previous) != NULL) {
//         frame->rtgc_spSize = _exceptionStack.size();
//         const bool saveAllRef = frame->rtgc_isRetained;
//         pal::foreachFrameRefs(frame, [saveAllRef, this](GCNode* node) {
//             if (!GCPolicy::LAZY_GC || saveAllRef || !node->isThreadLocal()) _exceptionStack.push_back(node);
//         });
//     }
// }   

// void GCContext::rewindFrame(GCFrame* frame) {
//     rtgc_assert(_currentException != NULL);
//     size_t count = frame == NULL ? _exceptionStack.size() : (size_t)frame->rtgc_spSize;
//     rtgc_assert(_exceptionStack.size() >= (size_t)count);
//     for (auto iter = _exceptionStack.begin(); count-- > 0; iter++) {  
//         (*iter)->releaseRoot();
//     }
//     _exceptionStack.resize(0);
//     bool frameChanged = _currentFrame != frame;
//     if (frameChanged) setCurrentFrame(frame);
//     pal::toNode(_currentException)->releaseRoot();
//     // if (frameChanged) {
//         resumeFrame();
//     // } else if (ELASTIC_FRAME) {
//     //     _currentFrame->rtgc_isRetained = false;
//     // }

//     _currentException = NULL;
// }



void SharedGCContext::init() {
    GCContext::init(NULL);
    _localCollectorCount --;
    _terminated = false;
    if (pthread_mutex_init(&lock_, nullptr) != 0 ||
        pthread_mutex_init(&timestampLock_, nullptr) != 0 || 
        pthread_cond_init(&cond_, nullptr) || 
        pthread_create(&gcThread_, nullptr, mainLoop, NULL)
    ) {
        throw "Cannot init rtgc scanner";
    }
}

void SharedGCContext::terminate() {
    Locker lock(&lock_);
    _terminated = true;
    notify();
}


void* SharedGCContext::mainLoop(void* argument) {
    while (true) {
        g_sharedContext.wait();
    }
}


void GCContext::terminateRTGC() {
    g_sharedContext.terminate();
}

void GCContext::initRTGC() {
    // rtgc_log("RTGC RTGC_ROOT_REF_INCREMENT %llx\n", RTGC_ROOT_REF_INCREMENT);
    // rtgc_log("RTGC RTGC_OBJECT_REF_INCREMENT %llx\n", RTGC_OBJECT_REF_INCREMENT);
    g_sharedContext.init();
}

ALWAYS_INLINE GCContext* GCContext::getGCContext(GCNode* node) {
    if (GCPolicy::canSuspendGC() && !node->isThreadLocal()) {
        return &g_sharedContext;
    } else {
        return pal::getCurrentThreadGCContext();
    }
}

// main loop of background garbage collection thread
void GCContext::triggerGC() {    
#if RTGC_MODE == RTGC_TRIGGER    
    {
        SpinLock lock(&_triggerLock);
        _sharedGCStatus = SharedGCStatus::Triggered;
    }
#endif
}


bool GCContext::syncGCStatus() {
#if RTGC_MODE != RTGC_TRIGGER
    return false;
#else
    switch (_sharedGCStatus) {
    case SharedGCStatus::Suspended:
        rtgc_assert(!_gcEnabled);
        return true;
    case SharedGCStatus::Triggered:
        if (_gcEnabled) return false;
        break;
    case SharedGCStatus::Collecting:
        if (!_gcEnabled) return true;
        break;
    }

    const bool gc = _sharedGCStatus == SharedGCStatus::Triggered;
    setLocalGCStatus(gc);

    return true;
#endif
}

void GCContext::setLocalGCStatus(bool enableGC) {
#if RTGC_MODE == RTGC_TRIGGER    
    if (_gcEnabled == enableGC) return;

    FrameOverlay* frame = _currentFrame;
    pal::foreachFrameRefs(frame, [enableGC](GCNode* node) { 
        if (!node->isThreadLocal()) {
            if (enableGC) node->retainRoot(); else node->releaseRoot(); 
        }
    });

    while ((frame = frame->previous) != NULL) {
        pal::foreachFrameRefs(frame, [enableGC](GCNode* node) { 
            if (enableGC) node->retainRoot(); else node->releaseRoot(); 
        });
    }

    _gcEnabled = enableGC;

    bool doCollectPublished = false;
    bool gcFinished = false;
    {
        SpinLock lock(&_triggerLock);
        if (enableGC) {
            if (++_activeLocalCollectorCount == _localCollectorCount) {
                if (_sharedGCStatus == SharedGCStatus::Triggered) {
                    doCollectPublished = true;
                }
            };
        } else {
            gcFinished = --_activeLocalCollectorCount == 0;
            rtgc_assert(_activeLocalCollectorCount >= 0);
        }
    }
    if (doCollectPublished) {
        g_sharedContext.clearGarbages(0);
        _sharedGCStatus = SharedGCStatus::Collecting;
    }
    if (gcFinished) {
        g_sharedContext.deallocGarbages();
        {
            SpinLock lock(&_triggerLock);
            _sharedGCStatus = SharedGCStatus::Suspended;
        }        
    }

#endif
}

void GCContext::terminateContext() {
    rtgc_assert(_unstableNodes.empty());
    rtgc_assert(_suspectedNodes->empty());
    rtgc_assert(_garbageQ == NULL);

#if RTGC_MODE == RTGC_TRIGGER    
    bool gcFinished = false;
    {
        SpinLock lock(&_triggerLock);
        _localCollectorCount --;
        rtgc_assert(_localCollectorCount >= 0);
        gcFinished = _gcEnabled && --_activeLocalCollectorCount == 0;
    }
    if (gcFinished) {
        g_sharedContext.deallocGarbages();
        {
            SpinLock lock(&_triggerLock);
            _sharedGCStatus = SharedGCStatus::Suspended;
        }        
    }
#endif
}

ALWAYS_INLINE GCContext* GCContext::getSharedContext() {
    return &g_sharedContext;
}