#include "util.h"

#if defined(__MINGW32__) || !defined(_WIN32)
    #include <sys/time.h>
#endif
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include "allocator.h"

EXTERN_C void CCSV_print_bytes(const void *const buffer, const size_t size) {
    assert(buffer != NULL);
    assert(size > 0);

    putchar('[');
    for(size_t i = 0; i < size - 1; i++) {
        printf("0x%02hhx, ", ((const unsigned char*)buffer)[i]);
    }
    printf("0x%02hhx]\n", ((const unsigned char*)buffer)[size - 1]);
}

EXTERN_C uint64_t CCSV_usec_timestamp(void) {
#if defined(__MINGW32__) || !defined(_WIN32)
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (uint64_t)(current_time.tv_sec * 1000000L + current_time.tv_usec);
#else
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t tt = (uint64_t)ft.dwHighDateTime;
    tt <<= 32ULL;
    tt |= (uint64_t)ft.dwLowDateTime;
    tt /= 10ULL;
    tt -= 11644473600000000ULL;
    return tt;
#endif
}

EXTERN_C size_t CCSV_safe_mult(const size_t a, const size_t b, bool *const success) {
    assert(success != NULL);

    if(b == 0U) {
        *success = true;
        return 0U;
    }

    if(a > SIZE_MAX / b) {
        *success = false;
        return 0U;
    }

    *success = true;
    return a * b;
}
