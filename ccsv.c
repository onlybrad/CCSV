#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "ccsv.h"
#include "lexer.h"
#include "file.h"
#include "util.h"

static enum CCSV_Error CCSV_parse_next_row(struct CCSV *const csv, CCSV_Row *const row) {
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
                if(!CCSV_Strings_push(row, "", (unsigned)static_strlen(""), &csv->arenas)) {
                    error = CCSV_ERROR_MEMORY;
                }
                state = CCSV_STATE_DONE;
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
                state = CCSV_STATE_DONE;
                error = CCSV_ERROR_MISSING_DBLQUOTE;
                break;

            case CCSV_TOKEN_SEPARATOR:
                if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                    break;
                }
                CCSV_Strings_reset(&csv->temp_strings);
                state = CCSV_STATE_COLUMN;
                break;

            case CCSV_TOKEN_CARRIAGE:
                state = CCSV_STATE_ESCAPING_CARRIAGE;
                break;

            case CCSV_TOKEN_NEWLINE:
                if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
                    state = CCSV_STATE_DONE;
                    error = CCSV_ERROR_MEMORY;
                    break;
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
                state = CCSV_STATE_DONE;
                error = CCSV_ERROR_MEMORY;
                break;
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
        if(!(CCSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
            error = CCSV_ERROR_MEMORY;
            break;
        }
        CCSV_Strings_reset(&csv->temp_strings);
        break;

    case CCSV_STATE_ENCLOSURE:
        error = CCSV_ERROR_MISSING_DBLQUOTE;
        break;

    default:;
    }

    return error;
}

static enum CCSV_Error CCSV_init_memory(struct CCSV *const csv) {
    assert(csv != NULL);

    bool success;
    const size_t strings_size = CCSV_safe_mult(csv->counters.strings, sizeof(char*), &success);
    if(!success) {
        return CCSV_ERROR_TOO_LARGE;
    }

    if(!CCSV_Arena_create_node(&csv->arenas.strings, strings_size)) {
        return CCSV_ERROR_MEMORY;
    }

    if(!CCSV_Arena_create_node(&csv->temp_arenas.strings, strings_size)) {
        return CCSV_ERROR_MEMORY;
    }

    if(!CCSV_Arena_create_node(&csv->arenas.chars, csv->counters.chars * (unsigned)sizeof(char))) {
        return CCSV_ERROR_MEMORY;
    }

    if(!CCSV_Arena_create_node(&csv->temp_arenas.chars, csv->counters.chars * (unsigned)sizeof(char))) {
        return CCSV_ERROR_MEMORY;
    }

    return CCSV_ERROR_NONE;
}

static enum CCSV_Error CCSV_from_string_common(struct CCSV *const csv, const char *const data, const int64_t length, const char separator) {
    assert(csv != NULL);
    assert(data != NULL);
    assert(separator != '\0');
    assert(length > 0);

    csv->data          = data;
    csv->length        = length;
    csv->separator     = separator;
    csv->current_token = NULL;
    CCSV_Tokens_init(&csv->tokens);
    CCSV_Strings_init(&csv->temp_strings);
    CCSV_Counters_init(&csv->counters);
    CCSV_Arena_init(&csv->arenas.strings, CCSV_ARENA_INFINITE_NODES, "CCSV Strings Arena");
    CCSV_Arena_init(&csv->temp_arenas.strings, CCSV_ARENA_INFINITE_NODES, "CCSV Temporary Strings Arena");
    CCSV_Arena_init(&csv->arenas.chars, CCSV_ARENA_INFINITE_NODES, "CCSV Chars Arena");
    CCSV_Arena_init(&csv->temp_arenas.chars, CCSV_ARENA_INFINITE_NODES, "CCSV Temporary Chars Arena");

    CCSV_Tokens_init(&csv->tokens);
    if(!CCSV_Tokens_reserve(&csv->tokens, (size_t)csv->length / 2)) {
        return CCSV_ERROR_MEMORY;
    }

    struct CCSV_Lexer lexer;
    CCSV_Lexer_init(&lexer, csv->data, (size_t)csv->length, csv->separator);

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

static size_t CCSV_Type_get_size(const enum CCSV_Type type) {
    switch(type) {
    case CCSV_TYPE_CHAR:
    case CCSV_TYPE_UCHAR:
    case CCSV_TYPE_SCHAR:
    case CCSV_TYPE_UINT8:
    case CCSV_TYPE_INT8:
        return sizeof(char);

    case CCSV_TYPE_SHORT:
    case CCSV_TYPE_USHORT:
        return sizeof(short);

    case CCSV_TYPE_INT:
    case CCSV_TYPE_UINT:
        return sizeof(int);

    case CCSV_TYPE_LONG:
    case CCSV_TYPE_ULONG:
        return sizeof(long);

    case CCSV_TYPE_LLONG:
    case CCSV_TYPE_ULLONG:
        return sizeof(long long);

    case CCSV_TYPE_FLOAT:
        return sizeof(float);

    case CCSV_TYPE_DOUBLE:
        return sizeof(double);

    case CCSV_TYPE_LDOUBLE:
        return sizeof(long double);

    case CCSV_TYPE_INT16:
    case CCSV_TYPE_UINT16:
        return sizeof(int16_t);

    case CCSV_TYPE_INT32:
    case CCSV_TYPE_UINT32:
        return sizeof(int32_t);

    case CCSV_TYPE_INT64:
    case CCSV_TYPE_UINT64:
        return sizeof(int64_t);

    case CCSV_TYPE_INTMAX:
    case CCSV_TYPE_UINTMAX:
        return sizeof(intmax_t);

    case CCSV_TYPE_SIZE:
        return sizeof(size_t);

    case CCSV_TYPE_STRING:
    case CCSV_TYPE_BOOL:
        assert(false && "this should be unreachable");
        break;
    }

    return 0;
}

static size_t CCSV_get_digit_size(const size_t size) {
    assert(size == 1 || size == 2 || size == 4 || size == 8 || size == 16);
    
    switch(size) {
    case 1:
        return 3 + sizeof((char)'-');
    case 2:
        return 5 + sizeof((char)'-');
    case 4:
        return 10 + sizeof((char)'-');
    case 8:
        return 20 + sizeof((char)'-');
    case 16:
        return 39 + sizeof((char)'-');
    }
    return 0;
}

static size_t CCSV_count_characters(const unsigned char *const read_ptr, bool *const needs_escape, const char separator) {
    assert(read_ptr != NULL);
    assert(needs_escape != NULL);
    assert(separator != '\0');

    char *string, *string_start;
    memcpy(&string_start, read_ptr, sizeof(string_start));
    assert(string_start != NULL);

    *needs_escape = false;
    size_t extra_dblquote = 0;
    for(string = string_start; *string != '\0'; string++) {
        if(*string == separator || *string == '\r' || *string == '\n') {
            *needs_escape = true;
        }

        if(*string == '"') {
            *needs_escape = true;
            extra_dblquote++;
        }
    }

    size_t count = (size_t)(string - string_start);
    if(*needs_escape) {
        count += sizeof((char)'"') + extra_dblquote + sizeof((char)'"');
    }

    return count;
}

static size_t CCSV_write_string(char *write_ptr, const unsigned char *const read_ptr, const bool needs_escape) {
    assert(write_ptr != NULL);
    assert(read_ptr != NULL);

    const char *string;
    memcpy(&string, read_ptr, sizeof(string));
    assert(string != NULL);
    if(needs_escape) {
        const char *const write_ptr_start = write_ptr;
        *(write_ptr++) = '"';
        for(;*string != '\0'; string++, write_ptr++) {
            if(*string == '"') {
                *(write_ptr++) = '"';
            }
            *write_ptr = *string;
        }
        *(write_ptr++) = '"';
        return (size_t)(write_ptr - write_ptr_start);
    }

    strcpy(write_ptr, string);
    return strlen(string);
}

static size_t CCSV_write_number(char *const write_ptr, const unsigned char *const read_ptr, const enum CCSV_Type type) {
    assert(write_ptr != NULL);
    assert(read_ptr != NULL);
    assert(type != CCSV_TYPE_STRING);
    assert(type != CCSV_TYPE_BOOL);
    
    union CCSV_Value {
        char               c;
        unsigned char      uc;
        signed char        sc;
        short              s;
        unsigned short     us;
        int                i;
        unsigned           u;
        long               l;
        unsigned long      ul;
        long long          ll;
        unsigned long long ull;
        float              f;
        double             d;
        long double        ld;
        int8_t             i8;
        uint8_t            u8;
        int16_t            i16;
        uint16_t           u16;
        int32_t            i32;
        uint32_t           u32;
        int64_t            i64;
        uint64_t           u64;
        intmax_t           imax;
        uintmax_t          umax;
        size_t             size;
    } value;

    switch(type) {
    case CCSV_TYPE_CHAR:
        memcpy(&value.c, read_ptr, sizeof(char));
        return (size_t)sprintf(write_ptr, "%c", value.c);
    case CCSV_TYPE_UCHAR:
        memcpy(&value.uc, read_ptr, sizeof(unsigned char));
        return (size_t)sprintf(write_ptr, "%u", value.uc);
    case CCSV_TYPE_SCHAR:
        memcpy(&value.sc, read_ptr, sizeof(signed char));
        return (size_t)sprintf(write_ptr, "%i", value.sc);
    case CCSV_TYPE_SHORT:
        memcpy(&value.s, read_ptr, sizeof(short));
        return (size_t)sprintf(write_ptr, "%hi", value.s);
    case CCSV_TYPE_USHORT:
        memcpy(&value.us, read_ptr, sizeof(unsigned short));
        return (size_t)sprintf(write_ptr, "%hu", value.us);
    case CCSV_TYPE_INT:
        memcpy(&value.i, read_ptr, sizeof(int));
        return (size_t)sprintf(write_ptr, "%i", value.i);
    case CCSV_TYPE_UINT:
        memcpy(&value.u, read_ptr, sizeof(unsigned));
        return (size_t)sprintf(write_ptr, "%u", value.u);
    case CCSV_TYPE_LONG:
        memcpy(&value.l, read_ptr, sizeof(long));
        return (size_t)sprintf(write_ptr, "%li", value.l);
    case CCSV_TYPE_ULONG:
        memcpy(&value.ul, read_ptr, sizeof(unsigned long));
        return (size_t)sprintf(write_ptr, "%lu", value.ul);
    case CCSV_TYPE_LLONG:
        memcpy(&value.ll, read_ptr, sizeof(long long));
        return (size_t)sprintf(write_ptr, "%lli", value.ll);
    case CCSV_TYPE_ULLONG:
        memcpy(&value.ull, read_ptr, sizeof(unsigned long long));
        return (size_t)sprintf(write_ptr, "%llu", value.ull);
    case CCSV_TYPE_FLOAT:
        memcpy(&value.f, read_ptr, sizeof(float));
        return (size_t)sprintf(write_ptr, "%f", value.f);
    case CCSV_TYPE_DOUBLE:
        memcpy(&value.d, read_ptr, sizeof(double));
        return (size_t)sprintf(write_ptr, "%lf", value.d);
    case CCSV_TYPE_LDOUBLE:
        memcpy(&value.ld, read_ptr, sizeof(long double));
        return (size_t)sprintf(write_ptr, "%Lf", value.ld);
    case CCSV_TYPE_INT8:
        memcpy(&value.i8, read_ptr, sizeof(int8_t));
        return (size_t)sprintf(write_ptr, "%"PRIi8, value.i8);
    case CCSV_TYPE_UINT8:
        memcpy(&value.u8, read_ptr, sizeof(uint8_t));
        return (size_t)sprintf(write_ptr, "%"PRIu8, value.u8);
    case CCSV_TYPE_INT16:
        memcpy(&value.i16, read_ptr, sizeof(int16_t));
        return (size_t)sprintf(write_ptr, "%"PRIi16, value.i16);
    case CCSV_TYPE_UINT16:
        memcpy(&value.u16, read_ptr, sizeof(uint16_t));
        return (size_t)sprintf(write_ptr, "%"PRIu16, value.u16);
    case CCSV_TYPE_INT32:
        memcpy(&value.i32, read_ptr, sizeof(int32_t));
        return (size_t)sprintf(write_ptr, "%"PRIi32, value.i32);
    case CCSV_TYPE_UINT32:
        memcpy(&value.u32, read_ptr, sizeof(uint32_t));
        return (size_t)sprintf(write_ptr, "%"PRIu32, value.u32);
    case CCSV_TYPE_INT64:
        memcpy(&value.i64, read_ptr, sizeof(int64_t));
        return (size_t)sprintf(write_ptr, "%"PRIi64, value.i64);
    case CCSV_TYPE_UINT64:
        memcpy(&value.u64, read_ptr, sizeof(uint64_t));
        return (size_t)sprintf(write_ptr, "%"PRIu64, value.u64);
    case CCSV_TYPE_INTMAX:
        memcpy(&value.imax, read_ptr, sizeof(intmax_t));
        return (size_t)sprintf(write_ptr, "%ji", value.imax);
    case CCSV_TYPE_UINTMAX:
        memcpy(&value.umax, read_ptr, sizeof(uintmax_t));
        return (size_t)sprintf(write_ptr, "%ju", value.umax);
    case CCSV_TYPE_SIZE:
        memcpy(&value.size, read_ptr, sizeof(size_t));
        return (size_t)sprintf(write_ptr, "%zu", value.size);
    case CCSV_TYPE_STRING:
    case CCSV_TYPE_BOOL:
        assert(false && "this should be unreachable");
        break;
    }

    return 0;
}

static size_t CCSV_write_bool(char *const write_ptr, const unsigned char *const read_ptr) {
    assert(write_ptr != NULL);
    assert(read_ptr != NULL);

    bool value;
    memcpy(&value, read_ptr, sizeof(value));
    
    const char *data;
    size_t length;
    if(value) {
        data = "true";
        length = static_strlen("true");
    } else {
        data = "false";
        length = static_strlen("false");
    }

    memcpy(write_ptr, data, length);

    return length;
}

static size_t CCSV_Struct_overestimate_size(const struct CCSV_Structs *const structs, bool *needs_escape, const char separator) {
    assert(structs != NULL);
    assert(structs->data != NULL);
    assert(structs->members != NULL);
    assert(structs->member_count > 0);
    assert(needs_escape != NULL);
    assert(separator != '\0');

    size_t total_size = 0;
    const unsigned char *struct_ptr = (const unsigned char*)structs->data;

    for(size_t i = 0; i < structs->count; i++, struct_ptr += structs->size) {
        for(const struct CCSV_StructMember *member = structs->members;
            member != structs->members + structs->member_count;
            member++, needs_escape++
        ) {
            switch(member->type) {
            case CCSV_TYPE_STRING:
                total_size += CCSV_count_characters(struct_ptr + member->offset, needs_escape, separator);
                break;
            
            case CCSV_TYPE_BOOL:
                total_size += static_strlen("false");
                break;
            
            default:
                total_size += CCSV_get_digit_size(CCSV_Type_get_size(member->type));
            }

            total_size += sizeof(separator);
        }
    }

    return total_size;
}

static char *CCSV_Struct_write_data(const struct CCSV_Structs *const structs, char *write_ptr, bool *needs_escape, const char separator) {
    assert(structs != NULL);
    assert(structs->data != NULL);
    assert(structs->members != NULL);
    assert(structs->member_count > 0);
    assert(needs_escape != NULL);
    assert(separator != '\0');

    const unsigned char *struct_ptr = (const unsigned char*)structs->data;
    for(size_t i = 0; i < structs->count; i++, struct_ptr += structs->size) {
        for(const struct CCSV_StructMember *member = structs->members;
            member != structs->members + structs->member_count;
            member++, needs_escape++
        ) {
            const unsigned char *const member_ptr = struct_ptr + member->offset;
            switch(member->type) {
            case CCSV_TYPE_STRING:
                write_ptr += CCSV_write_string(write_ptr, member_ptr, *needs_escape);
                break;

            case CCSV_TYPE_BOOL:
                write_ptr += CCSV_write_bool(write_ptr, member_ptr);
                break;
                
            default:
                write_ptr += CCSV_write_number(write_ptr, member_ptr, member->type);
            }
            *(write_ptr++) = separator;
        }
        *(write_ptr - 1) = '\n';
    }

    return write_ptr;
}

EXTERN_C enum CCSV_Error CCSV_from_string(struct CCSV *const csv, const char *const data, const int64_t length, const char separator) {
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

EXTERN_C bool CCSV_to_file(const struct CCSV_Structs *const headers, const struct CCSV_Structs *const structs, const char *const path, const char separator) {
    assert(headers != NULL);
    assert(structs != NULL);
    assert(path != NULL);
    assert(separator != '\0');
    assert(headers->count == 1);

    bool success;
    const size_t headers_bools_required = CCSV_safe_mult(headers->member_count, headers->count, &success);
    if(!success) {
        return false;
    }

    const size_t structs_bools_required = CCSV_safe_mult(structs->member_count, structs->count, &success);
    if(!success) {
        return false;
    }

    if(headers_bools_required > SIZE_MAX - structs_bools_required) {
        return false;
    }
    const size_t bools_required = headers_bools_required + structs_bools_required;

    bool *const needs_escape = (bool*)CCSV_CALLOC(bools_required, sizeof(bool));
    if(needs_escape == NULL) {
        return false;
    }

    bool *const headers_needs_escape = needs_escape,
         *const structs_needs_escape = needs_escape + headers_bools_required;

    const size_t buffer_size = CCSV_Struct_overestimate_size(headers, headers_needs_escape, separator) + CCSV_Struct_overestimate_size(structs, structs_needs_escape, separator);

    if ((uintmax_t)buffer_size >= MIN((uintmax_t)INT64_MAX, (uintmax_t)SIZE_MAX) - 1) {
        CCSV_FREE(needs_escape);
        return false;
    }

    char *const csv_data = (char*)CCSV_MALLOC((buffer_size + sizeof((char)'\0')) * sizeof(char));
    if(csv_data == NULL) {
        CCSV_FREE(needs_escape);
        return false;
    }
    char *write_ptr = csv_data;

    if(buffer_size == 0) {
        *write_ptr = '\0';
    } else {
        write_ptr = CCSV_Struct_write_data(headers, write_ptr, headers_needs_escape, separator);
        write_ptr = CCSV_Struct_write_data(structs, write_ptr, structs_needs_escape, separator);
        *(write_ptr - 1) = '\0';
    }

    struct CCSV_FileContents file_contents;
    file_contents.data = (unsigned char*)csv_data;

    if(buffer_size == 0) {
        file_contents.size = 0;
    } else {
        file_contents.size = (int64_t)(write_ptr - csv_data - 1);
    }
    
    success = CCSV_FileContents_put(&file_contents, path) == CCSV_FILECONTENTS_ERROR_NONE;

    CCSV_FREE(csv_data);
    CCSV_FREE(needs_escape);

    return success;
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
