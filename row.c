#include <assert.h>

#include "row.h"
#include "util.h"

EXTERN_C void CCSV_Row_init(CCSV_Row *const row) {
    assert(row != NULL);

    CCSV_Strings_init(row);
}