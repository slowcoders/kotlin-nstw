#ifndef RTGC_DEF_H
#define RTGC_DEF_H

#include "Memory.h"
#include "Porting.h"
#include <assert.h>
#include "Common.h"

#define USE_RTGC                            1
#define RTGC_DEBUG                          1

#define RTGC_TRACE_REF                      0
#define RTGC_TRACE_FREE                     0
#define RTGC_TRACE_GC                       0


#if RTGC_DEBUG
#  define RTGC_INLINE                       // ignore
#  define rtgc_log                          konan::consolePrintf
#  define rtgc_assert_f(condition, format, ...)  \
    do { \
        if (!(condition)) { \
            ::kotlin::internal::RuntimeAssertFailedPanic(true, nullptr, format, ##__VA_ARGS__); \
        } \
    } while (false)
#  define rtgc_assert(condition)             rtgc_assert_f((condition), "Assert Fail: (" #condition ") at " CURRENT_SOURCE_LOCATION)
#  define rtgc_assert_ref(ref, condition)     if (!(condition)) { RTGC_dumpRefInfo(ref, "Assert Error: " CURRENT_SOURCE_LOCATION "\n" #condition); rtgc_assert(condition); }
#else   
#  define RTGC_INLINE                       inline
#  define rtgc_log(...)                     // ignore
#  define rtgc_assert_f(condition, ...)      // ignore
#  define rtgc_assert(condition)             // ignore
#  define rtgc_assert_ref(ref, condition)     // ignore
#endif

namespace rtgc {
    struct GCNode;
};

bool rtgc_trap(void* pObj) NO_INLINE;

bool RTGC_dumpRefInfo(const ObjHeader* obj, const char* msg = "*") NO_INLINE;
bool RTGC_dumpRefInfo(const rtgc::GCNode* node, const char* msg = "*") NO_INLINE;
void RTGC_dump(const char* msg, const TypeInfo* typeInfo, const ObjHeader* obj, const rtgc::GCNode* node);

extern const void* RTGC_debugInstance;
bool RTGC_is_debug_pointer(const ObjHeader* obj);
bool RTGC_is_debug_pointer(const rtgc::GCNode* obj);

#if RTGC_DEBUG 
#  define rtgc_trace(tag, format, ...)   if ((tag) == 1) konan::consolePrintf(format, ##__VA_ARGS__);
#  define rtgc_trace_ref(tag, obj, msg) \
      (((tag) && ((tag) == 1 || RTGC_is_debug_pointer(obj))) ? RTGC_dumpRefInfo(obj, msg) : 0)
#  define rtgc_trace_ref_2(tag, obj, owner, msg) \
      if ((tag) && ((tag) == 1 || RTGC_is_debug_pointer(obj))) { RTGC_dumpRefInfo(obj, msg); RTGC_dumpRefInfo(owner, " <-- ref-owner"); }
#else
#  define rtgc_trace(tag, format, ...)              // do nothing
#  define rtgc_trace_ref(tag, obj, msg)             false
#  define rtgc_trace_ref_2(tag, obj, owner, msg)    // do nothing
#endif

#endif // RTGC_DEF_H
