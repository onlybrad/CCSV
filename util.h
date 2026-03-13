#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_UTIL_H
#define CCSV_UTIL_H

#ifndef _WIN32
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif
#endif

#ifndef MIN
    #define MIN(A,B) ((A)>(B)?(B):(A))
#endif
#ifndef MAX
    #define MAX(A,B) ((A)>(B)?(A):(B))
#endif

#ifndef static_strlen
    #define static_strlen(STR) (sizeof(STR) - 1)
#endif

#ifndef UNSIGNED_TO_VOID_PTR
    #define UNSIGNED_TO_VOID_PTR(UNSIGNED)((void*)(uintptr_t)(UNSIGNED))
#endif
#ifndef VOID_PTR_TO_UNSIGNED
    #define VOID_PTR_TO_UNSIGNED(VOID_PTR)((unsigned)(uintptr_t)(VOID_PTR))
#endif

#ifndef DBL_PRECISION
    #ifdef DBL_DECIMAL_DIG
        #define DBL_PRECISION DBL_DECIMAL_DIG
    #else
        #define DBL_PRECISION DBL_DIG
    #endif
#endif

void     CCSV_print_bytes   (const void *buffer, const size_t size);
uint64_t CCSV_usec_timestamp(void);

unsigned CCSV_safe_unsigned_mult          (unsigned a, unsigned b, bool *success);
unsigned CCSV_safe_unsigned_add           (unsigned a, unsigned b, bool *success);
bool     CCSV_check_unsigned_mult_overflow(unsigned a, unsigned b);

#endif

#ifdef __cplusplus
}
#endif