#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_FILE_H
#define CCSV_FILE_H

#ifndef _WIN32
    #define _FILE_OFFSET_BITS 64
    #include <unistd.h>
#endif

enum CCSV_FileContents_Error {
    CCSV_FILECONTENTS_ERROR_NONE,
    CCSV_FILECONTENTS_ERROR_WIN32_API,
    CCSV_FILECONTENTS_ERROR_TOO_LARGE,
    CCSV_FILECONTENTS_ERROR_MEMORY,
    CCSV_FILECONTENTS_ERROR_FOPEN,
    CCSV_FILECONTENTS_ERROR_FREAD,
    CCSV_FILECONTENTS_ERROR_FWRITE,
    CCSV_FILECONTENTS_ERROR_FSEEK,
    CCSV_FILECONTENTS_ERROR_FTELL,
    CCSV_FILECONTENTS_ERROR_FCLOSE
};

struct CCSV_FileContents {
    unsigned char *data;
    unsigned       size;
};

void                         CCSV_FileContents_init(struct CCSV_FileContents*);
void                         CCSV_FileContents_free(struct CCSV_FileContents*);
enum CCSV_FileContents_Error CCSV_FileContents_get(struct CCSV_FileContents*, const char *path);
enum CCSV_FileContents_Error CCSV_FileContents_put(const struct CCSV_FileContents*, const char *path);

#endif

#ifdef __cplusplus
}
#endif
