#include <assert.h>

#include "ccsv.h"
#include "lexer.h"
#include "file.h"
#include "util.h"

static enum CCSV_Error CCSV_parse_next_row(struct CCSV *const csv, struct CCSV_Strings *const row) {
    assert(csv != NULL);
    assert(row != NULL);

    const char separator[] = {csv->separator, '\0'}; 

    if(csv->current_token == csv->tokens.data + csv->tokens.count) {
        return CCSV_ERROR_EMPTY;
    }

    enum CCSV_State {
        CCSV_STATE_COLUMN,
        CCSV_STATE_SEPARATOR,
        CCSV_STATE_ENCLOSURE,
        CCSV_STATE_ESCAPING,
        CCSV_STATE_CARRIAGE,
        CCSV_STATE_ESCAPING_CARRIAGE,
        CCSV_STATE_DONE
    } state = CCSV_STATE_COLUMN;

    enum CCSV_Error error = CCSV_ERROR_NONE;

    for(;state != CCSV_STATE_DONE 
        && csv->current_token != csv->tokens.data + csv->tokens.count;
        csv->current_token++
    ) {
        const struct CCSV_Token *const token = csv->current_token;

        switch(state) {
        case CCSV_STATE_COLUMN:
            switch(token->type) {
            case CCSV_TOKEN_STRING:
                if(!CCSV_Strings_push(row, token->value, token->length, &csv->arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                    break;
                }
                state = CCSV_STATE_SEPARATOR;
                break;

            case CCSV_TOKEN_SEPARATOR:
                if(!CCSV_Strings_push(row, "", (unsigned)static_strlen(""), &csv->arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                break;

            case CCSV_TOKEN_NEWLINE:
            case CCSV_TOKEN_CARRIAGE:
                state = CCSV_STATE_DONE;
                error = CCSV_ERROR_TRAILING_SEPARATOR;
                break;

            case CCSV_TOKEN_DBLQUOTE:
                state = CCSV_STATE_ENCLOSURE;
                break;
            }
            break;

        case CCSV_STATE_SEPARATOR:
            switch(token->type) {
            case CCSV_TOKEN_STRING:
                assert(false && "this should never happen unless something went wrong with the lexer");
                state = CCSV_STATE_DONE;
                error = CCSV_ERROR_LEXER;
                break;

            case CCSV_TOKEN_SEPARATOR:
                state = CCSV_STATE_COLUMN;
                break;
            
            case CCSV_TOKEN_CARRIAGE:
                state = CCSV_STATE_CARRIAGE;
                break;

            case CCSV_TOKEN_NEWLINE:
                state = CCSV_STATE_DONE;
                break;

            case CCSV_TOKEN_DBLQUOTE:
                state = CCSV_STATE_DONE;
                error = CCSV_ERROR_MISSING_DBLQUOTE;
                break;
            }
            break;

        case CCSV_STATE_ENCLOSURE:
            switch(token->type) {
            case CCSV_TOKEN_STRING:
                if(!CCSV_Strings_push(&csv->temp_strings, token->value, token->length, &csv->temp_arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                break;

            case CCSV_TOKEN_SEPARATOR:
                if(!CCSV_Strings_push(&csv->temp_strings, separator, (unsigned)static_strlen(separator), &csv->temp_arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                break;

            case CCSV_TOKEN_CARRIAGE:
                if(!CCSV_Strings_push(&csv->temp_strings, "\r", (unsigned)static_strlen("\r"), &csv->temp_arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                break;

            case CCSV_TOKEN_NEWLINE:
                if(!CCSV_Strings_push(&csv->temp_strings, "\n", (unsigned)static_strlen("\n"), &csv->temp_arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                break;

            case CCSV_TOKEN_DBLQUOTE:
                state = CCSV_STATE_ESCAPING;
                break;
            }
            break;

        case CCSV_STATE_ESCAPING:
            switch(token->type) {
            case CCSV_TOKEN_STRING:
                if(token->type != CCSV_TOKEN_DBLQUOTE) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MISSING_DBLQUOTE;
                }
                break;

            case CCSV_TOKEN_SEPARATOR:
                if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->temp_arenas))) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                CCSV_Strings_reset(&csv->temp_strings);
                state = CCSV_STATE_COLUMN;
                break;

            case CCSV_TOKEN_CARRIAGE:
                state = CCSV_STATE_ESCAPING_CARRIAGE;
                break;

            case CCSV_TOKEN_NEWLINE:
                if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->temp_arenas))) {
                    error = CCSV_ERROR_MEMORY;
                }
                CCSV_Strings_reset(&csv->temp_strings);
                state = CCSV_STATE_DONE;
                break;
            
            case CCSV_TOKEN_DBLQUOTE:
                if(!CCSV_Strings_push(&csv->temp_strings, "\"", (unsigned)static_strlen("\""), &csv->temp_arenas)) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                }
                state = CCSV_STATE_ENCLOSURE;
                break;
            }
            break;

        case CCSV_STATE_CARRIAGE:
            state = CCSV_STATE_DONE;
            if(token->type != CCSV_TOKEN_NEWLINE) {
                csv->current_token--;
            }
            break;

        case CCSV_STATE_ESCAPING_CARRIAGE:
            if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
                error = CCSV_ERROR_MEMORY;
            }
            CCSV_Strings_reset(&csv->temp_strings);
            state = CCSV_STATE_DONE;
            if(token->type != CCSV_TOKEN_NEWLINE) {
                csv->current_token--;
            }
            break;

        case CCSV_STATE_DONE:
            assert(false && "this should be unreachable");
            break;
        }

    }

    switch(state) {
    case CCSV_STATE_ESCAPING:
        if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->temp_arenas))) {
            error = CCSV_ERROR_MEMORY;
        }
        CCSV_Strings_reset(&csv->temp_strings);
        break;

    case CCSV_STATE_ENCLOSURE:
        error = CCSV_ERROR_MISSING_DBLQUOTE;
        break;

    case CCSV_STATE_COLUMN:
        error = CCSV_ERROR_TRAILING_SEPARATOR;
        break;

    default:;
    }

    return error;
}

static enum CCSV_Error CCSV_init_memory(struct CCSV *const csv) {
    assert(csv != NULL);

    bool success;
    const unsigned strings_size = CCSV_safe_unsigned_mult(csv->counters.strings, (unsigned)sizeof(char*), &success);
    if(!success) {
        return CCSV_ERROR_TOO_LARGE;
    }

    if(!CCSV_Arena_create_node(&csv->arenas.strings, strings_size)) {
        return CCSV_ERROR_MEMORY;
    }

    if(!CCSV_Arena_create_node(&csv->temp_arenas.strings, strings_size)) {
        return CCSV_ERROR_MEMORY;
    }

    if(!CCSV_Arena_create_node(&csv->arenas.chars, csv->counters.chars * sizeof(char))) {
        return CCSV_ERROR_MEMORY;
    }

    if(!CCSV_Arena_create_node(&csv->temp_arenas.chars, csv->counters.chars * sizeof(char))) {
        return CCSV_ERROR_MEMORY;
    }

    return CCSV_ERROR_NONE;
}

static enum CCSV_Error CCSV_from_string_common(struct CCSV *const csv, const char *const data, const unsigned length, const char separator) {
    assert(csv != NULL);
    assert(data != NULL);
    assert(separator != '\0');

    csv->data       = data;
    csv->length     = length;
    csv->separator  = separator;
    csv->current_token = NULL;
    CCSV_Tokens_init(&csv->tokens);
    CCSV_Strings_init(&csv->temp_strings);
    CCSV_Counters_init(&csv->counters);
    CCSV_Arena_init(&csv->arenas.strings, CCSV_ARENA_INFINITE_NODES, "CCSV Strings Arena");
    CCSV_Arena_init(&csv->temp_arenas.strings, CCSV_ARENA_INFINITE_NODES, "CCSV Temporary Strings Arena");
    CCSV_Arena_init(&csv->arenas.chars, CCSV_ARENA_INFINITE_NODES, "CCSV Chars Arena");
    CCSV_Arena_init(&csv->temp_arenas.chars, CCSV_ARENA_INFINITE_NODES, "CCSV Temporary Chars Arena");

    CCSV_Tokens_init(&csv->tokens);
    if(!CCSV_Tokens_reserve(&csv->tokens, csv->length / 2U)) {
        return CCSV_ERROR_MEMORY;
    }

    struct CCSV_Lexer lexer;
    CCSV_Lexer_init(&lexer, csv->data, csv->length, csv->separator);

    enum CCSV_Error error;
    do {
        if(!CCSV_Lexer_tokenize(&lexer, &csv->tokens, &csv->counters)) {
            error = CCSV_ERROR_MEMORY;
            break;
        }
        csv->current_token = csv->tokens.data;

        error = CCSV_init_memory(csv);
    } while(0);

    if(error != CCSV_ERROR_NONE) {
        CCSV_free(csv);
    }
    return error;
}

EXTERN_C enum CCSV_Error CCSV_from_string(struct CCSV *const csv, const char *const data, const unsigned length, const char separator) {
    assert(csv != NULL);
    assert(data != NULL);
    assert(separator != '\0');

    CCSV_FileContents_init(&csv->file_contents);
    return CCSV_from_string_common(csv, data, length, separator);
}

EXTERN_C enum CCSV_Error CCSV_from_file(struct CCSV *const csv, const char *const path, const char separator) {
    assert(csv != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');
    assert(separator != '\0');

    CCSV_FileContents_init(&csv->file_contents);
    if(CCSV_FileContents_get(&csv->file_contents, path) != CCSV_FILECONTENTS_ERROR_NONE) {
        return CCSV_ERROR_FILE;
    }

    const enum CCSV_Error error = CCSV_from_string_common(csv, (const char*)csv->file_contents.data, csv->file_contents.size, separator);
    
    return error;
}

EXTERN_C void CCSV_free(struct CCSV *const csv) {
    assert(csv != NULL);

    CCSV_FileContents_free(&csv->file_contents);
    CCSV_Tokens_free(&csv->tokens);
    CCSV_Arena_free(&csv->arenas.strings);
    CCSV_Arena_free(&csv->temp_arenas.strings);
    CCSV_Arena_free(&csv->arenas.chars);
    CCSV_Arena_free(&csv->temp_arenas.chars);
}

EXTERN_C enum CCSV_Error CCSV_next_row(struct CCSV *const csv, CCSV_Row *const row) {
    assert(csv != NULL);
    assert(row != NULL);

    CCSV_Strings_init(row);
    CCSV_Strings_reserve(row, csv->counters.max_columns, &csv->arenas);

    const enum CCSV_Error error = CCSV_parse_next_row(csv, row);

    CCSV_Strings_reset(&csv->temp_strings);
    CCSV_Arena_reset(&csv->temp_arenas.chars);
    CCSV_Arena_reset(&csv->temp_arenas.strings);

    return error;
}
