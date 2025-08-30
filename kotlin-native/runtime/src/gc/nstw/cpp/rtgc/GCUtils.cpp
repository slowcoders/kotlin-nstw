

#ifdef _MSC_VER
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
#endif

#include "GCUtils.hpp"

#define _USE_JVM 0
#define _USE_MMAP 1
#define _ULIMIT 1
void* rtgc::mem::VirtualMemory::reserve_memory(size_t bytes) {
#if RTGC_DEBUG    
    int remain_check = bytes % MEM_BUCKET_SIZE;
    rtgc_assert(remain_check == 0);
#endif
    void* addr;    
#if _USE_JVM
    size_t total_reserved = bytes;
    size_t page_size = os::vm_page_size();
    size_t alignment = page_size;
    ReservedHeapSpace total_rs(total_reserved, alignment, page_size, NULL);
    addr = total_rs.base();// nullptr;
#elif defined(_MSC_VER)
    addr = VirtualAlloc(nullptr, bytes, MEM_RESERVE, PAGE_READWRITE);
#elif _USE_MMAP
    addr = mmap(nullptr, bytes, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
#else
    addr = malloc(bytes);
#if _ULIMIT    
    addr = realloc(addr, 4096);
#endif
#endif
    rtgc_assert_f(addr != NULL && addr != (void*)-1, 
        "fail to memory reservation size: %lu M", (bytes/(1024*1024)));
    if (false) rtgc_log("reserve_memory %p %dk\n", addr, (int)(bytes/1024));
    return addr;
}

void rtgc::mem::VirtualMemory::commit_memory(void* addr, void* bucket, size_t bytes) {
#if RTGC_DEBUG    
    int remain_check = bytes % MEM_BUCKET_SIZE;
    rtgc_assert(remain_check == 0);
#endif
#if _USE_JVM
    if (true) rtgc_log("commit_memory\n");
    return;
#elif defined(_MSC_VER)
    addr = VirtualAlloc(bucket, MEM_BUCKET_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (addr != 0) return;
#elif _USE_MMAP
    int res = mprotect(bucket, bytes, PROT_READ|PROT_WRITE);
    if (false) rtgc_log("commit_memory mprotect %p:%p %d res=%d\n", addr, bucket, (int)(bytes/1024), res);
    if (res == 0) return;
#elif _ULIMIT    
    void* mem = ::realloc(addr, offset + bytes);
    if (mem == addr) return;
#endif
    rtgc_assert_f(0, "OutOfMemoryError:E009");
}

void rtgc::mem::VirtualMemory::free(void* addr, size_t bytes) {
#if RTGC_DEBUG    
    int remain_check = bytes % MEM_BUCKET_SIZE;
    rtgc_assert(remain_check == 0);
#endif
#if _USE_JVM
    fatal(1, "free_memory\n");
    return;
#elif defined(_MSC_VER)
    addr = VirtualFree(addr, bytes, MEM_RELEASE);
    if (addr != 0) return;
#elif _USE_MMAP
    int res = munmap(addr, bytes);
    if (res == 0) return;
#else    
    ::free(addr);
    return;
#endif
    rtgc_assert_f(0, "Invalid Address:E00A");
}
