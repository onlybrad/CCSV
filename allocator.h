#ifdef __cplusplus
extern "C" {
#endif

#ifndef CCSV_ALLOCATOR
#define CCSV_ALLOCATOR

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CCSV_ARENA_INFINITE_NODES 0
#define CCSV_ARENA_MINIMUM_SIZE   1024

#if defined(__GNUC__) || defined(__clang__)
#   define CCSV_ALIGNOF(T) __alignof__(T)
#elif defined(_MSC_VER)
#   define CCSV_ALIGNOF(T) __alignof(T)
#else
    #error "Missing macro definition CCSV_ALIGNOF for this platform"
#endif

struct CCSV_ArenaNode {
    struct CCSV_ArenaNode *next;
    size_t                 size,
                           offset;
    //unsigned char        data[]; //use the CCSV_GET_DATA() macro to a get a pointer to this
};

struct CCSV_Arena {
    struct CCSV_ArenaNode *head,
                          *current;
    size_t                 node_count,
                           node_max;
#ifndef NDEBUG
    const char            *name;
#endif
};

#define CCSV_ARENA_ALLOC(ARENA, COUNT, TYPE) (TYPE*)CCSV_Arena_alloc_objects(ARENA, (COUNT), sizeof(TYPE), CCSV_ALIGNOF(TYPE))

void  CCSV_Arena_init         (struct CCSV_Arena*, size_t node_max, const char *name);
bool  CCSV_Arena_create_node  (struct CCSV_Arena*, size_t size);
void  CCSV_Arena_free         (struct CCSV_Arena*);
void  CCSV_Arena_reset        (struct CCSV_Arena*);
void *CCSV_Arena_alloc_objects(struct CCSV_Arena*, size_t count, size_t size, size_t alignment);
void *CCSV_Arena_alloc        (struct CCSV_Arena*, size_t size, size_t alignment);
bool  CCSV_Arena_reserve      (struct CCSV_Arena*, size_t size, size_t alignment);
char *CCSV_Arena_strdup       (struct CCSV_Arena*, const char *str, size_t *length);

#ifndef NDEBUG

struct CCSV_AllocationStats {
    unsigned allocated;
    unsigned deallocated;
};

void *CCSV_debug_malloc (size_t);
void *CCSV_debug_calloc (size_t, size_t);
void *CCSV_debug_realloc(void*, size_t);
char *CCSV_debug_strdup (const char*);
void  CCSV_debug_free   (void*);
const struct CCSV_AllocationStats *CCSV_get_allocation_stats(void);

#define CCSV_MALLOC  CCSV_debug_malloc
#define CCSV_CALLOC  CCSV_debug_calloc
#define CCSV_REALLOC CCSV_debug_realloc
#define CCSV_STRDUP  CCSV_debug_strdup
#define CCSV_FREE    CCSV_debug_free

#else

#define CCSV_MALLOC  malloc
#define CCSV_CALLOC  calloc
#define CCSV_REALLOC realloc
#define CCSV_FREE    free
#if defined(__MINGW32__) || !defined(_WIN32)
    #define CCSV_STRDUP strdup
#else
    #define CCSV_STRDUP _strdup
#endif

#endif

#endif

#ifdef __cplusplus
}
#endif

