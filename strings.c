#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "strings.h"
#include "util.h"

#define CCSV_STRINGS_MINIMUM_CAPACITY 8U

EXTERN_C void CCSV_Strings_init(struct CCSV_Strings *const strings) {
    assert(strings != NULL);

    strings->data         = NULL;
    strings->count        = 0U;
    strings->capacity     = 0U;
    strings->total_length = 0U;
}

EXTERN_C bool CCSV_Strings_reserve(struct CCSV_Strings *const strings, size_t capacity, struct CCSV_Arenas *const arenas) {
    assert(strings != NULL);
    assert(arenas != NULL);

    if(capacity == 0) {
        capacity = CCSV_STRINGS_MINIMUM_CAPACITY;
    }

    if(capacity <= strings->capacity) {
        return true;
    }

    char **data = (char**)CCSV_ARENA_ALLOC(&arenas->strings, capacity, char*);
    if(data == NULL) {
        return false;
    }
    memcpy(data, strings->data, strings->count * sizeof(char*));

    strings->capacity = capacity;
    strings->data     = data;

    return true;
}

EXTERN_C bool CCSV_Strings_push(struct CCSV_Strings *const strings, const char *const string, const size_t length, struct CCSV_Arenas *const arenas) {
    assert(strings != NULL);
    assert(string != NULL);

    if(strings->count > SIZE_MAX - 1) {
        return false;
    }

    if(strings->count == strings->capacity) {
        bool success;
        const size_t capacity = CCSV_safe_mult(strings->capacity, 2, &success);
        if(!success) {
            return false;
        }

        if(!CCSV_Strings_reserve(strings, capacity, arenas)) {
            return false;
        }
    }

    char *const copy = CCSV_ARENA_ALLOC(&arenas->chars, length + 1U, char);
    if(copy == NULL) {
        return false;
    }
    memcpy(copy, string, length);
    copy[length] = '\0';

    strings->data[strings->count++] = copy;
    strings->total_length          += length;

    return true;
}

EXTERN_C bool CCSV_Strings_push_nocopy(struct CCSV_Strings *const strings, char *const string, struct CCSV_Arenas *const arenas) {
    if(strings->count > SIZE_MAX - 1U) {
        return false;
    }

    if(strings->count == strings->capacity) {
        bool success;
        const size_t capacity = CCSV_safe_mult(strings->capacity, 2, &success);
        if(!success) {
            return false;
        }

        if(!CCSV_Strings_reserve(strings, capacity, arenas)) {
            return false;
        }
    }

    const size_t length = strlen(string);

    strings->data[strings->count++] = string;
    strings->total_length          += length;

    return true;
}

bool CCSV_Strings_concat(const struct CCSV_Strings *const src, struct CCSV_Strings *const dst, struct CCSV_Arenas *arenas) {
    assert(src != NULL);
    assert(dst != NULL);
    assert(src != dst);
    assert(arenas != NULL);

    char *concat_string = CCSV_ARENA_ALLOC(&arenas->chars, src->total_length + 1U, char);
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

    return CCSV_Strings_push_nocopy(dst, concat_string_start, arenas);
}

void CCSV_Strings_reset(struct CCSV_Strings *const strings) {
    assert(strings != NULL);

    strings->count        = 0U;
    strings->total_length = 0U;
}
