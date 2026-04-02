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
        "name", "age", "money", "\"is noob\"" 
    };

    struct Row rows_data[] = {
        {"Person1", 10, -50, true},
        {"Person2", 10, -10, false},
        {"Person3", 10,  50, true},
        {"Person4", 10, 100, false}
    };

    struct CCSV_StructMember headers_members[] = {
        {CCSV_TYPE_STRING, sizeof(const char*) * 0},
        {CCSV_TYPE_STRING, sizeof(const char*) * 1},
        {CCSV_TYPE_STRING, sizeof(const char*) * 2},
        {CCSV_TYPE_STRING, sizeof(const char*) * 3},
    };

    struct CCSV_StructMember rows_members[] = {
        {CCSV_TYPE_STRING, CCSV_OFFSETOF(struct Row, name)},
        {CCSV_TYPE_UINT,   CCSV_OFFSETOF(struct Row, age)},
        {CCSV_TYPE_INT32,  CCSV_OFFSETOF(struct Row, money)},
        {CCSV_TYPE_BOOL,   CCSV_OFFSETOF(struct Row, is_noob)},
    };

    struct CCSV_Structs headers;
    headers.data         = headers_data;
    headers.members      = headers_members;
    headers.member_count = (unsigned)ARRAY_LENGTH(headers_members);
    headers.count        = 1; //the whole array counts as 1 "struct"
    headers.size         = sizeof(headers_data); //the size of "struct" is the whole array
    
    struct CCSV_Structs rows;
    rows.data         = rows_data;
    rows.members      = rows_members;
    rows.member_count = (unsigned)ARRAY_LENGTH(rows_members);
    rows.count        = ARRAY_LENGTH(rows_data);
    rows.size         = sizeof(struct Row);

    const char *const path = "./tests/person.csv";
    if(!CCSV_to_file(&headers, &rows, path, ',')) {
        exit(EXIT_FAILURE);
    }

    remove(path);
}

int main(void) {
    test_CCSV_from_file();
    test_CCSV_to_file();

    return EXIT_SUCCESS;
}