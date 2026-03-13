#include <stdlib.h>
#include <stdio.h>
#include "../ccsv.h"

int main(void) {
    struct CCSV csv;
    enum CCSV_Error error = CCSV_from_file(&csv, "tests/test1.csv", ',');
    if(error != CCSV_ERROR_NONE) {
        return EXIT_FAILURE;
    }

    CCSV_Row row;
    error = CCSV_next_row(&csv, &row);
    while(error == CCSV_ERROR_NONE) {
        for(unsigned i = 0U; i < row.count - 1U; i++) {
            printf("%s, ", row.data[i]);
        }
        puts(row.data[row.count - 1U]);
        error = CCSV_next_row(&csv, &row);
    }

    CCSV_free(&csv);

    return EXIT_SUCCESS;
}