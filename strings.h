#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_STRINGS_H
#define OB_CSV_STRINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "arenas.h"

struct OB_CSV_Strings {
    char     **data;
    size_t   count;
    size_t   capacity;
    size_t   total_length;
};

void OB_CSV_Strings_init       (struct OB_CSV_Strings*);
bool OB_CSV_Strings_reserve    (struct OB_CSV_Strings*, size_t capacity, struct OB_CSV_Arenas*);
bool OB_CSV_Strings_push       (struct OB_CSV_Strings*, const char*, size_t length, struct OB_CSV_Arenas*);
bool OB_CSV_Strings_push_nocopy(struct OB_CSV_Strings*, char*, struct OB_CSV_Arenas*);
bool OB_CSV_Strings_concat     (const struct OB_CSV_Strings *const src, struct OB_CSV_Strings *const dst, struct OB_CSV_Arenas *arenas);
void OB_CSV_Strings_reset      (struct OB_CSV_Strings*);

#endif

#ifdef __cplusplus
}
#endif