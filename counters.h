#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef CCSV_COUNTERS_H
#define CCSV_COUNTERS_H

struct CCSV_Counters {
    size_t strings,
           max_columns,
           chars;
};

void CCSV_Counters_init(struct CCSV_Counters *const counters);

#endif

#ifdef __cplusplus
}
#endif
