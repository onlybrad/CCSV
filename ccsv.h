#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_H
#define CCSV_H

#include <stdint.h>

#include "tokens.h"
#include "counters.h"
#include "ccsv.h"
#include "row.h"
#include "arenas.h"
#include "file.h"

enum CCSV_Error {
    CCSV_ERROR_NONE,
    CCSV_ERROR_LEXER,
    CCSV_ERROR_FILE,
    CCSV_ERROR_MISSING_DBLQUOTE,
    CCSV_ERROR_EMPTY,
    CCSV_ERROR_TOO_LARGE,
    CCSV_ERROR_MEMORY,
};

struct CCSV {
    struct CCSV_Token       *current_token;
    const char              *data;
    int64_t                  length;
    struct CCSV_FileContents file_contents;
    struct CCSV_Counters     counters;
    struct CCSV_Tokens       tokens;
    struct CCSV_Arenas       arenas;
    struct CCSV_Arenas       temp_arenas;
    struct CCSV_Strings      temp_strings;
    char                     separator;
};

enum CCSV_Type {
    CCSV_TYPE_CHAR,
    CCSV_TYPE_UCHAR,
    CCSV_TYPE_SCHAR,
    CCSV_TYPE_SHORT,
    CCSV_TYPE_USHORT,
    CCSV_TYPE_BOOL,
    CCSV_TYPE_INT,
    CCSV_TYPE_UINT,
    CCSV_TYPE_LONG,
    CCSV_TYPE_ULONG,
    CCSV_TYPE_LLONG,
    CCSV_TYPE_ULLONG,
    CCSV_TYPE_FLOAT,
    CCSV_TYPE_DOUBLE,
    CCSV_TYPE_LDOUBLE,
    CCSV_TYPE_INT8,
    CCSV_TYPE_UINT8,
    CCSV_TYPE_INT16,
    CCSV_TYPE_UINT16,
    CCSV_TYPE_INT32,
    CCSV_TYPE_UINT32,
    CCSV_TYPE_INT64,
    CCSV_TYPE_UINT64,
    CCSV_TYPE_INTMAX,
    CCSV_TYPE_UINTMAX,
    CCSV_TYPE_SIZE,
    CCSV_TYPE_STRING
};

struct CCSV_StructMember {
    enum CCSV_Type type;
    size_t         offset;
};

struct CCSV_Structs {
    const void                     *data;
    const struct CCSV_StructMember *members;
    unsigned                        member_count;
    size_t                          size;
    size_t                          count;
};

enum CCSV_Error CCSV_from_string(struct CCSV*, const char *data, int64_t length, char separator);
enum CCSV_Error CCSV_from_file  (struct CCSV*, const char *path, char separator);
enum CCSV_Error CCSV_next_row   (struct CCSV*, CCSV_Row*);
void            CCSV_free       (struct CCSV*);
bool            CCSV_to_file    (const struct CCSV_Structs *headers, const struct CCSV_Structs *structs, const char *path, const char separator);

#endif

#ifdef __cplusplus
}
#endif