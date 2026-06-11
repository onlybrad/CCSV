#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_H
#define OB_CSV_H

#include <stdint.h>

#include "tokens.h"
#include "counters.h"
#include "obcsv.h"
#include "row.h"
#include "arenas.h"
#include "file.h"

enum OB_CSV_Error {
    OB_CSV_ERROR_NONE,
    OB_CSV_ERROR_LEXER,
    OB_CSV_ERROR_FILE,
    OB_CSV_ERROR_MISSING_DBLQUOTE,
    OB_CSV_ERROR_EMPTY,
    OB_CSV_ERROR_TOO_LARGE,
    OB_CSV_ERROR_MEMORY,
};

struct CCSV {
    struct OB_CSV_Token       *current_token;
    const char              *data;
    int64_t                  length;
    struct OB_CSV_FileContents file_contents;
    struct OB_CSV_Counters     counters;
    struct OB_CSV_Tokens       tokens;
    struct OB_CSV_Arenas       arenas;
    struct OB_CSV_Arenas       temp_arenas;
    struct OB_CSV_Strings      temp_strings;
    char                     separator;
};

enum OB_CSV_Type {
    OB_CSV_TYPE_CHAR,
    OB_CSV_TYPE_UCHAR,
    OB_CSV_TYPE_SCHAR,
    OB_CSV_TYPE_SHORT,
    OB_CSV_TYPE_USHORT,
    OB_CSV_TYPE_BOOL,
    OB_CSV_TYPE_INT,
    OB_CSV_TYPE_UINT,
    OB_CSV_TYPE_LONG,
    OB_CSV_TYPE_ULONG,
    OB_CSV_TYPE_LLONG,
    OB_CSV_TYPE_ULLONG,
    OB_CSV_TYPE_FLOAT,
    OB_CSV_TYPE_DOUBLE,
    OB_CSV_TYPE_LDOUBLE,
    OB_CSV_TYPE_INT8,
    OB_CSV_TYPE_UINT8,
    OB_CSV_TYPE_INT16,
    OB_CSV_TYPE_UINT16,
    OB_CSV_TYPE_INT32,
    OB_CSV_TYPE_UINT32,
    OB_CSV_TYPE_INT64,
    OB_CSV_TYPE_UINT64,
    OB_CSV_TYPE_INTMAX,
    OB_CSV_TYPE_UINTMAX,
    OB_CSV_TYPE_SIZE,
    OB_CSV_TYPE_STRING
};

struct OB_CSV_StructMember {
    enum OB_CSV_Type type;
    size_t         offset;
};

struct OB_CSV_Structs {
    const void                     *data;
    const struct OB_CSV_StructMember *members;
    unsigned                        member_count;
    size_t                          size;
    size_t                          count;
};

enum OB_CSV_Error OB_CSV_from_string(struct CCSV*, const char *data, int64_t length, char separator);
enum OB_CSV_Error OB_CSV_from_file  (struct CCSV*, const char *path, char separator);
enum OB_CSV_Error OB_CSV_next_row   (struct CCSV*, OB_CSV_Row*);
void            OB_CSV_free       (struct CCSV*);
bool            OB_CSV_to_file    (const struct OB_CSV_Structs *headers, const struct OB_CSV_Structs *structs, const char *path, const char separator);

#endif

#ifdef __cplusplus
}
#endif