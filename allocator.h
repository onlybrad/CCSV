#ifdef __cplusplus
extern "C" {
#endif

#ifndef OB_CSV_ALLOCATOR
#define OB_CSV_ALLOCATOR

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define OB_CSV_ARENA_INFINITE_NODES 0
#define OB_CSV_ARENA_MINIMUM_SIZE   1024

#if defined(__GNUC__) || defined(__clang__)
#   define OB_CSV_ALIGNOF(T) __alignof__(T)
#elif defined(_MSC_VER)
#   define OB_CSV_ALIGNOF(T) __alignof(T)
#else
    #error "Missing macro definition OB_CSV_ALIGNOF for this platform"
#endif

struct OB_CSV_ArenaNode {
    struct OB_CSV_ArenaNode *next;
    size_t                 size,
                           offset;
    //unsigned char        data[]; //use the OB_CSV_GET_DATA() macro to a get a pointer to this
};

struct OB_CSV_Arena {
    struct OB_CSV_ArenaNode *head,
                          *current;
    size_t                 node_count,
                           node_max;
#ifndef NDEBUG
    const char            *name;
#endif
};

#define OB_CSV_ARENA_ALLOC(ARENA, COUNT, TYPE) (TYPE*)OB_CSV_Arena_alloc_objects(ARENA, (COUNT), sizeof(TYPE), OB_CSV_ALIGNOF(TYPE))

void  OB_CSV_Arena_init         (struct OB_CSV_Arena*, size_t node_max, const char *name);
bool  OB_CSV_Arena_create_node  (struct OB_CSV_Arena*, size_t size);
void  OB_CSV_Arena_free         (struct OB_CSV_Arena*);
void  OB_CSV_Arena_reset        (struct OB_CSV_Arena*);
void *OB_CSV_Arena_alloc_objects(struct OB_CSV_Arena*, size_t count, size_t size, size_t alignment);
void *OB_CSV_Arena_alloc        (struct OB_CSV_Arena*, size_t size, size_t alignment);
bool  OB_CSV_Arena_reserve      (struct OB_CSV_Arena*, size_t size, size_t alignment);
char *OB_CSV_Arena_strdup       (struct OB_CSV_Arena*, const char *str, size_t *length);

#ifndef NDEBUG

struct OB_CSV_AllocationStats {
    unsigned allocated;
    unsigned deallocated;
};

void *OB_CSV_debug_malloc (size_t);
void *OB_CSV_debug_calloc (size_t, size_t);
void *OB_CSV_debug_realloc(void*, size_t);
char *OB_CSV_debug_strdup (const char*);
void  OB_CSV_debug_free   (void*);
const struct OB_CSV_AllocationStats *OB_CSV_get_allocation_stats(void);

#define OB_CSV_MALLOC  OB_CSV_debug_malloc
#define OB_CSV_CALLOC  OB_CSV_debug_calloc
#define OB_CSV_REALLOC OB_CSV_debug_realloc
#define OB_CSV_STRDUP  OB_CSV_debug_strdup
#define OB_CSV_FREE    OB_CSV_debug_free

#else

#define OB_CSV_MALLOC  malloc
#define OB_CSV_CALLOC  calloc
#define OB_CSV_REALLOC realloc
#define OB_CSV_FREE    free
#if defined(__MINGW32__) || !defined(_WIN32)
    #define OB_CSV_STRDUP strdup
#else
    #define OB_CSV_STRDUP _strdup
#endif

#endif

#endif

#ifdef __cplusplus
}
#endif

