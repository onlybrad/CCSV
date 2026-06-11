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

static int OB_CSV_fseek(FILE *const file, const int64_t offset, const int origin) {
#ifdef _WIN32
    return _fseeki64(file, (__int64)offset, origin);
#elif LONG_MAX < LLONG_MAX
    return fseeko(file, (off_t)offset, origin);
#else
    return fseek(file, (long)offset, origin);
#endif
}

static int64_t OB_CSV_ftell(FILE *const file) {
#ifdef _WIN32
    return (int64_t)_ftelli64(file);
#elif LONG_MAX < LLONG_MAX
    return (int64_t)ftello(file);
#else
    return (int64_t)ftell(file);
#endif    
}

static enum OB_CSV_FileContents_Error OB_CSV_fopen(FILE **const file, const char *const path, const char *const mode) {
    assert(file != NULL);
    assert(strcmp(mode, "rb") == 0 || strcmp(mode, "wb") == 0);

#ifdef _WIN32
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if(wide_length == 0) {
        *file = NULL;
        return OB_CSV_FILECONTENTS_ERROR_WIN32_API;
    }

    wchar_t *const wpath = (wchar_t*)OB_CSV_MALLOC((size_t)wide_length * sizeof(wchar_t));
    if(wpath == NULL) {
        *file = NULL;
        return OB_CSV_FILECONTENTS_ERROR_MEMORY;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wide_length);

    *file = _wfopen(wpath, strcmp(mode, "rb") == 0 ? L"rb" : L"wb");

    OB_CSV_FREE(wpath);
#else
    file = fopen(path, mode);
#endif

    return *file == NULL 
        ? OB_CSV_FILECONTENTS_ERROR_FOPEN
        : OB_CSV_FILECONTENTS_ERROR_NONE;
}

EXTERN_C void OB_CSV_FileContents_init(struct OB_CSV_FileContents *const file_contents) {
    assert(file_contents != NULL);

    file_contents->data = NULL;
    file_contents->size = 0;
}

EXTERN_C void OB_CSV_FileContents_free(struct OB_CSV_FileContents *const file_contents) {
    assert(file_contents != NULL);

    OB_CSV_FREE(file_contents->data);
    OB_CSV_FileContents_init(file_contents);
}

EXTERN_C enum OB_CSV_FileContents_Error OB_CSV_FileContents_get(struct OB_CSV_FileContents *const file_contents, const char *const path) {
    assert(file_contents != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');

    FILE *file;
    enum OB_CSV_FileContents_Error error = OB_CSV_fopen(&file, path, "rb");
    if(error != OB_CSV_FILECONTENTS_ERROR_NONE) {
        return error;
    }

    int64_t length;
    do {
        if(file == NULL) {
            error = OB_CSV_FILECONTENTS_ERROR_FOPEN;
            break;
        }

        if(OB_CSV_fseek(file, 0, SEEK_END) != 0) {
            error = OB_CSV_FILECONTENTS_ERROR_FSEEK;
            break;
        }

        length = OB_CSV_ftell(file);
        if(length == -1) {
            error = OB_CSV_FILECONTENTS_ERROR_FTELL;
            break;
        }

        if(OB_CSV_fseek(file, 0, SEEK_SET) != 0) {
            error = OB_CSV_FILECONTENTS_ERROR_FSEEK;
            break;
        }

        //the buffer returned has 1 extra byte allocated in case a null terminated string is required
        file_contents->data = (unsigned char*)OB_CSV_MALLOC(((size_t)length + 1) * sizeof(unsigned char));
        if(file_contents->data == NULL) {
            error = OB_CSV_FILECONTENTS_ERROR_MEMORY;
            break;
        }
        
        if(fread(file_contents->data, sizeof(unsigned char), (size_t)length, file) != (size_t)length) {
            error = OB_CSV_FILECONTENTS_ERROR_FREAD;
            break;
        }
        file_contents->data[length] = '\0';

        error               = OB_CSV_FILECONTENTS_ERROR_NONE;
        file_contents->size = length;
    } while(0);

    if(error != OB_CSV_FILECONTENTS_ERROR_NONE) {
        OB_CSV_FileContents_free(file_contents);
    }
    
    fclose(file);
    
    return error;
}

EXTERN_C enum OB_CSV_FileContents_Error OB_CSV_FileContents_put(const struct OB_CSV_FileContents *const file_contents, const char *const path) {
    assert(file_contents != NULL);
    assert(path != NULL);
    assert(path[0] != '\0');

    const size_t size = file_contents->size == 0U
        ? strlen((char*)file_contents->data)
        : (size_t)file_contents->size;

    FILE *file;
    enum OB_CSV_FileContents_Error error = OB_CSV_fopen(&file, path, "wb");
    if(error != OB_CSV_FILECONTENTS_ERROR_NONE) {
        return error;
    }

    if(fwrite(file_contents->data, sizeof(file_contents->data[0]), size, file) != size) {
        fclose(file);
        return OB_CSV_FILECONTENTS_ERROR_FWRITE;
    }

    if(fclose(file) != 0) {
        error = OB_CSV_FILECONTENTS_ERROR_FCLOSE;
    }
       
    return error;
}