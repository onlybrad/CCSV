#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "strings.h"
#include "util.h"

#define OB_CSV_STRINGS_MINIMUM_CAPACITY 8U

EXTERN_C void OB_CSV_Strings_init(struct OB_CSV_Strings *const strings) {
    assert(strings != NULL);

    strings->data         = NULL;
    strings->count        = 0U;
    strings->capacity     = 0U;
    strings->total_length = 0U;
}

EXTERN_C bool OB_CSV_Strings_reserve(struct OB_CSV_Strings *const strings, size_t capacity, struct OB_CSV_Arenas *const arenas) {
    assert(strings != NULL);
    assert(arenas != NULL);

    if(capacity == 0) {
        capacity = OB_CSV_STRINGS_MINIMUM_CAPACITY;
    }

    if(capacity <= strings->capacity) {
        return true;
    }

    char **data = (char**)OB_CSV_ARENA_ALLOC(&arenas->strings, capacity, char*);
    if(data == NULL) {
        return false;
    }
    memcpy(data, strings->data, strings->count * sizeof(char*));

    strings->capacity = capacity;
    strings->data     = data;

    return true;
}

EXTERN_C bool OB_CSV_Strings_push(struct OB_CSV_Strings *const strings, const char *const string, const size_t length, struct OB_CSV_Arenas *const arenas) {
    assert(strings != NULL);
    assert(string != NULL);

    if(strings->count > SIZE_MAX - 1) {
        return false;
    }

    if(strings->count == strings->capacity) {
        bool success;
        const size_t capacity = OB_CSV_safe_mult(strings->capacity, 2, &success);
        if(!success) {
            return false;
        }

        if(!OB_CSV_Strings_reserve(strings, capacity, arenas)) {
            return false;
        }
    }

    char *const copy = OB_CSV_ARENA_ALLOC(&arenas->chars, length + 1U, char);
    if(copy == NULL) {
        return false;
    }
    memcpy(copy, string, length);
    copy[length] = '\0';

    strings->data[strings->count++] = copy;
    strings->total_length          += length;

    return true;
}

EXTERN_C bool OB_CSV_Strings_push_nocopy(struct OB_CSV_Strings *const strings, char *const string, struct OB_CSV_Arenas *const arenas) {
    if(strings->count > SIZE_MAX - 1U) {
        return false;
    }

    if(strings->count == strings->capacity) {
        bool success;
        const size_t capacity = OB_CSV_safe_mult(strings->capacity, 2, &success);
        if(!success) {
            return false;
        }

        if(!OB_CSV_Strings_reserve(strings, capacity, arenas)) {
            return false;
        }
    }

    const size_t length = strlen(string);

    strings->data[strings->count++] = string;
    strings->total_length          += length;

    return true;
}

bool OB_CSV_Strings_concat(const struct OB_CSV_Strings *const src, struct OB_CSV_Strings *const dst, struct OB_CSV_Arenas *arenas) {
    assert(src != NULL);
    assert(dst != NULL);
    assert(src != dst);
    assert(arenas != NULL);

    char *concat_string = OB_CSV_ARENA_ALLOC(&arenas->chars, src->total_length + 1U, char);
    if(concat_string == NULL) {
        return false;
    }
    concat_string[0] = '\0';
    char *const concat_string_start = concat_string;

    for(size_t i = 0; i < src->count; i++) {
        const size_t length = strlen(src->data[i]);
        strcpy(concat_string, src->data[i]);
        concat_string += length;
    }

    return OB_CSV_Strings_push_nocopy(dst, concat_string_start, arenas);
}

void OB_CSV_Strings_reset(struct OB_CSV_Strings *const strings) {
    assert(strings != NULL);

    strings->count        = 0U;
    strings->total_length = 0U;
}
