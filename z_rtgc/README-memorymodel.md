## Memory model module and selector
```kotlin
// kotlin-native/runtim/build.gradle.kts
    module("strict")
    module("relaxed")
    module("legacy_memory_manager", file("src/legacymm")) 
    module("experimental_memory_manager", file("src/mm"))

// KonanConfig.kt:249
    internal val runtimeNativeLibraries: List<String> = mutableListOf<String>().apply {
        if (debug) add("debug.bc")
        when (memoryModel) {
            MemoryModel.STRICT -> {
                add("strict.bc")
                add("legacy_memory_manager.bc")
            }
            MemoryModel.RELAXED -> {
                add("relaxed.bc")
                add("legacy_memory_manager.bc")
            }
            MemoryModel.EXPERIMENTAL -> {
                add("common_gc.bc")
                add("experimental_memory_manager.bc")
                when (gc) {
                    GC.SAME_THREAD_MARK_AND_SWEEP -> {
                        add("same_thread_ms_gc.bc")
                    }
                    GC.NOOP -> {
                        add("noop_gc.bc")
                    }
                    GC.CONCURRENT_MARK_AND_SWEEP -> {
                        add("concurrent_ms_gc.bc")
                    }
                }
            }
        }
        if (shouldCoverLibraries || shouldCoverSources) add("profileRuntime.bc")
        if (target.supportsCoreSymbolication()) {
            add("source_info_core_symbolication.bc")
        }
        if (target.supportsLibBacktrace()) {
            add("source_info_libbacktrace.bc")
            add("libbacktrace.bc")
        }
        when (allocationMode) {
            AllocationMode.MIMALLOC -> {
                add("opt_alloc.bc")
                add("mimalloc.bc")
            }
            AllocationMode.STD -> {
                add("std_alloc.bc")
            }
        }
    }
```

## about new memory model
The most noticeable change in the new memory manager is lifting restrictions on object sharing. 
You don't need to freeze objects to share them between threads, specifically:
* Top-level properties can be accessed and modified by any thread without using @SharedImmutable.
* Objects passing through interop can be accessed and modified by any thread without freezing them.
* Worker.executeAfter no longer requires operations to be frozen.
* Worker.execute no longer requires producers to return an isolated object subgraph.
* Reference cycles containing AtomicReference and FreezableAtomicReference do not cause memory leaks.