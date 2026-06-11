#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "obcsv.h"
#include "lexer.h"
#include "file.h"
#include "util.h"

static enum OB_CSV_Error OB_CSV_parse_next_row(struct CCSV *const csv, OB_CSV_Row *const row) {
    assert(csv != NULL);
    assert(row != NULL);

    const char separator[] = {csv->separator, '\0'}; 

    if(csv->current_token == csv->tokens.data + csv->tokens.count) {
        return OB_CSV_ERROR_EMPTY;
    }

    enum OB_CSV_State {
        OB_CSV_STATE_COLUMN,
        OB_CSV_STATE_SEPARATOR,
        OB_CSV_STATE_ENCLOSURE,
        OB_CSV_STATE_ESCAPING,
        OB_CSV_STATE_CARRIAGE,
        OB_CSV_STATE_ESCAPING_CARRIAGE,
        OB_CSV_STATE_DONE
    } state = OB_CSV_STATE_COLUMN;

    enum OB_CSV_Error error = OB_CSV_ERROR_NONE;

    for(;state != OB_CSV_STATE_DONE 
        && csv->current_token != csv->tokens.data + csv->tokens.count;
        csv->current_token++
    ) {
        const struct OB_CSV_Token *const token = csv->current_token;

        switch(state) {
        case OB_CSV_STATE_COLUMN:
            switch(token->type) {
            case OB_CSV_TOKEN_STRING:
                if(!OB_CSV_Strings_push(row, token->value, token->length, &csv->arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                    break;
                }
                state = OB_CSV_STATE_SEPARATOR;
                break;

            case OB_CSV_TOKEN_SEPARATOR:
                if(!OB_CSV_Strings_push(row, "", (unsigned)static_strlen(""), &csv->arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                }
                break;

            case OB_CSV_TOKEN_NEWLINE:
            case OB_CSV_TOKEN_CARRIAGE:
                if(!OB_CSV_Strings_push(row, "", (unsigned)static_strlen(""), &csv->arenas)) {
                    error = OB_CSV_ERROR_MEMORY;
                }
                state = OB_CSV_STATE_DONE;
                break;

            case OB_CSV_TOKEN_DBLQUOTE:
                state = OB_CSV_STATE_ENCLOSURE;
                break;
            }
            break;

        case OB_CSV_STATE_SEPARATOR:
            switch(token->type) {
            case OB_CSV_TOKEN_STRING:
                assert(false && "this should never happen unless something went wrong with the lexer");
                state = OB_CSV_STATE_DONE;
                error = OB_CSV_ERROR_LEXER;
                break;

            case OB_CSV_TOKEN_SEPARATOR:
                state = OB_CSV_STATE_COLUMN;
                break;
            
            case OB_CSV_TOKEN_CARRIAGE:
                state = OB_CSV_STATE_CARRIAGE;
                break;

            case OB_CSV_TOKEN_NEWLINE:
                state = OB_CSV_STATE_DONE;
                break;

            case OB_CSV_TOKEN_DBLQUOTE:
                state = OB_CSV_STATE_DONE;
                error = OB_CSV_ERROR_MISSING_DBLQUOTE;
                break;
            }
            break;

        case OB_CSV_STATE_ENCLOSURE:
            switch(token->type) {
            case OB_CSV_TOKEN_STRING:
                if(!OB_CSV_Strings_push(&csv->temp_strings, token->value, token->length, &csv->temp_arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                }
                break;

            case OB_CSV_TOKEN_SEPARATOR:
                if(!OB_CSV_Strings_push(&csv->temp_strings, separator, (unsigned)static_strlen(separator), &csv->temp_arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                }
                break;

            case OB_CSV_TOKEN_CARRIAGE:
                if(!OB_CSV_Strings_push(&csv->temp_strings, "\r", (unsigned)static_strlen("\r"), &csv->temp_arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                }
                break;

            case OB_CSV_TOKEN_NEWLINE:
                if(!OB_CSV_Strings_push(&csv->temp_strings, "\n", (unsigned)static_strlen("\n"), &csv->temp_arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                }
                break;

            case OB_CSV_TOKEN_DBLQUOTE:
                state = OB_CSV_STATE_ESCAPING;
                break;
            }
            break;

        case OB_CSV_STATE_ESCAPING:
            switch(token->type) {
            case OB_CSV_TOKEN_STRING:
                state = OB_CSV_STATE_DONE;
                error = OB_CSV_ERROR_MISSING_DBLQUOTE;
                break;

            case OB_CSV_TOKEN_SEPARATOR:
                if(!(OB_CSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                    break;
                }
                OB_CSV_Strings_reset(&csv->temp_strings);
                state = OB_CSV_STATE_COLUMN;
                break;

            case OB_CSV_TOKEN_CARRIAGE:
                state = OB_CSV_STATE_ESCAPING_CARRIAGE;
                break;

            case OB_CSV_TOKEN_NEWLINE:
                if(!(OB_CSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                    break;
                }
                OB_CSV_Strings_reset(&csv->temp_strings);
                state = OB_CSV_STATE_DONE;
                break;
            
            case OB_CSV_TOKEN_DBLQUOTE:
                if(!OB_CSV_Strings_push(&csv->temp_strings, "\"", (unsigned)static_strlen("\""), &csv->temp_arenas)) {
                    state = OB_CSV_STATE_DONE;
                    error = OB_CSV_ERROR_MEMORY;
                }
                state = OB_CSV_STATE_ENCLOSURE;
                break;
            }
            break;

        case OB_CSV_STATE_CARRIAGE:
            state = OB_CSV_STATE_DONE;
            if(token->type != OB_CSV_TOKEN_NEWLINE) {
                csv->current_token--;
            }
            break;

        case OB_CSV_STATE_ESCAPING_CARRIAGE:
            if(!(OB_CSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
                state = OB_CSV_STATE_DONE;
                error = OB_CSV_ERROR_MEMORY;
                break;
            }
            OB_CSV_Strings_reset(&csv->temp_strings);
            state = OB_CSV_STATE_DONE;
            if(token->type != OB_CSV_TOKEN_NEWLINE) {
                csv->current_token--;
            }
            break;

        case OB_CSV_STATE_DONE:
            assert(false && "this should be unreachable");
            break;
        }
    }

    switch(state) {
    case OB_CSV_STATE_ESCAPING:
        if(!(OB_CSV_Strings_concat(&csv->temp_strings, row, &csv->arenas))) {
            error = OB_CSV_ERROR_MEMORY;
            break;
        }
        OB_CSV_Strings_reset(&csv->temp_strings);
        break;

    case OB_CSV_STATE_ENCLOSURE:
        error = OB_CSV_ERROR_MISSING_DBLQUOTE;
        break;

    default:;
    }

    return error;
}

static enum OB_CSV_Error OB_CSV_init_memory(struct CCSV *const csv) {
    assert(csv != NULL);

    bool success;
    const size_t strings_size = OB_CSV_safe_mult(csv->counters.strings, sizeof(char*), &success);
    if(!success) {
        return OB_CSV_ERROR_TOO_LARGE;
    }

    if(!OB_CSV_Arena_create_node(&csv->arenas.strings, strings_size)) {
        return OB_CSV_ERROR_MEMORY;
    }

    if(!OB_CSV_Arena_create_node(&csv->temp_arenas.strings, strings_size)) {
        return OB_CSV_ERROR_MEMORY;
    }

    if(!OB_CSV_Arena_create_node(&csv->arenas.chars, csv->counters.chars * (unsigned)sizeof(char))) {
        return OB_CSV_ERROR_MEMORY;
    }

    if(!OB_CSV_Arena_create_node(&csv->temp_arenas.chars, csv->counters.chars * (unsigned)sizeof(char))) {
        return OB_CSV_ERROR_MEMORY;
    }

    return OB_CSV_ERROR_NONE;
}

static enum OB_CSV_Error OB_CSV_from_string_common(struct CCSV *const csv, const char *const data, const int64_t length, const char separator) {
    assert(csv != NULL);
    assert(data != NULL);
    assert(separator != '\0');
    assert(length > 0);

    csv->data          = data;
    csv->length        = length;
    csv->separator     = separator;
    csv->current_token = NULL;
    OB_CSV_Tokens_init(&csv->tokens);
    OB_CSV_Strings_init(&csv->temp_strings);
    OB_CSV_Counters_init(&csv->counters);
    OB_CSV_Arena_init(&csv->arenas.strings, OB_CSV_ARENA_INFINITE_NODES, "CCSV Strings Arena");
    OB_CSV_Arena_init(&csv->temp_arenas.strings, OB_CSV_ARENA_INFINITE_NODES, "CCSV Temporary Strings Arena");
    OB_CSV_Arena_init(&csv->arenas.chars, OB_CSV_ARENA_INFINITE_NODES, "CCSV Chars Arena");
    OB_CSV_Arena_init(&csv->temp_arenas.chars, OB_CSV_ARENA_INFINITE_NODES, "CCSV Temporary Chars Arena");

    OB_CSV_Tokens_init(&csv->tokens);
    if(!OB_CSV_Tokens_reserve(&csv->tokens, (size_t)csv->length / 2)) {
        return OB_CSV_ERROR_MEMORY;
    }

    struct OB_CSV_Lexer lexer;
    OB_CSV_Lexer_init(&lexer, csv->data, (size_t)csv->length, csv->separator);

    enum OB_CSV_Error error;
    do {
        if(!OB_CSV_Lexer_tokenize(&lexer, &csv->tokens, &csv->counters)) {
            error = OB_CSV_ERROR_MEMORY;
            break;
        }
        csv->current_token = csv->tokens.data;

        error = OB_CSV_init_memory(csv);
    } while(0);

    if(error != OB_CSV_ERROR_NONE) {
        OB_CSV_free(csv);
    }
    return error;
}

static size_t OB_CSV_Type_get_size(const enum OB_CSV_Type type) {
    switch(type) {
    case OB_CSV_TYPE_CHAR:
    case OB_CSV_TYPE_UCHAR:
    case OB_CSV_TYPE_SCHAR:
    case OB_CSV_TYPE_UINT8:
    case OB_CSV_TYPE_INT8:
        return sizeof(char);

    case OB_CSV_TYPE_SHORT:
    case OB_CSV_TYPE_USHORT:
        return sizeof(short);

    case OB_CSV_TYPE_INT:
    case OB_CSV_TYPE_UINT:
        return sizeof(int);

    case OB_CSV_TYPE_LONG:
    case OB_CSV_TYPE_ULONG:
        return sizeof(long);

    case OB_CSV_TYPE_LLONG:
    case OB_CSV_TYPE_ULLONG:
        return sizeof(long long);

    case OB_CSV_TYPE_FLOAT:
        return sizeof(float);

    case OB_CSV_TYPE_DOUBLE:
        return sizeof(double);

    case OB_CSV_TYPE_LDOUBLE:
        return sizeof(long double);

    case OB_CSV_TYPE_INT16:
    case OB_CSV_TYPE_UINT16:
        return sizeof(int16_t);

    case OB_CSV_TYPE_INT32:
    case OB_CSV_TYPE_UINT32:
        return sizeof(int32_t);

    case OB_CSV_TYPE_INT64:
    case OB_CSV_TYPE_UINT64:
        return sizeof(int64_t);

    case OB_CSV_TYPE_INTMAX:
    case OB_CSV_TYPE_UINTMAX:
        return sizeof(intmax_t);

    case OB_CSV_TYPE_SIZE:
        return sizeof(size_t);

    case OB_CSV_TYPE_STRING:
    case OB_CSV_TYPE_BOOL:
        assert(false && "this should be unreachable");
        break;
    }

    return 0;
}

static size_t OB_CSV_get_digit_size(const size_t size) {
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

static size_t OB_CSV_count_characters(const unsigned char *const read_ptr, bool *const needs_escape, const char separator) {
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

static size_t OB_CSV_write_string(char *write_ptr, const unsigned char *const read_ptr, const bool needs_escape) {
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

static size_t OB_CSV_write_number(char *const write_ptr, const unsigned char *const read_ptr, const enum OB_CSV_Type type) {
    assert(write_ptr != NULL);
    assert(read_ptr != NULL);
    assert(type != OB_CSV_TYPE_STRING);
    assert(type != OB_CSV_TYPE_BOOL);
    
    union OB_CSV_Value {
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
    case OB_CSV_TYPE_CHAR:
        memcpy(&value.c, read_ptr, sizeof(char));
        return (size_t)sprintf(write_ptr, "%c", value.c);
    case OB_CSV_TYPE_UCHAR:
        memcpy(&value.uc, read_ptr, sizeof(unsigned char));
        return (size_t)sprintf(write_ptr, "%u", value.uc);
    case OB_CSV_TYPE_SCHAR:
        memcpy(&value.sc, read_ptr, sizeof(signed char));
        return (size_t)sprintf(write_ptr, "%i", value.sc);
    case OB_CSV_TYPE_SHORT:
        memcpy(&value.s, read_ptr, sizeof(short));
        return (size_t)sprintf(write_ptr, "%hi", value.s);
    case OB_CSV_TYPE_USHORT:
        memcpy(&value.us, read_ptr, sizeof(unsigned short));
        return (size_t)sprintf(write_ptr, "%hu", value.us);
    case OB_CSV_TYPE_INT:
        memcpy(&value.i, read_ptr, sizeof(int));
        return (size_t)sprintf(write_ptr, "%i", value.i);
    case OB_CSV_TYPE_UINT:
        memcpy(&value.u, read_ptr, sizeof(unsigned));
        return (size_t)sprintf(write_ptr, "%u", value.u);
    case OB_CSV_TYPE_LONG:
        memcpy(&value.l, read_ptr, sizeof(long));
        return (size_t)sprintf(write_ptr, "%li", value.l);
    case OB_CSV_TYPE_ULONG:
        memcpy(&value.ul, read_ptr, sizeof(unsigned long));
        return (size_t)sprintf(write_ptr, "%lu", value.ul);
    case OB_CSV_TYPE_LLONG:
        memcpy(&value.ll, read_ptr, sizeof(long long));
        return (size_t)sprintf(write_ptr, "%lli", value.ll);
    case OB_CSV_TYPE_ULLONG:
        memcpy(&value.ull, read_ptr, sizeof(unsigned long long));
        return (size_t)sprintf(write_ptr, "%llu", value.ull);
    case OB_CSV_TYPE_FLOAT:
        memcpy(&value.f, read_ptr, sizeof(float));
        return (size_t)sprintf(write_ptr, "%f", value.f);
    case OB_CSV_TYPE_DOUBLE:
        memcpy(&value.d, read_ptr, sizeof(double));
        return (size_t)sprintf(write_ptr, "%lf", value.d);
    case OB_CSV_TYPE_LDOUBLE:
        memcpy(&value.ld, read_ptr, sizeof(long double));
        return (size_t)sprintf(write_ptr, "%Lf", value.ld);
    case OB_CSV_TYPE_INT8:
        memcpy(&value.i8, read_ptr, sizeof(int8_t));
        return (size_t)sprintf(write_ptr, "%" PRIi8, value.i8);
    case OB_CSV_TYPE_UINT8:
        memcpy(&value.u8, read_ptr, sizeof(uint8_t));
        return (size_t)sprintf(write_ptr, "%" PRIu8, value.u8);
    case OB_CSV_TYPE_INT16:
        memcpy(&value.i16, read_ptr, sizeof(int16_t));
        return (size_t)sprintf(write_ptr, "%" PRIi16, value.i16);
    case OB_CSV_TYPE_UINT16:
        memcpy(&value.u16, read_ptr, sizeof(uint16_t));
        return (size_t)sprintf(write_ptr, "%" PRIu16, value.u16);
    case OB_CSV_TYPE_INT32:
        memcpy(&value.i32, read_ptr, sizeof(int32_t));
        return (size_t)sprintf(write_ptr, "%" PRIi32, value.i32);
    case OB_CSV_TYPE_UINT32:
        memcpy(&value.u32, read_ptr, sizeof(uint32_t));
        return (size_t)sprintf(write_ptr, "%" PRIu32, value.u32);
    case OB_CSV_TYPE_INT64:
        memcpy(&value.i64, read_ptr, sizeof(int64_t));
        return (size_t)sprintf(write_ptr, "%" PRIi64, value.i64);
    case OB_CSV_TYPE_UINT64:
        memcpy(&value.u64, read_ptr, sizeof(uint64_t));
        return (size_t)sprintf(write_ptr, "%" PRIu64, value.u64);
    case OB_CSV_TYPE_INTMAX:
        memcpy(&value.imax, read_ptr, sizeof(intmax_t));
        return (size_t)sprintf(write_ptr, "%ji", value.imax);
    case OB_CSV_TYPE_UINTMAX:
        memcpy(&value.umax, read_ptr, sizeof(uintmax_t));
        return (size_t)sprintf(write_ptr, "%ju", value.umax);
    case OB_CSV_TYPE_SIZE:
        memcpy(&value.size, read_ptr, sizeof(size_t));
        return (size_t)sprintf(write_ptr, "%zu", value.size);
    case OB_CSV_TYPE_STRING:
    case OB_CSV_TYPE_BOOL:
        assert(false && "this should be unreachable");
        break;
    }

    return 0;
}

static size_t OB_CSV_write_bool(char *const write_ptr, const unsigned char *const read_ptr) {
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

static size_t OB_CSV_Struct_overestimate_size(const struct OB_CSV_Structs *const structs, bool *needs_escape, const char separator) {
    assert(structs != NULL);
    assert(structs->data != NULL);
    assert(structs->members != NULL);
    assert(structs->member_count > 0);
    assert(needs_escape != NULL);
    assert(separator != '\0');

    size_t total_size = 0;
    const unsigned char *struct_ptr = (const unsigned char*)structs->data;

    for(size_t i = 0; i < structs->count; i++, struct_ptr += structs->size) {
        for(const struct OB_CSV_StructMember *member = structs->members;
            member != structs->members + structs->member_count;
            member++, needs_escape++
        ) {
            switch(member->type) {
            case OB_CSV_TYPE_STRING:
                total_size += OB_CSV_count_characters(struct_ptr + member->offset, needs_escape, separator);
                break;
            
            case OB_CSV_TYPE_BOOL:
                total_size += static_strlen("false");
                break;
            
            default:
                total_size += OB_CSV_get_digit_size(OB_CSV_Type_get_size(member->type));
            }

            total_size += sizeof(separator);
        }
    }

    return total_size;
}

static char *OB_CSV_Struct_write_data(const struct OB_CSV_Structs *const structs, char *write_ptr, bool *needs_escape, const char separator) {
    assert(structs != NULL);
    assert(structs->data != NULL);
    assert(structs->members != NULL);
    assert(structs->member_count > 0);
    assert(needs_escape != NULL);
    assert(separator != '\0');

    const unsigned char *struct_ptr = (const unsigned char*)structs->data;
    for(size_t i = 0; i < structs->count; i++, struct_ptr += structs->size) {
        for(const struct OB_CSV_StructMember *member = structs->members;
            member != structs->members + structs->member_count;
            member++, needs_escape++
        ) {
            const unsigned char *const member_ptr = struct_ptr + member->offset;
            switch(member->type) {
            case OB_CSV_TYPE_STRING:
                write_ptr += OB_CSV_write_string(write_ptr, member_ptr, *needs_escape);
                break;

            case OB_CSV_TYPE_BOOL:
                write_ptr += OB_CSV_write_bool(write_ptr, member_ptr);
                break;
                
            default:
                write_ptr += OB_CSV_write_number(write_ptr, member_ptr, member->type);
            }
            *(write_ptr++) = separator;
        }
        *(write_ptr - 1) = '\n';
    }

    return write_ptr;
}

EXTERN_C enum OB_CSV_Error OB_CSV_from_string(struct CCSV *const csv, const char *const data, const int64_t length, const char separator) {
    assert(csv != NULL);
    assert(data != NULL);
    assert(separator != '\0');

    OB_CSV_FileContents_init(&csv->file_contents);
    return OB_CSV_from_string_common(csv, data, length, separator);
}

EXTERN_C enum OB_CSV_Error OB_CSV_from_file(struct CCSV *const csv, const char *const path, const char separator) {
    assert(csv != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');
    assert(separator != '\0');

    OB_CSV_FileContents_init(&csv->file_contents);
    if(OB_CSV_FileContents_get(&csv->file_contents, path) != OB_CSV_FILECONTENTS_ERROR_NONE) {
        return OB_CSV_ERROR_FILE;
    }

    const enum OB_CSV_Error error = OB_CSV_from_string_common(csv, (const char*)csv->file_contents.data, csv->file_contents.size, separator);
    
    return error;
}

EXTERN_C void OB_CSV_free(struct CCSV *const csv) {
    assert(csv != NULL);

    OB_CSV_FileContents_free(&csv->file_contents);
    OB_CSV_Tokens_free(&csv->tokens);
    OB_CSV_Arena_free(&csv->arenas.strings);
    OB_CSV_Arena_free(&csv->temp_arenas.strings);
    OB_CSV_Arena_free(&csv->arenas.chars);
    OB_CSV_Arena_free(&csv->temp_arenas.chars);
}

EXTERN_C bool OB_CSV_to_file(const struct OB_CSV_Structs *const headers, const struct OB_CSV_Structs *const structs, const char *const path, const char separator) {
    assert(headers != NULL);
    assert(structs != NULL);
    assert(path != NULL);
    assert(separator != '\0');
    assert(headers->count == 1);

    bool success;
    const size_t headers_bools_required = OB_CSV_safe_mult(headers->member_count, headers->count, &success);
    if(!success) {
        return false;
    }

    const size_t structs_bools_required = OB_CSV_safe_mult(structs->member_count, structs->count, &success);
    if(!success) {
        return false;
    }

    if(headers_bools_required > SIZE_MAX - structs_bools_required) {
        return false;
    }
    const size_t bools_required = headers_bools_required + structs_bools_required;

    bool *const needs_escape = (bool*)OB_CSV_CALLOC(bools_required, sizeof(bool));
    if(needs_escape == NULL) {
        return false;
    }

    bool *const headers_needs_escape = needs_escape,
         *const structs_needs_escape = needs_escape + headers_bools_required;

    const size_t buffer_size = OB_CSV_Struct_overestimate_size(headers, headers_needs_escape, separator) + OB_CSV_Struct_overestimate_size(structs, structs_needs_escape, separator);

    if ((uintmax_t)buffer_size >= MIN((uintmax_t)INT64_MAX, (uintmax_t)SIZE_MAX) - 1) {
        OB_CSV_FREE(needs_escape);
        return false;
    }

    char *const csv_data = (char*)OB_CSV_MALLOC((buffer_size + sizeof((char)'\0')) * sizeof(char));
    if(csv_data == NULL) {
        OB_CSV_FREE(needs_escape);
        return false;
    }
    char *write_ptr = csv_data;

    if(buffer_size == 0) {
        *write_ptr = '\0';
    } else {
        write_ptr = OB_CSV_Struct_write_data(headers, write_ptr, headers_needs_escape, separator);
        write_ptr = OB_CSV_Struct_write_data(structs, write_ptr, structs_needs_escape, separator);
        *(write_ptr - 1) = '\0';
    }

    struct OB_CSV_FileContents file_contents;
    file_contents.data = (unsigned char*)csv_data;

    if(buffer_size == 0) {
        file_contents.size = 0;
    } else {
        file_contents.size = (int64_t)(write_ptr - csv_data - 1);
    }
    
    success = OB_CSV_FileContents_put(&file_contents, path) == OB_CSV_FILECONTENTS_ERROR_NONE;

    OB_CSV_FREE(csv_data);
    OB_CSV_FREE(needs_escape);

    return success;
}

EXTERN_C enum OB_CSV_Error OB_CSV_next_row(struct CCSV *const csv, OB_CSV_Row *const row) {
    assert(csv != NULL);
    assert(row != NULL);

    OB_CSV_Strings_init(row);
    OB_CSV_Strings_reserve(row, csv->counters.max_columns, &csv->arenas);

    const enum OB_CSV_Error error = OB_CSV_parse_next_row(csv, row);

    OB_CSV_Strings_reset(&csv->temp_strings);
    OB_CSV_Arena_reset(&csv->temp_arenas.chars);
    OB_CSV_Arena_reset(&csv->temp_arenas.strings);

    return error;
}
