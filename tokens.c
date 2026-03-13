#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "allocator.h"
#include "util.h"

EXTERN_C void CCSV_Tokens_init(struct CCSV_Tokens *const tokens) {
    assert(tokens != NULL);

    tokens->data     = NULL;
    tokens->capacity = 0U;
    tokens->count    = 0U;
}

EXTERN_C bool CCSV_Tokens_reserve(struct CCSV_Tokens *const tokens, unsigned capacity) {
    assert(tokens != NULL);

    if(capacity < CCSV_TOKENS_MINIMUM_CAPACITY) {
        capacity = CCSV_TOKENS_MINIMUM_CAPACITY;
    }

    if(capacity <= tokens->capacity) {
        return true;
    }

    struct CCSV_Token *data = (struct CCSV_Token*)CCSV_REALLOC(tokens->data, (size_t)capacity * sizeof(*data));
    if(data == NULL) {
        return false;
    }

    tokens->capacity = capacity;
    tokens->data     = data;

    return true;
}

void CCSV_Tokens_reset(struct CCSV_Tokens *const tokens) {
    assert(tokens != NULL);

    tokens->count = 0U;
}

EXTERN_C void CCSV_Tokens_free(struct CCSV_Tokens *const tokens) {
    assert(tokens != NULL);

    CCSV_FREE(tokens->data);
    CCSV_Tokens_init(tokens);
}

EXTERN_C struct CCSV_Token *CCSV_Tokens_next(struct CCSV_Tokens *const tokens) {
    assert(tokens != NULL);

    if(tokens->count == tokens->capacity) {
        bool success;
        const unsigned new_capacity = CCSV_safe_unsigned_mult(tokens->capacity, 2U, &success);
        if(!success || !CCSV_Tokens_reserve(tokens, new_capacity)) {
            return NULL;
        }
    }

    struct CCSV_Token *const token = tokens->data + tokens->count;
    tokens->count++;

    return token;
}
