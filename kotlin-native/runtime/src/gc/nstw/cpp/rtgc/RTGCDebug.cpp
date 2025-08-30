#include "MemoryPrivate_rtgc.hpp"
#include "KString.h"
#include "Porting.h"
#include "std_support/CStdlib.hpp"
#include "Natives.h"
#include "StackTrace.hpp"
#include "RTGC_PAL_impl.h"
#include <string.h>

using namespace rtgc;

static const bool DUMP_STACK_TARACE = 0;
static const char* debugClassNames[] = {
    0,
    "kotlin:AssertionError",
    // "kotlin.text:PatternSyntaxException",
    // ":SplayTree.Node",
    // "kotlinx.cli:ArgParserResult",
    // ":Lazy",
    // "kotlin:Any",
    // "kotlin:Array",
    // "kotlin:ByteArray",
    // "kotlin:CharArray",
    // "kotlin:IntArray",
    // "kotlin:Pair",
    // "kotlin:Error",
    // "kotlin:String",
    // "kotlin:IllegalArgumentException",
    // "kotlin:IllegalStateException",
    // "kotlin.collections:ArrayList",
    // "kotlin.collections:HashMap",
    // "kotlin.collections:HashMap.KeysItr",
    // "kotlin.collections:HashMap.EntriesItr",
    // "kotlin.native.ref:WeakReference",
    // "kotlin.native.concurrent:Lock",
    // "kotlin.native.concurrent:AtomicInt",
    // "kotlin.native.concurrent:WorkerBoundReference",
    // "kotlin.native.concurrent:AtomicReference",
    // "kotlin.native.internal.test:TopLevelSuite",
    // "kotlin.native.internal.test:TopLevelSuite.TestCase",
    // "kotlin.native.internal:$createCleaner$lambda$0$FUNCTION_REFERENCE$28",
    // "kotlin.native.internal:CleanerImpl",
    // "kotlinx.cinterop:ForeignObjCObject",
    // "runtime.concurrent.worker_bound_reference0:A",
    // "runtime.concurrent.worker_bound_reference0:C1",
    // "runtime.basic.cleaner_workers:FunBox",
    // "runtime.basic.cleaner_basic:FunBox",
};

constexpr int CNT_DEBUG_CLASS = sizeof(debugClassNames) / sizeof(debugClassNames[0]);
static const TypeInfo* debugKlass[CNT_DEBUG_CLASS];
const void* RTGC_debugInstance = NULL;

static const ObjHeader* objectOf(const GCNode* node) {
    return (node == NULL || node->isImmutableRoot()) ? nullptr : (const ObjHeader*)pal::toObject(node);
}

static const char* skip_text(const char* text, ArrayHeader* kstr, char end_of_text) {
    if (kstr == NULL) {
        return text[0] == end_of_text ? text + 1 : NULL;
    }
    int len_name = kstr->count_;
    const KChar* utf16 = CharArrayAddressOfElementAt(kstr, 0);
    for (; --len_name >= 0 && *text != end_of_text; ) {
        if (*text++ != *utf16++) return NULL;
    }
    if (len_name == -1 && *text == end_of_text) {
        return text + 1;
    }
    return NULL;
}

bool RTGC_is_debug_pointer(const ObjHeader* obj) {
    if (obj == NULL) return false;

    const TypeInfo* objType = obj->type_info();
    if (objType == nullptr) return false;

    if (true) {
        GCNode* node = pal::toNode(obj);
        // bool isImmutable = node != NULL && node->isImmutable();
        bool isImmRoot = node != NULL && node->isImmutableRoot();
        if (isImmRoot) {
            rtgc_trace_ref(isImmRoot, node, "imm root");
        }
        return isImmRoot;// isImmutable;
    }

    if (RTGC_debugInstance != NULL) {
        return RTGC_debugInstance == obj;
    }

    for (int i = 0; i < CNT_DEBUG_CLASS; i ++) {
        const char* className = debugClassNames[i];
        if (className == NULL) continue;
        if (debugKlass[i] == NULL) {
            ArrayHeader* package = objType->packageName_->array();
            ArrayHeader* name = objType->relativeName_->array();
            const char* cp = skip_text(className, package, ':');
            if (cp != NULL && skip_text(cp, name, 0) != NULL) {
                debugKlass[i] = objType;
                RTGC_dump("add-debug-class", objType, obj, pal::toNode(obj));
                return true;
            }
        } else if (objType == debugKlass[i]) {
            // if (RTGC_debugInstance == NULL) {
            //     RTGC_debugInstance = obj;
            // }
            // return RTGC_debugInstance == obj;
            return true;
        }
    }
    return false;
}

bool RTGC_is_debug_pointer(const GCNode* node) {
    const ObjHeader* obj = objectOf(node);
    return RTGC_is_debug_pointer(obj);
}

static volatile bool ENABLE_RTGC_MM_trap = RTGC_DEBUG;
bool rtgc_trap(void* pObj) {
    return ENABLE_RTGC_MM_trap;
}

bool RTGC_dumpRefInfo(const ObjHeader* obj, const char* msg) {
    if (obj == NULL) return false;
    RTGC_dump(msg, obj->type_info(), obj, pal::toNode(obj));
    return true;
}

bool RTGC_dumpRefInfo(const GCNode* node, const char* msg) {
    const ObjHeader* obj = objectOf(node);
    const TypeInfo* objType = obj == NULL ? NULL : obj->type_info();
    RTGC_dump(msg, objType, obj, node);
    return true;
}

void RTGC_dump(const char* msg, const TypeInfo* objType, const ObjHeader* obj, const GCNode* node) {
    if (false) {
        konan::consolePrintf("%s %p th=%lx rc=%llx\n", 
            msg, node, konan::currentThreadId(), node == NULL ? 0 : node->refCountBitFlags());
        return;
    }

    const char* classname = "??";
    const char* package_ = "??";
    const char* super_classname = "??";
    const char* super_package_ = "??";
    if (objType != NULL) {
        if (objType->relativeName_ != NULL) {
            classname = CreateCStringFromString(objType->relativeName_);
        }
        if (objType->packageName_ != NULL) {
            package_ = CreateCStringFromString(objType->packageName_);
        }
        if (false && objType->superType_ != NULL) {
            if (objType->superType_->relativeName_ != NULL) {
                super_classname = CreateCStringFromString(objType->superType_->relativeName_);
            }
            if (objType->superType_->packageName_ != NULL) {
                super_package_ = CreateCStringFromString(objType->superType_->packageName_);
            }
        }
    } else {
        classname = "?";
        package_ = "?";
    }

    if (!strcmp(msg, "skip zeroStackRef")) {
        konan::consolePrintf("--- %s\n", msg);
    }

    char stackTrace[8192*8] = "\0";
    if (DUMP_STACK_TARACE) {
        constexpr int kSkipFrames = 1;
        kotlin::StackTrace trace = kotlin::StackTrace<>::current(kSkipFrames);
        auto stackTraceStrings = kotlin::GetStackTraceStrings(trace.data());
        char* stBuff = stackTrace; 
        for (auto& frame : stackTraceStrings) {
            memcpy(stBuff, frame.c_str(), frame.size());
            stBuff += frame.size();
            *stBuff ++ = '\n';
        }
        *stBuff ++ = '\n';
        stackTrace[922] = '\n'; 
        stackTrace[923] = 0; 
    }

    GCNode* anchor = node == NULL ? NULL : node->getAnchor();
    konan::consolePrintf("%s [%p] -> %p(%s:%s) th: %lx rc_flags=%llx\n%s", 
        msg, anchor, node, package_, classname,
        konan::currentThreadId(), node == NULL ? 0 : node->refCountBitFlags(), stackTrace);

    if (classname[0] != '?') DisposeCString((char*)classname);
    if (package_[0] != '?') DisposeCString((char*)package_);
    if (super_package_[0] != '?') DisposeCString((char*)super_package_);
    if (super_classname[0] != '?') DisposeCString((char*)super_classname);
    // kotlin::PrintStackTraceStderr();
    rtgc_trap(NULL);
}
