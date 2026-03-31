#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"
#include "util.h"

static size_t CCSV_Lexer_read_string(const struct CCSV_Lexer *const lexer) {
    assert(lexer != NULL);

    const size_t position = lexer->position; 

    size_t i;
    for(i = 0U; position + i < lexer->length; i++) {
        const char c = lexer->data[position + i];
        
        if(c == '"' || c == '\n' || c == '\r' || c == lexer->separator) {
            return i;
        }
    }

    return i;
}

EXTERN_C void CCSV_Lexer_init(struct CCSV_Lexer *const lexer, const char *const data, const size_t length, const char separator) {
    assert(lexer != NULL);
    assert(data != NULL);
    assert(length > 0U);
    assert(separator != '\0');

    lexer->data      = data;
    lexer->length    = length;
    lexer->position  = 0;
    lexer->separator = separator;
}

EXTERN_C bool CCSV_Lexer_tokenize(struct CCSV_Lexer *const lexer, struct CCSV_Tokens *const tokens, struct CCSV_Counters *const counters) {
    assert(lexer != NULL);
    assert(tokens != NULL);
    assert(counters != NULL);

    size_t current_column_count = 0;

    while(lexer->position < lexer->length) {
        struct CCSV_Token *token = CCSV_Tokens_next(tokens);
        if(token == NULL) {
            return false;
        }

        token->value = lexer->data + lexer->position;
        if(*token->value == lexer->separator) {
            token->length    = 1U;
            token->type      = CCSV_TOKEN_SEPARATOR;
            counters->chars += sizeof(lexer->separator);
            current_column_count++;
        } else switch(*token->value) {
        case '"':
            token->length    = 1U;
            token->type      = CCSV_TOKEN_DBLQUOTE;
            counters->chars += sizeof((char)'"');
            break;

        case '\r':
            token->length         = 1U;
            token->type           = CCSV_TOKEN_CARRIAGE;
            counters->chars      += sizeof((char)'\r');
            counters->max_columns = MAX(counters->max_columns, current_column_count);
            current_column_count  = 0U;
            break;

        case '\n':
            token->length         = 1U;
            token->type           = CCSV_TOKEN_NEWLINE;
            counters->chars      += sizeof((char)'\n');
            counters->max_columns = MAX(counters->max_columns, current_column_count);
            current_column_count  = 0U;
            break;

        default:
            token->length    = CCSV_Lexer_read_string(lexer);
            token->type      = CCSV_TOKEN_STRING;
            counters->chars += token->length + sizeof((char)'\0');
            counters->strings++;
        }

        lexer->position += token->length;
    }
    
    return true;
}
