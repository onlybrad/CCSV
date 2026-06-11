#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_TOKEN_H
#define OB_CSV_TOKEN_H

#include <stdio.h>

enum OB_CSV_Token_Type {
    OB_CSV_TOKEN_STRING,
    OB_CSV_TOKEN_SEPARATOR,
    OB_CSV_TOKEN_DBLQUOTE,
    OB_CSV_TOKEN_NEWLINE,
    OB_CSV_TOKEN_CARRIAGE
};

struct OB_CSV_Token {
    const char           *value;
    size_t                length;
    enum OB_CSV_Token_Type  type;
};

#endif

#ifdef __cplusplus
}
#endif