#include <assert.h>

#include "row.h"
#include "util.h"

EXTERN_C void OB_CSV_Row_init(OB_CSV_Row *const row) {
    assert(row != NULL);

    OB_CSV_Strings_init(row);
}