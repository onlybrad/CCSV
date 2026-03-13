#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_STRINGS_H
#define CCSV_STRINGS_H

#include <stdbool.h>

#include "arenas.h"

struct CCSV_Strings {
    char     **data;
    unsigned   count;
    unsigned   capacity;
    unsigned   total_length;
};

void CCSV_Strings_init       (struct CCSV_Strings*);
bool CCSV_Strings_reserve    (struct CCSV_Strings*, unsigned capacity, struct CCSV_Arenas*);
bool CCSV_Strings_push       (struct CCSV_Strings*, const char*, unsigned length, struct CCSV_Arenas*);
bool CCSV_Strings_push_nocopy(struct CCSV_Strings*, char*, struct CCSV_Arenas*);
bool CCSV_Strings_concat     (const struct CCSV_Strings *const src, struct CCSV_Strings *const dst, struct CCSV_Arenas *arenas);
void CCSV_Strings_reset      (struct CCSV_Strings*);

#endif

#ifdef __cplusplus
}
#endif