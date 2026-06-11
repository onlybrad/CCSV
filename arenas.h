#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_ARENAS_H
#define OB_CSV_ARENAS_H

#include "allocator.h"

struct OB_CSV_Arenas {
    struct OB_CSV_Arena strings;
    struct OB_CSV_Arena chars;
};

#endif

#ifdef __cplusplus
}
#endif