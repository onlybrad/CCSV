#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef OB_CSV_COUNTERS_H
#define OB_CSV_COUNTERS_H

struct OB_CSV_Counters {
    size_t strings,
           max_columns,
           chars;
};

void OB_CSV_Counters_init(struct OB_CSV_Counters *const counters);

#endif

#ifdef __cplusplus
}
#endif
