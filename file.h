#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_FILE_H
#define OB_CSV_FILE_H

#ifndef _WIN32
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
#endif

#include <stdint.h>

enum OB_CSV_FileContents_Error {
    OB_CSV_FILECONTENTS_ERROR_NONE,
    OB_CSV_FILECONTENTS_ERROR_WIN32_API,
    OB_CSV_FILECONTENTS_ERROR_MEMORY,
    OB_CSV_FILECONTENTS_ERROR_FOPEN,
    OB_CSV_FILECONTENTS_ERROR_FREAD,
    OB_CSV_FILECONTENTS_ERROR_FWRITE,
    OB_CSV_FILECONTENTS_ERROR_FSEEK,
    OB_CSV_FILECONTENTS_ERROR_FTELL,
    OB_CSV_FILECONTENTS_ERROR_FCLOSE
};

struct OB_CSV_FileContents {
    unsigned char *data;
    int64_t        size;
};

void                         OB_CSV_FileContents_init(struct OB_CSV_FileContents*);
void                         OB_CSV_FileContents_free(struct OB_CSV_FileContents*);
enum OB_CSV_FileContents_Error OB_CSV_FileContents_get (struct OB_CSV_FileContents*, const char *path);
enum OB_CSV_FileContents_Error OB_CSV_FileContents_put (const struct OB_CSV_FileContents*, const char *path);

#endif

#ifdef __cplusplus
}
#endif
