#include <stdlib.h>
#include <stdio.h>
#include "../ccsv.h"
#include "../util.h"

static void test_CCSV_from_file(void) {
    struct CCSV csv;
    enum CCSV_Error error = CCSV_from_file(&csv, "tests/test1.csv", ',');
    if(error != CCSV_ERROR_NONE) {
        exit(EXIT_FAILURE);
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
}

static void test_CCSV_to_file(void) {
    struct Row {
        const char *name;
        unsigned    age;
        int32_t     money;
        bool        is_noob;
    };

    const char *headers_data[] = {
        "name", "age", "money", "is noob" 
    };

    struct Row rows_data[] = {
        {"Person1", 10, -50, true},
        {"Person2", 10, -10, false},
        {"Person3", 10,  50, true},
        {"Person4", 10, 100, false}
    };

    struct CCSV_StructMember headers_members[] = {
        {CCSV_TYPE_STRING, sizeof(const char*) * 0, false},
        {CCSV_TYPE_STRING, sizeof(const char*) * 1, false},
        {CCSV_TYPE_STRING, sizeof(const char*) * 2, false},
        {CCSV_TYPE_STRING, sizeof(const char*) * 3, false},
    };

    struct CCSV_StructMember rows_members[] = {
        {CCSV_TYPE_STRING, CCSV_OFFSETOF(struct Row, name), false},
        {CCSV_TYPE_UINT,   CCSV_OFFSETOF(struct Row, age), false},
        {CCSV_TYPE_INT32,  CCSV_OFFSETOF(struct Row, money), false},
        {CCSV_TYPE_BOOL,   CCSV_OFFSETOF(struct Row, is_noob), false},
    };

    struct CCSV_Struct headers = {headers_data, headers_members, (unsigned)ARRAY_LENGTH(headers_members)};
    struct CCSV_Struct rows[] = {
        {rows_data + 0, rows_members, (unsigned)ARRAY_LENGTH(rows_members)},
        {rows_data + 1, rows_members, (unsigned)ARRAY_LENGTH(rows_members)},
        {rows_data + 2, rows_members, (unsigned)ARRAY_LENGTH(rows_members)},
        {rows_data + 3, rows_members, (unsigned)ARRAY_LENGTH(rows_members)},
    };


    const char *const path = "./tests/person.csv";
    if(!CCSV_to_file(&headers, rows, (unsigned)ARRAY_LENGTH(rows), path, ',')) {
        exit(EXIT_FAILURE);
    }

    remove(path);
}

int main(void) {
    test_CCSV_from_file();
    test_CCSV_to_file();

    return EXIT_SUCCESS;
}