#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "allocator.h"
#include "util.h"

#define CCSV_GET_DATA(NODE) ((unsigned char *)((NODE) + 1))

#ifndef NDEBUG

static struct CCSV_AllocationStats allocation_stats;

void *CCSV_debug_malloc(const size_t size) {
    void *const ret = malloc(size);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;
}

void *CCSV_debug_calloc(const size_t count, const size_t size) {
    void *const ret = calloc(count, size);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;
}

void *CCSV_debug_realloc(void *const ptr, const size_t size) {
    void *const ret = realloc(ptr, size);

    if(ret != NULL) {
        if(ptr != NULL) {
            allocation_stats.deallocated++;
        }
        allocation_stats.allocated++;
    }

    return ret;
}

char *CCSV_debug_strdup(const char *const str) {
#if defined(__MINGW32__) || !defined(_WIN32)
    #define CCSV_STRDUP_FUNC strdup
#else
    #define CCSV_STRDUP_FUNC _strdup
#endif

    char *const ret = CCSV_STRDUP_FUNC(str);
    if(ret != NULL) {
        allocation_stats.allocated++;
    }

    return ret;

#undef CCSV_STRDUP_FUNC
}

void CCSV_debug_free(void *ptr) {
    if(ptr == NULL) {
        return;
    }

    free(ptr);
    allocation_stats.deallocated++;
}

const struct CCSV_AllocationStats *CCSV_get_allocation_stats(void) {
    return &allocation_stats;
}

#endif

static struct CCSV_ArenaNode *CCSV_ArenaNode_new(const size_t size) {
    assert(size > 0);

    struct CCSV_ArenaNode *const node = (struct CCSV_ArenaNode *)CCSV_CALLOC(sizeof(*node) + size, sizeof(unsigned char));
    if(node == NULL) {
        return NULL;
    }

    node->size   = size;
    node->offset = 0;
    node->next   = NULL;
    
    return node;
}

static bool CCSV_Arena_create_next_node(struct CCSV_Arena *const arena, const size_t size) {
    assert(arena != NULL);
    assert(size > 0);

    size_t node_size = arena->current->size;
    if(node_size < size) {
        if(node_size > (size_t)UINT_MAX / 2) {
            node_size = size;
        } else do {
            node_size *= 2;
        } while(node_size < size);
    }

    struct CCSV_ArenaNode *const current = arena->current;
    struct CCSV_ArenaNode *const next    = current->next;
    if(next == NULL) {
        if(arena->node_count == arena->node_max) {
            return false;
        }

        if((current->next = CCSV_ArenaNode_new(node_size)) != NULL) {
            arena->current = current->next;
            arena->node_count++;
            return true;
        }

        return false;
    }

    if(next->size < size) {
        struct CCSV_ArenaNode *const next_next = next->next;
        CCSV_FREE(next);

        if((current->next = CCSV_ArenaNode_new(node_size)) != NULL) {
            current->next->next = next_next;
            arena->current      = current->next;
            return true;
        }

        current->next = next_next;
        arena->node_count--;
        return false;
    }

    return true;
}

EXTERN_C void CCSV_Arena_init(struct CCSV_Arena *const arena, const size_t node_max, const char *const name) {
    assert(arena != NULL);

    arena->node_count = 0U;
    arena->node_max   = node_max;
    arena->head       = NULL;
    arena->current    = NULL;

#ifndef NDEBUG
    arena->name = name;
#else
    (void)name;
#endif

}

EXTERN_C bool CCSV_Arena_create_node(struct CCSV_Arena *const arena, size_t size) {
    assert(arena != NULL);
    assert(size > 0U);

    if(arena->head != NULL) {
        return true;
    }

    if(size < CCSV_ARENA_MINIMUM_SIZE) {
        size = CCSV_ARENA_MINIMUM_SIZE;
    }

    arena->current = arena->head = CCSV_ArenaNode_new(size);

    if(arena->head != NULL) {
        arena->node_count = 1U;
    }

    return arena->head != NULL;
}

EXTERN_C void CCSV_Arena_free(struct CCSV_Arena *const arena) {
    assert(arena != NULL);
    
    struct CCSV_ArenaNode *current = arena->head;
    arena->head = NULL;
    while(current != NULL) {
        struct CCSV_ArenaNode *const next = current->next;
        CCSV_FREE(current);
        current = next;
    }

#ifndef NDEBUG
    CCSV_Arena_init(arena, arena->node_max, arena->name);
#else
    CCSV_Arena_init(arena, arena->node_max, NULL);
#endif
}

EXTERN_C void CCSV_Arena_reset(struct CCSV_Arena *const arena) {
    assert(arena != NULL);
    
    arena->current      = arena->head;
    arena->head->offset = 0U;
}

EXTERN_C void *CCSV_Arena_alloc_objects(struct CCSV_Arena *const arena, const size_t count, const size_t size, const size_t alignment) {
    assert(arena != NULL);
    assert(count > 0U);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    bool success;
    const size_t total_size = CCSV_safe_mult(count, size, &success);
    
    return success ? CCSV_Arena_alloc(arena, total_size, alignment) : NULL;
}

EXTERN_C void *CCSV_Arena_alloc(struct CCSV_Arena *const arena, const size_t size, size_t alignment) {
    assert(arena != NULL);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    if(alignment == 0) {
        alignment = CCSV_ALIGNOF(uintmax_t);
    }

    if(!CCSV_Arena_create_node(arena, size)) {
        return NULL;
    }

    const uintptr_t start_address = (uintptr_t)(CCSV_GET_DATA(arena->current) + arena->current->offset);
    uintptr_t aligned_address     = (start_address + ((uintptr_t)alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    size_t padding              = (size_t)(aligned_address - start_address);

    if(arena->current->offset + padding + size > arena->current->size) {
        if(!CCSV_Arena_create_next_node(arena, size)) {
            return NULL;
        }

        aligned_address = (uintptr_t)(CCSV_GET_DATA(arena->current));
        padding         = 0U;
    }

    arena->current->offset += padding + size;
    return (void*)aligned_address;
}

bool CCSV_Arena_reserve(struct CCSV_Arena *const arena, const size_t size, size_t alignment) {
    assert(arena != NULL);
    assert(size > 0U);
    assert((alignment & (alignment - 1U)) == 0U);

    if(alignment == 0) {
        alignment = CCSV_ALIGNOF(uintmax_t);
    }

    if(!CCSV_Arena_create_node(arena, size)) {
        return NULL;
    }

    const uintptr_t start_address = (uintptr_t)(CCSV_GET_DATA(arena->current) + arena->current->offset);
    uintptr_t aligned_address     = (start_address + ((uintptr_t)alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    size_t padding              = (size_t)(aligned_address - start_address);

    if(arena->current->offset + padding + size <= arena->current->size) {
        return true;
    }

    return CCSV_Arena_create_next_node(arena, size);
}


EXTERN_C char *CCSV_Arena_strdup(struct CCSV_Arena *const arena, const char *const str, size_t *const length) {
    assert(arena != NULL);
    assert(str != NULL);

    const size_t len = strlen(str);
    char *const copy = CCSV_ARENA_ALLOC(arena, len + 1U, char);
    if(copy == NULL) {
        return NULL;
    }

    if(length != NULL) {
        *length = len;
    }
    
    return strcpy(copy, str);
}