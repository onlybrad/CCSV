#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_Tokens_H
#define CCSV_Tokens_H

#define CCSV_TOKENS_MINIMUM_CAPACITY 8

#include <stdbool.h>
#include <stdint.h>

#include "token.h"

struct CCSV_Tokens {
    struct CCSV_Token *data;
    size_t             count,
                       capacity;
};

void CCSV_Tokens_init              (struct CCSV_Tokens *tokens);
void CCSV_Tokens_free              (struct CCSV_Tokens *tokens);
bool CCSV_Tokens_reserve           (struct CCSV_Tokens *tokens, size_t capacity);
void CCSV_Tokens_reset             (struct CCSV_Tokens *tokens);
struct CCSV_Token *CCSV_Tokens_next(struct CCSV_Tokens *tokens);

#endif

#ifdef __cplusplus
}
#endif
