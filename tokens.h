#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_Tokens_H
#define OB_CSV_Tokens_H

#define OB_CSV_TOKENS_MINIMUM_CAPACITY 8

#include <stdbool.h>
#include <stdint.h>

#include "token.h"

struct OB_CSV_Tokens {
    struct OB_CSV_Token *data;
    size_t             count,
                       capacity;
};

void OB_CSV_Tokens_init              (struct OB_CSV_Tokens *tokens);
void OB_CSV_Tokens_free              (struct OB_CSV_Tokens *tokens);
bool OB_CSV_Tokens_reserve           (struct OB_CSV_Tokens *tokens, size_t capacity);
void OB_CSV_Tokens_reset             (struct OB_CSV_Tokens *tokens);
struct OB_CSV_Token *OB_CSV_Tokens_next(struct OB_CSV_Tokens *tokens);

#endif

#ifdef __cplusplus
}
#endif
