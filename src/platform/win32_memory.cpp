#include "pal.h"
#include <objbase.h>

#if !XLANG_PLATFORM_WINDOWS
#error "This file is only for targeting Windows"
#endif

namespace xlang
{
    void* XLANG_CALL xlang_mem_alloc(size_t count) noexcept
    {
        return ::CoTaskMemAlloc(count);
    }

    void XLANG_CALL xlang_mem_free(void* ptr) noexcept
    {
        return ::CoTaskMemFree(ptr);
    }
}
