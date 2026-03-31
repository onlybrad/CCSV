#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_LEXER_H
#define CCSV_LEXER_H

#include <stdbool.h>
#include "tokens.h"
#include "counters.h"

struct CCSV_Lexer {
    const char *data;
    size_t      length,
                position;
    char        separator;
};

void CCSV_Lexer_init(struct CCSV_Lexer*, const char *data, size_t length, char separator);
bool CCSV_Lexer_tokenize(struct CCSV_Lexer*, struct CCSV_Tokens*, struct CCSV_Counters*);

#endif

#ifdef __cplusplus
}
#endif