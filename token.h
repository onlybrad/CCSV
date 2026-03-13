#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_TOKEN_H
#define CCSV_TOKEN_H

#include <stdio.h>

enum CCSV_Token_Type {
    CCSV_TOKEN_STRING,
    CCSV_TOKEN_SEPARATOR,
    CCSV_TOKEN_DBLQUOTE,
    CCSV_TOKEN_NEWLINE,
    CCSV_TOKEN_CARRIAGE
};

struct CCSV_Token {
    const char           *value;
    unsigned              length;
    enum CCSV_Token_Type  type;
};

#endif

#ifdef __cplusplus
}
#endif