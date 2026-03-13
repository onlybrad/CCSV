#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_ARENAS_H
#define CCSV_ARENAS_H

#include "allocator.h"

struct CCSV_Arenas {
    struct CCSV_Arena strings;
    struct CCSV_Arena chars;
};

#endif

#ifdef __cplusplus
}
#endif