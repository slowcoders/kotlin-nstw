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

    inline void finishScan(GCNode* node, bool isTributary) { 
        if (node->refCount() > 1) {
            if (node->get_color() >= S_GRAY) {
                node->set_color(isTributary ? S_TRIBUTARY_BLACK : S_BLACK); 
            } else {
                node->set_color(S_CYCLIC_BLACK); 
            }
            rtgc_trace_ref(RTGC_TRACE_GC, node, "markBlack");
            _blackNodes.push_back(node);
        } else {
            clearMark(node);
        }
    }

    inline void enqueCircuitNode(GCNode* node) {
        rtgc_trace_ref(RTGC_TRACE_GC, node, "enqueCircuitNode");
        rtgc_assert_ref(node, node->isThreadLocal());
        rtgc_assert_ref(node, !node->isGarbageMarked());
        rtgc_assert_ref(node, node->get_color() == S_GRAY);
        node->set_color(S_RED); 

        _external_rc += node->refCount() - 1;
        _circuitNodes.push_back(node);
    }

    inline static void clearMark(GCNode* node) {
        node->set_color(S_WHITE); 
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
            if (node->isGarbageMarked()) {
                continue;
            }
            rtgc_assert(!node->isGarbageMarked());
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
            node->tryDecreaseExternalRefCount(0);
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



void RTCollector::scanSuspected(GCNode* suspected) {
    rtgc_trace_ref(RTGC_TRACE_GC, suspected, "\nscanSuspected");

    rtgc_assert(suspected->isTributary());

    __int128 i128 = 0;
    i128 += 1;
    rtgc_assert_f(true, "__i128 %d\n", (int)i128);
    
    this->_external_rc = 1;
    this->_toVisit.resize(0);
    rtgc_assert(_cntCyclicGarbage == _circuitNodes.size());
    rtgc_assert_ref(suspected, suspected->isEnquedToScan());     
    // Unstable = Not scanned yet.
    // rtgc_assert_ref(suspected, suspected->get_color() == S_UNSTABLE);

    _toVisit.push_back(suspected);
    _toVisit.back()._isTributary = true;
    int cnt;
    while (!_toVisit.empty()) {
        VisitFrame& frame = _toVisit.back();
        auto* anchor = frame._node;
        rtgc_assert(!anchor->isGarbageMarked());
        int mark = anchor->get_color();
        if (mark != S_WHITE) {
            _toVisit.pop_back();
            if (mark == S_RED || (MARK_PINK_ON_SMALL_CIRCUIT_START && mark == S_PINK)) {
                markBrown(anchor);
                if (isGray(anchor->getAnchor())) {
                    rtgc_trace_ref(RTGC_TRACE_GC, anchor, "begin clear reds");
                    int toPopup = _circuitNodes.size() - frame._count; 
                    for (auto iter = _circuitNodes.rbegin(); --toPopup >= 0; iter ++) {
                        GCNode* node = *iter;
                        rtgc_trace_ref(RTGC_TRACE_GC, node, "clear reds");
                        finishScan(node, false);
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
                finishScan(anchor, frame._isTributary);
                // if (!frame._isTributary) {
                //     // konan::consoleErrorf("====== unmark tributary %p\n", anchor);
                //     if (0) anchor->setTributary<false>(false);
                // }
            }
            continue;
        }
        markGray(anchor);
        frame._rc = _external_rc;
        frame._count = _circuitNodes.size();
        anchor->saveExternalRefCount();

        GCNode* node;
        bool isTributray = false;
        for (rtgc::pal::ChildNodeIterator iter(anchor); (node = iter.nextNode()) != NULL; ) {
            if (node == anchor) continue;

            rtgc_assert_ref(node, !node->isGarbageMarked());

            if (!node->isTributary()) {
                rtgc_trace_ref(RTGC_TRACE_GC, node, "skip primitive");
                isTributray = true;
                continue;
            }

            // tributary anchor 는 자유롭게 변경할 수 있다(????)
            int mark = node->get_color();                
            switch (mark) {
                case S_WHITE:
                    if (!node->tryAssignAnchor(anchor)) {
                        isTributray = true;
                        cnt = _toVisit.size();
                        if (!node->tryDecreaseExternalRefCount(1)) {
                            rtgc_trace_ref(RTGC_TRACE_GC, node, "push white");
                            _toVisit.push_back(node);
                        }
                    }
                    break;
                
                case S_GRAY: {
                    /* 중간에 circuit 을 포함할 수 있다.*/
                    bool isComplexCircuit = _circuitNodes.size() > 0;
                    rtgc_trace_ref(RTGC_TRACE_GC, node, isComplexCircuit ? "multi-cycle found" : "gray found");
                    // node->tryDecreaseExternalRefCount();
                    if (!markCyclicPath(anchor, node)) goto scan_finshed;
                    if (node == suspected) {
                        node->setAnchor_unsafe(anchor);
                    } else if (MARK_PINK_ON_SMALL_CIRCUIT_START && !isComplexCircuit && node->refCount() < 5) {
                        // rtgc_log("pink\n");
                        rtgc_assert(node->getAnchor()->get_color() == S_GRAY || node->getAnchor()->get_color() == S_PINK);
                        node->set_color(S_PINK);
                    }
                    break;
                }
                case S_BROWN: 
                    rtgc_trace_ref(RTGC_TRACE_GC, node, "found brown");
                    // node->tryDecreaseExternalRefCount();
                    node = findRedOrPink(node);
                    if (!markCyclicPath(anchor, node)) goto scan_finshed;
                    break;

                case S_PINK:
                case S_RED:
                    rtgc_trace_ref(RTGC_TRACE_GC, node, "found red");
                    // node->tryDecreaseExternalRefCount();
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
                        _external_rc += n->refCount() - 1;
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
    } else {
        rtgc_trace(RTGC_TRACE_GC, "not found garbage\n");
        for (auto iter = _circuitNodes.begin() + _cntCyclicGarbage; iter < _circuitNodes.end(); iter++) {
            finishScan(*iter, false);
        }
        _circuitNodes.resize(_cntCyclicGarbage);

        cnt = _toVisit.size();
        for (auto iter = _toVisit.adr_at(0) ; --cnt >= 0; iter++) {
            finishScan(iter->_node, false);
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
            rtgc_assert_ref(node, node->isEnquedToScan());

            rtgc_trace_ref(RTGC_TRACE_GC, node, "collectCyclicGarbage");
            if (node->isGarbageMarked()) {
                if (RTGC_DEBUG) {
                    node->unmarkEnquedToScan<false>();
                }
                toatlGarbageInSuspected --;
        rtgc_trace(RTGC_TRACE_GC, "totalGarbageInSuspected: %d suspected nodes\n", 
            toatlGarbageInSuspected);
                // rtgc_log("del suspected %p\n", node);
                rtgc::pal::deallocNode(this, node);
                continue;
            } 
            
            if (node->refCount() == 0) {
                node->unmarkEnquedToScan();
                enqueGarbage(node);
                this->destroyGarbages();
                continue;
            }

            if (!node->isUnstable()) {
                node->unmarkEnquedToScan();
                continue;
            }

            if (node->getExternalRefCount() != 0) {
                // node->markUnstable();
            } else if (!node->isPrimitiveRef() && node->isUnstable()) {
                collector.scanSuspected(node);
            }
            node->unmarkEnquedToScan();
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

void GCNode::unmarkEnquedToScanNode(GCNode* node, rtgc::GCContext* context) {
    for (auto it = context->_suspectedNodes->begin(); it != context->_suspectedNodes->end(); ++it) {
        // rtgc_assert((*it)->isThreadLocal());
        if (*it == node) {
            node->unmarkEnquedToScan();
            // node->markUnstable();
            *it = NULL;
            break;
        }
    }
    for (auto it = context->_unstableNodes.begin(); it != context->_unstableNodes.end(); ++it) {
        // rtgc_assert((*it)->isThreadLocal());
        if (*it == node) {
            node->unmarkEnquedToScan();
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
        rtgc_assert(!anchor->isGarbageMarked());

        if (anchor->marked()) {
            rc -= (RTGC_OBJECT_REF_INCREMENT >> (RTGC_NODE_FLAGS_BITS + RTGC_SAFE_REF_BITS));
            continue;
        }
        anchor->mark();
        nodes->push_back(anchor);

        GCNode* node;
        for (rtgc::pal::ChildNodeIterator iter(anchor); (node = iter.nextNode()) != NULL; ) {
            rtgc_assert_ref(node, !node->isGarbageMarked());
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
