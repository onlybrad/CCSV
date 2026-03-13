#include "file.h"

#ifdef _WIN32
    #include <windows.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "util.h"
#include "allocator.h"

static int CCSV_fseek(FILE *const file, const int64_t offset, const int origin) {
#ifdef _WIN32
    return _fseeki64(file, (__int64)offset, origin);
#elif LONG_MAX < LLONG_MAX
    return fseeko(file, (off_t)offset, origin);
#else
    return fseek(file, (long)offset, origin);
#endif
}

static int64_t CCSV_ftell(FILE *const file) {
#ifdef _WIN32
    return (int64_t)_ftelli64(file);
#elif LONG_MAX < LLONG_MAX
    return (int64_t)ftello(file);
#else
    return (int64_t)ftell(file);
#endif    
}

static enum CCSV_FileContents_Error CCSV_fopen(FILE **const file, const char *const path, const char *const mode) {
    assert(file != NULL);
    assert(strcmp(mode, "rb") == 0 || strcmp(mode, "wb") == 0);

#ifdef _WIN32
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if(wide_length == 0) {
        *file = NULL;
        return CCSV_FILECONTENTS_ERROR_WIN32_API;
    }

    wchar_t *const wpath = (wchar_t*)CCSV_MALLOC((size_t)wide_length * sizeof(wchar_t));
    if(wpath == NULL) {
        *file = NULL;
        return CCSV_FILECONTENTS_ERROR_MEMORY;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wide_length);

    *file = _wfopen(wpath, strcmp(mode, "rb") == 0 ? L"rb" : L"wb");

    CCSV_FREE(wpath);
#else
    file = fopen(path, mode);
#endif

    return *file == NULL 
        ? CCSV_FILECONTENTS_ERROR_FOPEN
        : CCSV_FILECONTENTS_ERROR_NONE;
}

EXTERN_C void CCSV_FileContents_init(struct CCSV_FileContents *const file_contents) {
    assert(file_contents != NULL);

    file_contents->data = NULL;
    file_contents->size = 0U;
}

void CCSV_FileContents_free(struct CCSV_FileContents *const file_contents) {
    assert(file_contents != NULL);

    CCSV_FREE(file_contents->data);
    CCSV_FileContents_init(file_contents);
}

EXTERN_C enum CCSV_FileContents_Error CCSV_FileContents_get(struct CCSV_FileContents *const file_contents, const char *const path) {
    assert(file_contents != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');

    FILE *file;
    enum CCSV_FileContents_Error error = CCSV_fopen(&file, path, "rb");
    if(error != CCSV_FILECONTENTS_ERROR_NONE) {
        return error;
    }

    int64_t length;
    do {
        if(file == NULL) {
            error = CCSV_FILECONTENTS_ERROR_FOPEN;
            break;
        }

        if(CCSV_fseek(file, 0, SEEK_END) != 0) {
            error = CCSV_FILECONTENTS_ERROR_FSEEK;
            break;
        }

        length = CCSV_ftell(file);
        if(length == -1) {
            error = CCSV_FILECONTENTS_ERROR_FTELL;
            break;
        }

        if(length >= (int64_t)UINT_MAX) {
            error = CCSV_FILECONTENTS_ERROR_TOO_LARGE;
            break;
        }

        if(CCSV_fseek(file, 0, SEEK_SET) != 0) {
            error = CCSV_FILECONTENTS_ERROR_FSEEK;
            break;
        }

        //the buffer returned has 1 extra byte allocated in case a null terminated string is required
        file_contents->data = (unsigned char*)CCSV_MALLOC(((size_t)length + 1) * sizeof(unsigned char));
        if(file_contents->data == NULL) {
            error = CCSV_FILECONTENTS_ERROR_MEMORY;
            break;
        }
        file_contents->data[length] = '\0';
        
        if(fread(
            file_contents->data,
            sizeof(unsigned char),
            (size_t)length, 
            file
        ) != (size_t)length) {
            error = CCSV_FILECONTENTS_ERROR_FREAD;
            break;
        }

        error               = CCSV_FILECONTENTS_ERROR_NONE;
        file_contents->size = (unsigned)length;
    } while(0);

    if(error != CCSV_FILECONTENTS_ERROR_NONE) {
        CCSV_FileContents_free(file_contents);
    }
    
    fclose(file);
    
    return error;
}

enum CCSV_FileContents_Error CCSV_FileContents_put(const struct CCSV_FileContents *const file_contents, const char *const path) {
    assert(file_contents != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');

    const size_t size = file_contents->size == 0U
        ? strlen((char*)file_contents->data)
        : (size_t)file_contents->size;

    FILE *file;
    enum CCSV_FileContents_Error error = CCSV_fopen(&file, path, "wb");
    if(error != CCSV_FILECONTENTS_ERROR_NONE) {
        return error;
    }

    if(fwrite(
        file_contents->data,
        sizeof(file_contents->data[0]),
        size,
        file
    ) != size) {
        fclose(file);
        return CCSV_FILECONTENTS_ERROR_FWRITE;
    }

    if(fclose(file) != 0) {
        error = CCSV_FILECONTENTS_ERROR_FCLOSE;
    }
       
    return error;
}