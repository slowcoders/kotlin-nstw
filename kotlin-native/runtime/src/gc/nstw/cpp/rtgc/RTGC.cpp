#include "KString.h"
#include "Porting.h"
#include "std_support/CStdlib.hpp"
#include "Natives.h"
#include "StackTrace.hpp"

#include "MemoryPrivate_rtgc.hpp"
#include "RTGC_PAL_impl.h"
#include "GCUtils.hpp"

#include <thread>
#include <pthread.h>

#define L1_CACHE_SIZE 128

using namespace rtgc;
volatile int RTGC_CRC_2 = 1;



struct VisitFrame {
    VisitFrame() {}
    VisitFrame(GCNode* node) {
        _node = node;
        _isTributary = false;
    }
    GCNode* _node;
    uint16_t _count;
    bool _isTributary;
    int _rc;
};

struct Bridge {
    GCNode* _foreignNode;
    GCNode* _localNode;

    Bridge(GCNode* foreignNode, GCNode* localNode) {
        this->_foreignNode = foreignNode;
        this->_localNode = localNode;
    }
    bool isCircuitBridge() {
        return _foreignNode != NULL && _foreignNode->onCircuit();
    }
};

class RTCollector {
    NodeVector _circuitNodes;
    rtgc::mem::HugeArray<VisitFrame>  _toVisit;
    rtgc::mem::HugeArray<Bridge>  _bridges;
    int _external_rc;
    int _rootRefCount;
    size_t _cntCyclicGarbage;

public:
    NodeDeque _blackNodes;

    RTCollector() { _cntCyclicGarbage = 0; _bridges.initialize(); _toVisit.initialize(); }
    
    inline static bool isRedOrPink(GCNode* node) { return node->get_color() <= S_RED; }

    inline static bool isBrown(GCNode* node) { return node->get_color() == S_BROWN; }

    inline static bool isGray(GCNode* node) { return node->get_color() == S_GRAY; }

    inline static void markGray(GCNode* node) { 
        node->set_color(S_GRAY); 
        rtgc_trace_ref(RTGC_TRACE_GC, node, "markGray");
    }

    inline static void markBrown(GCNode* node) { 
        rtgc_assert(isRedOrPink(node));
        node->set_color(S_BROWN); 
        rtgc_trace_ref(RTGC_TRACE_GC, node, "markBrown");
    }

    template <bool _clearMark>
    inline void finishScan(GCNode* node, bool isTributary) { 
        if (node->getObjectRefCount() > 1) {
            if (node->get_color() >= S_GRAY) {
                if (!((node->get_color() == S_GRAY || node->get_color() == S_STABLE_ANCHORED))) {
                    rtgc_assert(node->get_color() == S_GRAY || node->get_color() == S_STABLE_ANCHORED);
                }
                node->set_color(isTributary ? S_TRIBUTARY_BLACK : S_BLACK); 
            } else {
                node->set_color(S_CYCLIC_BLACK); 
            }
            rtgc_trace_ref(RTGC_TRACE_GC, node, "markBlack");
            _blackNodes.push_back(node);
        } else if (_clearMark) {
            clearMark(node);
        } else {
            rtgc_assert(node->get_color() == S_STABLE_ANCHORED);
        }
    }

    inline void enqueCircuitNode(GCNode* node) {
        rtgc_trace_ref(RTGC_TRACE_GC, node, "enqueCircuitNode");
        rtgc_assert_ref(node, node->isThreadLocal());
        rtgc_assert_ref(node, !node->isDestroyed());
        rtgc_assert_ref(node, node->get_color() == S_GRAY);
        node->set_color(S_RED); 

        _external_rc += node->getObjectRefCount() - 1;
        _circuitNodes.push_back(node);
    }

    inline static void clearMark(GCNode* node) {
        // rtgc_assert(node->get_color() != S_UNSTABLE);
        if (node->get_color() >= S_GRAY) {
            rtgc_assert(node->get_color() == S_GRAY || node->get_color() == S_STABLE_ANCHORED);
            node->set_color(S_WHITE); //S_STABLE_ANCHORED); 
        } else {
            node->set_color(S_WHITE); 
        }
        rtgc_trace_ref(RTGC_TRACE_GC, node, "clearMark");
    }

    bool markCyclicPath(GCNode* start, GCNode* end);

    void scanSuspected(GCNode* suspected);

    void destroyCyclicGarbages();

    static GCNode* findRedOrPink(GCNode* node) {
        rtgc_assert(isBrown(node));
        for (; !isRedOrPink(node = node->getAnchor()); ) {
            rtgc_assert_ref(node, isBrown(node));
        }
        return node;
    }    

    ref_count_t getExternalRefCountOfThreadLocalSubGraph(GCNode* node);

    void destroyGarbages() {
        for (GCNode* node : _blackNodes) {
            if (node->isDestroyed()) {
                continue;
            }
            rtgc_assert(!node->isDestroyed());
            uint8_t color;
            switch (node->get_color()) {
            case S_BLACK:           color = S_WHITE; break;
            case S_TRIBUTARY_BLACK: color = S_WHITE; break;
            case S_CYCLIC_BLACK:    color = S_WHITE; break;
            default: color = S_WHITE; // rtgc_assert(false);
            }
            node->set_color(color);
        }
        destroyCyclicGarbages();        
    }
};



bool RTCollector::markCyclicPath(GCNode* anchor, GCNode* end) {
    _external_rc --;
    for (GCNode* node = anchor;; node = node->getAnchor()) {
        if (!isRedOrPink(node)) {
            node->setSafeRefCount(node->getSafeRefCount() - 1);
            enqueCircuitNode(node);
        }
        if (node == end) break;
    }
    
    if (_external_rc <= 0) {
        rtgc_assert(_external_rc >= 0);
    }
    rtgc_trace(RTGC_TRACE_GC, "_external_rc %d\n", _external_rc);
    if (_external_rc == 0) {
        rtgc_trace(RTGC_TRACE_GC, "_external_rc2 %d\n", _external_rc);
    }
    return _external_rc > 0;
}

void RTCollector::destroyCyclicGarbages() {
    GCContext* context = rtgc::pal::getCurrentThreadGCContext();
    for (GCNode* node : _circuitNodes) {
        context->enqueGarbage(node);
    }
    NodeVector unstableQ;
    context->destroyGarbages();
    _circuitNodes.clear();
}

static const bool MARK_PINK_ON_SMALL_CIRCUIT_START = false;


bool GCContext::checkStillSuspected(GCNode* unstable) {
    rtgc_assert(!unstable->isDestroyed());
    rtgc_assert_ref(unstable, !unstable->isAcyclic());

    rtgc_trace_ref(RTGC_TRACE_GC, unstable, "check cyclic");
    // rtgc_assert_ref(unstable, !unstable->hasRootRef());
    // rtgc_assert_ref(unstable, unstable->hasObjectRef());
    rtgc_assert_ref(unstable, unstable->isThreadLocal());

    int max_repeat = 32; // 순환 경로 내 무한 루프 방지 용.
    int obj_rc = unstable->getObjectRefCount();
    for (GCNode* node = unstable->getAnchor(); node != NULL && --max_repeat > 0; node = node->getAnchor()) {
        if (node->isDestroyed()) {
            // rtgc_assert_ref(node, !node->isDestroyed());
            return true;
        }
        if (!node->hasObjectRef()) {
            // 경로 시작점에 다다른 경우, 검색을 멈춘다.
            if (node->refCount() == 0) {
                rtgc_assert_ref(node, node->isSuspected());
                // return false;
            } 
            rtgc_trace_ref(RTGC_TRACE_REF, node, "not garbage");
            return false;
        }
        if (node->isSuspected()) {
            rtgc_trace_ref(RTGC_TRACE_REF, node, "sibling suspected detected");
            return false;
        }

        if (node == unstable) {
            if (obj_rc == 1) {
                for (GCNode* node = unstable;;) {
                    GCNode* anchor = node->getAnchor();
                    this->enqueGarbage(node);
                    if (anchor->isDestroyed()) break;
                    node = anchor;
                }
                return false;
            }
            GCNode::markTributaryPath(unstable);
            return true;
        }
        // if (node->get_color() == S_UNSTABLE) {
        //     rtgc_trace_ref(RTGC_TRACE_REF, node, "sibling suspected detected");
        //     goto skip_cyclic_check;
        // }

        if (node->hasRootRef()) {
            // 검사를 미룬다.
            // if (!node->isStableAnchored()) {
            //     node->markUnstable();
            // }
            rtgc_trace_ref(RTGC_TRACE_REF, node, "skip unstable!");
        // skip_cyclic_check:
            // rtgc_assert(!unstable->isSuspected());
            // unstable->set_color(S_UNSTABLE_TAIL);
            return false;
        }

        obj_rc += node->getObjectRefCount() - 1;
        rtgc_assert(obj_rc >= 0);
    }
    return true;
}

void RTCollector::scanSuspected(GCNode* suspected) {
    rtgc_trace_ref(RTGC_TRACE_GC, suspected, "\nscanSuspected");

    const bool scanTributaryOnly = suspected->isTributary();

    this->_external_rc = 1;
    this->_toVisit.resize(0);
    rtgc_assert(_cntCyclicGarbage == _circuitNodes.size());
    rtgc_assert_ref(suspected, suspected->isSuspected());     
    // Unstable = Not scanned yet.
    // rtgc_assert_ref(suspected, suspected->get_color() == S_UNSTABLE);

    _toVisit.push_back(suspected);
    _toVisit.back()._isTributary = true;
    int cnt;
    while (!_toVisit.empty()) {
        VisitFrame& frame = _toVisit.back();
        auto* anchor = frame._node;
        rtgc_assert(!anchor->isDestroyed());
        int mark = anchor->get_color();
        if (mark != S_WHITE && mark < S_STABLE_ANCHORED) {
            _toVisit.pop_back();
            if (mark == S_RED || (MARK_PINK_ON_SMALL_CIRCUIT_START && mark == S_PINK)) {
                markBrown(anchor);
                if (isGray(anchor->getAnchor())) {
                    rtgc_trace_ref(RTGC_TRACE_GC, anchor, "begin clear reds");
                    int toPopup = _circuitNodes.size() - frame._count; 
                    for (auto iter = _circuitNodes.rbegin(); --toPopup >= 0; iter ++) {
                        GCNode* node = *iter;
                        rtgc_trace_ref(RTGC_TRACE_GC, node, "clear reds");
                        finishScan<true>(node, false);
                    }
                    if (MARK_PINK_ON_SMALL_CIRCUIT_START && mark == S_PINK) {
                        GCNode* cyclic_anchor = _circuitNodes[frame._count];
                        if (RTGC_DEBUG) {
                            GCNode* node;
                            for (rtgc::pal::ChildNodeIterator iter(cyclic_anchor); (node = iter.nextNode()) != NULL; ) {
                                if (node == anchor) break;
                            }
                            rtgc_assert(node != NULL);
                        }
                        anchor->setAnchor_unsafe(cyclic_anchor);
                    }
                    _circuitNodes.resize(frame._count);
                    _external_rc = frame._rc;
                }
            } else if (mark == S_BROWN) {
                rtgc_trace_ref(RTGC_TRACE_GC, anchor, "pop brown");
                if (--_external_rc == 0) {
                    rtgc_trace(RTGC_TRACE_GC, "clear _circuitNodes with brown");
                    goto scan_finshed;
                }
            } else if (mark != S_BLACK && mark != S_TRIBUTARY_BLACK && mark != S_CYCLIC_BLACK) {
                // anchor 가 toVisited 에 다수 포함될 수 있다.
                // 이 때, anchor 의 obj_ref_cnt > 1 이므로, WHITE 로 변경되지 않는다.
                finishScan<true>(anchor, frame._isTributary);
                if (!frame._isTributary) {
                    // konan::consoleErrorf("====== unmark tributary %p\n", anchor);
                    if (0) anchor->setTributary<false>(false);
                }
            }
            continue;
        }
        markGray(anchor);
        frame._rc = _external_rc;
        frame._count = _circuitNodes.size();
        anchor->setSafeRefCount(anchor->getObjectRefCount());

        GCNode* node;
        bool isTributray = false;
        for (rtgc::pal::ChildNodeIterator iter(anchor); (node = iter.nextNode()) != NULL; ) {
            if (node == anchor) continue;

            rtgc_assert_ref(node, !node->isDestroyed());

            if (node->isAcyclic()) {
                rtgc_trace_ref(RTGC_TRACE_GC, node, "skip acyclic");
                continue;
            }

            rtgc_assert(scanTributaryOnly || !node->isTributary());
            rtgc_assert_ref(node, anchor->isTributary() || !node->isTributary());
            if (scanTributaryOnly && !node->isTributary()) {
                rtgc_trace_ref(RTGC_TRACE_GC, node, "skip primary");
                isTributray = true;
                continue;
            }

            if (node->hasRootRef()) {
                rtgc_trace_ref(RTGC_TRACE_GC, node, "skip rootReachble");
                // Suspected 상태라면, collectCyclicGarbage 에서 _suspectedNodes 에서 제거한 후 Unstable 로 변경한다.
                // node->markUnstable();
                _external_rc += node->getRootRefCount();
                // continue;
            }
            // tributary anchor 는 자유롭게 변경할 수 있다(????)
            int mark = node->get_color();                
            switch (mark) {
                // case S_UNSTABLE:
                // case S_UNSTABLE_TAIL:
                // case S_STABLE_ANCHORED:
                //     node->set_color(S_WHITE);//STABLE_ANCHORED);
                    // no-break;
                case S_WHITE:
                    if (!scanTributaryOnly) {
                        rtgc_assert_ref(node, node->getAnchor() == anchor);
                    } else if (RTGC_CRC_2 && node->getObjectRefCount() < 16
                            && node->getAnchor() != NULL && node->getAnchor() != anchor) {         
                        this->_bridges.push_back(Bridge(node, anchor));
                        cnt = _toVisit.size();
                        isTributray = true;
                        break;
                    } else {         
                        node->setAnchor_unsafe(anchor);                
                    }
                    rtgc_trace_ref(RTGC_TRACE_GC, node, "push white");
                    _toVisit.push_back(node);
                    break;
                
                case S_GRAY: {
                    /* 중간에 circuit 을 포함할 수 있다.*/
                    bool isComplexCircuit = _circuitNodes.size() > 0;
                    rtgc_trace_ref(RTGC_TRACE_GC, node, isComplexCircuit ? "multi-cycle found" : "gray found");
                    // node->setSafeRefCount(node->getSafeRefCount() - 1);
                    if (!markCyclicPath(anchor, node)) goto scan_finshed;
                    if (node == suspected) {
                        node->setAnchor_unsafe(anchor);
                    } else if (MARK_PINK_ON_SMALL_CIRCUIT_START && !isComplexCircuit && node->getObjectRefCount() < 5) {
                        // rtgc_log("pink\n");
                        rtgc_assert(node->getAnchor()->get_color() == S_GRAY || node->getAnchor()->get_color() == S_PINK);
                        node->set_color(S_PINK);
                    }
                    break;
                }
                case S_BROWN: 
                    rtgc_trace_ref(RTGC_TRACE_GC, node, "found brown");
                    node->setSafeRefCount(node->getSafeRefCount() - 1);
                    node = findRedOrPink(node);
                    if (!markCyclicPath(anchor, node)) goto scan_finshed;
                    break;

                case S_PINK:
                case S_RED:
                    rtgc_trace_ref(RTGC_TRACE_GC, node, "found red");
                    node->setSafeRefCount(node->getSafeRefCount() - 1);
                    if (!markCyclicPath(anchor, node)) goto scan_finshed;
                    break;

                // case S_GRAY_T:
                // case S_BROWN_T:
                // case S_RED_T:
                //     rtgc_assert_ref(node, rtgc::ENABLE_BK_GC);
                    break;

                case S_TRIBUTARY_BLACK:
                    if (RTGC_CRC_2 && node->getAnchor() != NULL && node->getAnchor() != anchor) {         
                        // UNFINISHED_BLACK 으로 처리.
                        this->_bridges.push_back(Bridge(node, anchor));
                    }
                case S_BLACK:
                case S_CYCLIC_BLACK:
                    if (node->getSafeRefCount() > 0) {
                        if (node->getAnchor() == NULL || node->getSafeRefCount() > 1) {
                            node->setSafeRefCount(node->getSafeRefCount() - 1);
                        }
                    }
                    rtgc_trace_ref(RTGC_TRACE_GC, node, "skip trace");
                    isTributray = true;
                    break;

                default:
                    rtgc_trace_ref(1, node, "invalid state");
                    rtgc_assert(false);
            }
            if (isTributray) {
                for (auto iter = &_toVisit.end(); --cnt >= 0; iter --) {
                    VisitFrame& frame = *iter;
                    if (frame._isTributary) break;
                    frame._isTributary = true;
                }
            }
        };            
    }

    scan_finshed:
    if (_external_rc != 0 && !_bridges.empty()) {
        // konan::consoleErrorf("====== bridge nodes: %d\n", (int)_bridges.size());
        for (bool rescan = true; rescan; ) {
            rescan = false;
            cnt = _bridges.size();
            for (auto iter = &_bridges.front(); --cnt >= 0; iter ++) {
                Bridge& bridge = *iter;
                if (bridge.isCircuitBridge()) {
                    bridge._foreignNode = NULL;
                    for (GCNode* n = bridge._localNode; !n->onCircuit(); n = n->getAnchor()) {
                        n->set_color(S_CYCLIC_BLACK);
                        _external_rc += n->getObjectRefCount() - 1;
                    }
                    _external_rc -= 1;
                    rtgc_assert(_external_rc >= 0);
                    // konan::consoleErrorf("====== red bridge found\n");
                    rescan = true;
                }
            }
        }

        cnt = _bridges.size();
        for (auto iter = &_bridges.front(); --cnt >= 0; iter ++) {
            Bridge& bridge = *iter;
            GCNode* node = bridge._foreignNode;
            if (node != NULL) {
                node->set_color(S_BLACK);
                _blackNodes.push_back(node);
            }
        }

        if (_external_rc == 0) {
            // konan::consoleErrorf("====== garbage bridge found\n");
        }
    }
    this->_bridges.clear();

    if (_external_rc== 0) {
        rtgc_trace(RTGC_TRACE_GC, "found garbage circuit\n");
        _cntCyclicGarbage = _circuitNodes.size();
        // cnt = _toVisit.size();
        // for (auto iter = _toVisit.adr_at(0); --cnt >= 0; iter++) {
        //     uint8_t color = iter->_node->get_color();
        //     rtgc_assert(color != S_WHITE);
        //     if (color == S_STABLE_ANCHORED) {
        //         finishScan<false>(iter->_node, false);
        //     }
        // }
        if (RTGC_DEBUG) {
            // for (auto iter = _circuitNodes.begin() + _cntCyclicGarbage; iter < _circuitNodes.end(); iter++) {
            //     uint8_t color = (*iter)->get_color();
            //     rtgc_assert_ref(*iter, color == S_RED || color == S_BROWN);            
            //     (*iter)->set_color(color - (S_GRAY - S_GRAY_T));
            // }
        }
    } else {
        rtgc_trace(RTGC_TRACE_GC, "not found garbage\n");
        for (auto iter = _circuitNodes.begin() + _cntCyclicGarbage; iter < _circuitNodes.end(); iter++) {
            finishScan<true>(*iter, false);
        }
        _circuitNodes.resize(_cntCyclicGarbage);

        cnt = _toVisit.size();
        for (auto iter = _toVisit.adr_at(0) ; --cnt >= 0; iter++) {
            finishScan<true>(iter->_node, false);
        }
    }
}

extern volatile int toatlGarbageInSuspected;

// 각 쓰레드 별로 호출되어야 한다. freeQ 먼저 처리 후에!
void GCContext::collectCyclicGarbage() {
    if (false) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms);
    }
    rtgc_trace(RTGC_TRACE_GC, "rtgc::collectCyclicGarbage\n");
    NodeDeque* suspectedQ;
    while (!(suspectedQ = _suspectedNodes)->empty()) {
        _suspectedNodes = &_nodeDeques[suspectedQ == &_nodeDeques[0] ? 1 : 0];
        RTCollector collector;
        while (!suspectedQ->empty()) {    
            GCNode* node = suspectedQ->back();
            suspectedQ->pop_back();
            if (node == NULL) continue;
            rtgc_assert_ref(node, node->isSuspected());

            rtgc_trace_ref(RTGC_TRACE_GC, node, "collectCyclicGarbage");
            if (node->isDestroyed()) {
                if (RTGC_DEBUG) {
                    node->unmarkSuspected<false>();
                }
                toatlGarbageInSuspected --;
        rtgc_trace(RTGC_TRACE_GC, "totalGarbageInSuspected: %d suspected nodes\n", 
            toatlGarbageInSuspected);
                // rtgc_log("del suspected %p\n", node);
                rtgc::pal::deallocNode(this, node);
                continue;
            } 
            
            if (node->refCount() == 0) {
                node->unmarkSuspected();
                enqueGarbage(node);
                this->destroyGarbages();
                continue;
            }

            if (!node->isUnstable()) {
                node->unmarkSuspected();
                continue;
            }

            if (node->hasRootRef()) {
                // node->markUnstable();
            } else if (!node->isImmutable() && !node->isAcyclic() && node->isUnstable()) {
                if (true || this->checkStillSuspected(node)) {
                    // rtgc_assert_ref(node, !node->isAcyclic());
                    collector.scanSuspected(node);
                }
                // if (!node->isDestroyed()) {
                //     GCNode* n = node->getAnchor();
                //     if (n != NULL && n->get_color() == S_WHITE) {
                //         for (; n != NULL; n = n->getAnchor()) {
                //             if (n->isSuspected()) break;
                //             if (n->getObjectRefCount() > 1) {
                //                 if (n->getSafeRefCount() == 0) {
                //                     n->setSafeRefCount(1);
                //                 }
                //                 n->set_color(S_BLACK);
                //                 collector._blackNodes.push_back(n);
                //             }
                //         }
                //     }
                // }
            }
            node->unmarkSuspected();
        }
        collector.destroyGarbages();
        rtgc_assert(suspectedQ->empty());
    }
}



#if 0    
static void spinLock(volatile int* lock) {
    while (!rtgc::pal::comp_set<true>(lock, 0, 1)) {
        // do loop;
    }
}

static void releaseSpin(volatile int* lock) {
    *lock = 0;
}
#endif

void GCNode::unmarkSuspectedNode(GCNode* node, rtgc::GCContext* context) {
    for (auto it = context->_suspectedNodes->begin(); it != context->_suspectedNodes->end(); ++it) {
        // rtgc_assert((*it)->isThreadLocal());
        if (*it == node) {
            node->unmarkSuspected();
            // node->markUnstable();
            *it = NULL;
            break;
        }
    }
    for (auto it = context->_unstableNodes.begin(); it != context->_unstableNodes.end(); ++it) {
        // rtgc_assert((*it)->isThreadLocal());
        if (*it == node) {
            node->unmarkSuspected();
            // node->markUnstable();
            *it = NULL;
            break;
        }
    }
}

ref_count_t GCNode::getExternalRefCountOfThreadLocalSubGraph(GCNode* root, NodeVector* nodes) {
    rtgc_assert(root->isThreadLocal());
    signed_ref_count_t rc = root->refCount();
    
    NodeVector _toVisit;
    _toVisit.push_back(root);
    while (!_toVisit.empty()) {
        auto* anchor = _toVisit.back();
        rtgc_assert(anchor->isThreadLocal());
        _toVisit.pop_back();
        rtgc_assert(!anchor->isDestroyed());

        if (anchor->marked()) {
            rc -= (RTGC_OBJECT_REF_INCREMENT >> (RTGC_NODE_FLAGS_BITS + RTGC_SAFE_REF_BITS));
            continue;
        }
        anchor->mark();
        nodes->push_back(anchor);

        GCNode* node;
        for (rtgc::pal::ChildNodeIterator iter(anchor); (node = iter.nextNode()) != NULL; ) {
            rtgc_assert_ref(node, !node->isDestroyed());
            if (node->isThreadLocal()) {
                if (!node->marked()) {
                    rc += node->refCount();
                    _toVisit.push_back(node);
                }
                rc -= (RTGC_OBJECT_REF_INCREMENT >> (RTGC_NODE_FLAGS_BITS + RTGC_SAFE_REF_BITS));    
                rtgc_assert(rc >= 0);
            } else {
                rtgc_assert(node->isImmutable());
            }
        };            
    }

    for (auto it = nodes->begin(); it != nodes->end(); ++it) {
        (*it)->unMark();
    }
    rtgc_assert(rc >= (signed_ref_count_t)root->getRootRefCount());
    return rc;
}


/**
 TO DO
 1) _unreachableNodes 와 _unstableNodes 분리.
 2) markUnstable() 최적화.
 2) freezeSubGraph, clearSubGraph 실행 시 GC SKIP
 3) markSupected frozen root if any sub-node were suspected-state.
 * 
 */
