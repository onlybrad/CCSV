#include <assert.h>
#include <stdlib.h>

#include "counters.h"
#include "util.h"

EXTERN_C void OB_CSV_Counters_init(struct OB_CSV_Counters *const counters) {
    assert(counters != NULL);

    counters->chars       = 0U;
    counters->max_columns = 0U;
    counters->strings     = 0U;
}
