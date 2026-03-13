#ifdef __cplusplus
extern "C" {
#endif


#ifndef CCSV_COUNTERS_H
#define CCSV_COUNTERS_H

struct CCSV_Counters {
    unsigned strings,
             max_columns,
             chars;
};

void CCSV_Counters_init(struct CCSV_Counters *const counters);

#endif

#ifdef __cplusplus
}
#endif
