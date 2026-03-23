#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_H
#define CCSV_H

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
    unsigned                 length;
    struct CCSV_FileContents file_contents;
    struct CCSV_Counters     counters;
    struct CCSV_Tokens       tokens;
    struct CCSV_Arenas       arenas;
    struct CCSV_Arenas       temp_arenas;
    struct CCSV_Strings      temp_strings;
    char                     separator;
};

enum CCSV_Error CCSV_from_string(struct CCSV*, const char *data, unsigned length, char separator);
enum CCSV_Error CCSV_from_file  (struct CCSV*, const char *path, char separator);
void            CCSV_free       (struct CCSV*);
enum CCSV_Error CCSV_next_row   (struct CCSV*, CCSV_Row*);

#endif

#ifdef __cplusplus
}
#endif