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

#if 0
namespace rtgc {
struct GlobalCircuitLock {
    GlobalCircuitLock() {
        // Acquire a global lock to prevent concurrent modifications to the circuit.
        // This is a placeholder for actual locking logic.
    }

    ~GlobalCircuitLock() {
        // Release the global lock.
        // This is a placeholder for actual unlocking logic.
    }
};

class GCCircuit {

};

class GCRing : public GCNode {
    GCNode* _sharedCyclicHead;
    GCCircuit* _circuit;
public:
    GCRing() : GCNode() {
    }

    ~GCRing() {
        // This destructor is called when the ring is destroyed.
        // It should not be used to free resources, as the ring is managed by the garbage collector.
        rtgc_trace(RTGC_TRACE_GC, "Ring destroyed\n");
    }

    bool isCyclic() const {
        return true;
    }

    template<bool Atomic>
    RTGC_INLINE void addReferrer(GCNode* referrer) {
        while (this->isCyclic()) {
            GlobalCircuitLock lock;
            if (!this->isCyclic()) break;

        }
        // Marking a ring does not change its state, but it can be used for debugging purposes.
        rtgc_trace(RTGC_TRACE_GC, "Marking Ring\n");
    }

    template<bool Atomic>
    RTGC_INLINE ReachableState removeReferrer(GCNode* referrer) {
        while (this->isCyclic()) {
            GlobalCircuitLock lock;
            if (!this->isCyclic()) break;

            rtgc_assert_ref(this, this->_anchor != NULL);
            GCRing* this_ring = this->getRing();
            if (referrer == this->anchor_ || this_ring == this->_anchor && this_ring->_sharedCyclicHead == referrer) {
                rtgc_assert_ref(referrer, referrer->isCyclic());
                this_ring->breakRing(referrer);
                return ReachableState::Unstable;
            } 

            this->decreaseObjectRefCount<Atomic>(false);
            GCCircuit* this_circuit = this_ring->getCircuit();
            if (referrer->isCyclic()) {
                GCCircuit* anchor_circuit = referrer->getCircuit();
                if (this_circuit == anchor_circuit) return;
            } 
            
            this_circuit->_extRefCount -= 1;
        }
    }

    bool isRing() const { return true; };

    void breakRing(GCNode* tail) {
        NodeVector broken_nodes;
        for (GCNode* broken = tail; broken != NULL; broken = broken->getAnchor()) {
            broken->markBrokenCyclic();
            broken_nodes.push_back(broken);
        }

        for (int idx_tail = 0; idx_tail < broken_nodes.size(); idx_tail ++) {
            GCNode* broken_tail = broken_nodes.at(idx_tail);
            rtgc_assert_ref(broken_tail, broken_tail->isCyclic());
            if (!broken_tail->isSharedCyclic()) continue;

            for (auto* referent : broken_tail->getReferents()) {
                GCNode* anchor = referent->getAnchor();
                if (anchor == NULL || !anchor->isRing()) continue;

                GCRing* referent_ring = (GCRing*)anchor;
                GCNode* shared_head = referent_ring->_sharedCyclicHead;

                if (shared_head == broken_tail) {
                    if (shared_head->isBrokenCyclic()) {
                        shared_head->unmarkBrokenCyclic();
                        shared_head->setAnchor_unsafe(referent_ring->getAnchor());

                        referent->setAnchor_unsafe(broken_tail);
                        GCRing::dealloc(referent_ring);
                    } else {
                        // resuse referent_ring
                    }
                    continue;
                } 
                
                int idx_head = idx_tail;
                for (; --idx_head >= 0; ) {
                    if (broken_nodes.at(idx_head) == shared_head) {
                        goto found_shared_head;
                    }
                }
                // 
                referent_ring->breakRing(broken_tail);
                continue;

            found_shared_head:
                int idx_node = idx_tail;
                if (broken_tail->isBrokenCyclic()) {
                    // tail 변경
                    referent->setAnchor_unsafe(broken_tail);
                    broken_tail->unmarkBrokenCyclic();
                    for (; --idx_node >= idx_head; ) {
                        GCNode* node = broken_nodes.at(idx_node);
                        if (node->isBrokenCyclic()) {
                            node->unmarkBrokenCyclic();
                        } else {
                            broken_nodes.at(idx_node+1)->setAnchor_unsafe(referent_ring);
                        }
                    }
                }

                if (shared_head->isBrokenCyclic()) {
                    // do not unmarkBrokenCyclic shared_head;
                    shared_head->setAnchor_unsafe(referent_ring->getAnchor());
                    for (; ++ idx_head <= idx_tail; ) {
                        GCNode* node = broken_nodes.at(idx_head);
                        if (node->isBrokenCyclic()) {
                            node->unmarkBrokenCyclic();
                        } else {
                            node->setAnchor_unsafe(referent_ring);
                            referent_ring->_sharedCyclicHead = node;
                            referent_ring->setAnchor_unsafe(broken_nodes.at(idx_node-1));
                            break;
                        }
                    }
                    rtgc_assert(idx_head <= idx_tail);
                }
            }
        }
        for (int idx_tail = 0; idx_tail < broken_nodes.size(); idx_tail ++) {
            GCNode* broken_tail = broken_nodes.at(idx_tail);
            if (broken_tail->isBrokenCyclic()) {
                broken_tail->unmarkCyclic();
            }
        }
    }
};
#endif