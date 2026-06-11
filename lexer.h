#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_LEXER_H
#define OB_CSV_LEXER_H

#include <stdbool.h>
#include "tokens.h"
#include "counters.h"

struct OB_CSV_Lexer {
    const char *data;
    size_t      length,
                position;
    char        separator;
};

void OB_CSV_Lexer_init(struct OB_CSV_Lexer*, const char *data, size_t length, char separator);
bool OB_CSV_Lexer_tokenize(struct OB_CSV_Lexer*, struct OB_CSV_Tokens*, struct OB_CSV_Counters*);

#endif

#ifdef __cplusplus
}
#endif