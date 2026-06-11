#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "allocator.h"
#include "util.h"

EXTERN_C void OB_CSV_Tokens_init(struct OB_CSV_Tokens *const tokens) {
    assert(tokens != NULL);

    tokens->data     = NULL;
    tokens->capacity = 0U;
    tokens->count    = 0U;
}

EXTERN_C bool OB_CSV_Tokens_reserve(struct OB_CSV_Tokens *const tokens, size_t capacity) {
    assert(tokens != NULL);

    if(capacity < OB_CSV_TOKENS_MINIMUM_CAPACITY) {
        capacity = OB_CSV_TOKENS_MINIMUM_CAPACITY;
    }

    if(capacity <= tokens->capacity) {
        return true;
    }

    struct OB_CSV_Token *data = (struct OB_CSV_Token*)OB_CSV_REALLOC(tokens->data, (size_t)capacity * sizeof(*data));
    if(data == NULL) {
        return false;
    }

    tokens->capacity = capacity;
    tokens->data     = data;

    return true;
}

void OB_CSV_Tokens_reset(struct OB_CSV_Tokens *const tokens) {
    assert(tokens != NULL);

    tokens->count = 0U;
}

EXTERN_C void OB_CSV_Tokens_free(struct OB_CSV_Tokens *const tokens) {
    assert(tokens != NULL);

    OB_CSV_FREE(tokens->data);
    OB_CSV_Tokens_init(tokens);
}

EXTERN_C struct OB_CSV_Token *OB_CSV_Tokens_next(struct OB_CSV_Tokens *const tokens) {
    assert(tokens != NULL);

    if(tokens->count > SIZE_MAX - 1) {
        return NULL;
    }

    if(tokens->count == tokens->capacity) {
        bool success;
        const size_t new_capacity = OB_CSV_safe_mult(tokens->capacity, 2, &success);
        if(!success || !OB_CSV_Tokens_reserve(tokens, new_capacity)) {
            return NULL;
        }
    }

    struct OB_CSV_Token *const token = tokens->data + tokens->count;
    tokens->count++;

    return token;
}
