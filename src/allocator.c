/**
 * @file allocator.c
 * @brief Implementation of allocator.h
 *
 * This file contains the actual implementation of arena, pool, and buffer
 * allocators. The declarations are in allocator.h.
 *
 * @author Sadiq Idris (@sadichel)
 * @license MIT
 * @version 1.0 — January 2026
 */

#include <contain/allocator.h>

#ifdef ALLOC_DEBUG

static int arena_owns(Arena *arena, void *ptr, size_t stride) {
    uintptr_t p = (uintptr_t)ptr;
    while (arena) {
        uintptr_t start = (uintptr_t)arena->buffer;
        uintptr_t end = start + arena->capacity;

        if (p >= start && p < end) {
            uintptr_t offset = p - start;
            if (offset % stride == 0)
                return 1;
            return 0;
        }
        arena = arena->next;
    }
    return 0;
}

static int buffer_owns(Buffer *buf, void *ptr, size_t stride) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)buf->buffer;
    uintptr_t end = start + buf->capacity;

    if (p >= start && p < end) {
        uintptr_t offset = p - start;
        if (offset % stride == 0)
            return 1;
    }
    return 0;
}

static int pool_owns(Pool *pool, void *ptr, size_t stride) {
    uintptr_t p = (uintptr_t)ptr;
    while (pool) {
        uintptr_t start = (uintptr_t)pool->base;
        size_t total = pool->block_size * pool->capacity;
        uintptr_t end = start + total;

        if (p >= start && p < end) {
            uintptr_t offset = p - start;
            if (offset % stride == 0)
                return 1;
            return 0;
        }
        pool = pool->next;
    }
    return 0;
}

static int pool_freed(Pool *pool, void *ptr) {
    void *curr = pool->freelist;
    while (curr) {
        if (curr == ptr) {
            fprintf(stderr, "DOUBLE FREE DETECTED [%p]\n", ptr);
            return 1;
        }
        curr = *(void **)curr;
    }
    return 0;
}

static int allocator_freed(Allocator *a, void *ptr) {
    void *curr = a->freelist;
    while (curr) {
        if (curr == ptr) {
            fprintf(stderr, "DOUBLE FREE DETECTED [%p]\n", ptr);
            return 1;
        }
        curr = *(void **)curr;
    }
    return 0;
}

#endif

static size_t allocator_grow_capacity(size_t curr, size_t need, GrowthPolicy policy);


/* ===================== ARENA ===================== */

static bool arena_validate_params(uintptr_t addr, size_t arena_size, size_t capacity, size_t align) {
    size_t pad = align - 1;

    if (align == 0) {
        ALLOC_DEBUG_PRINT("arena: align must be non-zero");
        return false;
    }

    if ((align & pad) != 0) {
        ALLOC_DEBUG_PRINT("arena: align must be power of two");
        return false;
    }

    if (addr > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("arena: address alignment overflow");
        return false;
    }

    if (arena_size > SIZE_MAX - capacity) {
        ALLOC_DEBUG_PRINT("arena: arena_size + capacity overflow");
        return false;
    }

    if (arena_size + capacity > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("arena: total allocation overflow");
        return false;
    }

    return true;
}

static Arena *arena_create_impl(size_t capacity, size_t align) {
    if (!arena_validate_params(0, sizeof(Arena), capacity, align))
        return NULL;

    size_t total = sizeof(Arena) + capacity + align - 1;

    Arena *a = (Arena *)malloc(total);
    if (!a) {
        ALLOC_DEBUG_PRINT("arena_create: malloc failed");
        return NULL;
    }

    uintptr_t start = (uintptr_t)a + sizeof(*a);
    uintptr_t aligned = (start + align - 1) & ~(uintptr_t)(align - 1);
    
    ALLOC_POISON_ALLOC(aligned, capacity);

    a->buffer = (uint8_t *)aligned;
    a->capacity = capacity;
    a->used = 0;
    a->next = NULL;

    return a;
}

static Arena *arena_from_impl(void *buf, size_t capacity, size_t align) {
    if (!arena_validate_params((uintptr_t)buf, 0, capacity, align))
        return NULL;

    uintptr_t start = (uintptr_t)buf;
    uintptr_t aligned = (start + align - 1) & ~(uintptr_t)(align - 1);

    size_t diff = (size_t)(aligned - start);

    if (capacity <= diff) {
        ALLOC_DEBUG_PRINT("arena_from: buffer too small after alignment");
        return NULL;
    }

    size_t usable_capacity = capacity - diff;

    Arena *a = (Arena *)malloc(sizeof(Arena));
    if (!a) {
        ALLOC_DEBUG_PRINT("arena_from: malloc failed");
        return NULL;
    }
    
    ALLOC_POISON_ALLOC(aligned, usable_capacity);

    a->buffer = (uint8_t *)aligned;
    a->capacity = usable_capacity;
    a->used = 0;
    a->next = NULL;

    return a;
}

static void *arena_alloc_impl(Arena *arena, size_t size, size_t align) {
    size_t pad = align - 1;

    if (align == 0 || (align & pad) != 0) {
        ALLOC_DEBUG_PRINT("arena_alloc failed: align must be power of two");
        return NULL;
    }

    uintptr_t curr = (uintptr_t)arena->buffer + arena->used;

    if (pad > SIZE_MAX - curr) {
        ALLOC_DEBUG_PRINT("arena_alloc failed: alignment too large");
        return NULL;
    }

    uintptr_t aligned = (curr + pad) & ~(uintptr_t)pad;
    size_t padding = (size_t)(aligned - curr);

    size_t remaining = arena->capacity - arena->used;

    if (padding > remaining || size > remaining - padding)
        return NULL;

    arena->used += padding + size;

    ALLOC_POISON_ALLOC(aligned, size);

    return (void *)aligned;
}

static void *arena_stride_alloc(Arena *arena, size_t stride) {
    if (stride > arena->capacity - arena->used)
        return NULL;
    void *ptr = arena->buffer + arena->used;
    
    ALLOC_POISON_ALLOC(ptr, stride);
    
    arena->used += stride;
    return ptr;
}

Arena *arena_create(size_t capacity) {
    if (capacity == 0) return NULL;
    return arena_create_impl(capacity, 1);
}

Arena *arena_create_aligned(size_t capacity, size_t align) {
    if (capacity == 0) return NULL;
    return arena_create_impl(capacity, align);
}

Arena *arena_from(void *buf, size_t capacity) {
    if (!buf || capacity == 0) return NULL;
    return arena_from_impl(buf, capacity, 1);
}

Arena *arena_from_aligned(void *buf, size_t capacity, size_t align) {
    if (!buf || capacity == 0) return NULL;
    return arena_from_impl(buf, capacity, align);
}

void *arena_alloc(Arena *arena, size_t size) {
    if (!arena || size == 0) return NULL;
    return arena_alloc_impl(arena, size, 1);
}

void *arena_alloc_aligned(Arena *arena, size_t size, size_t align) {
    if (!arena || size == 0) return NULL;
    return arena_alloc_impl(arena, size, align);
}

void arena_reset(Arena *arena) {
    while (arena) {
        ALLOC_POISON_ALLOC(arena->buffer, arena->capacity);
        arena->used = 0;
        arena = arena->next;
    }
}

void arena_destroy(Arena *arena) {
    while (arena) {
        Arena *next = arena->next;
        free(arena);
        arena = next;
    }
}

size_t arena_numlist(const Arena *arena) {
    size_t count = 0;
    while (arena) {
        count++;
        arena = arena->next;
    }
    return count;
}

static void *arena_alloc_adpt(Allocator *a) {
    Arena *curr = (Arena *)a->tail;

    while (1) {
        void *chunk = arena_stride_alloc(curr, a->stride);
        if (chunk) return chunk;

        if (curr->next) {
            curr = curr->next;
            continue;
        }

        if (a->grow == GROW_NONE) return NULL;

        size_t next_cap = allocator_grow_capacity(curr->capacity, curr->capacity, a->grow);
        if (!next_cap) return NULL;

        Arena *next = arena_create_impl(next_cap, a->alignment);
        if (!next) {
            ALLOC_DEBUG_PRINT("arena_alloc failed: could not create next arena");
            return NULL;
        }

        curr->next = next;
        a->tail = next;
        a->capacity += next->capacity;
        curr = next;
    }
}

static void arena_free_adpt(Allocator *a, void *ptr) {
    if (!ptr || a->stride < sizeof(void *)) return;

#ifdef ALLOC_DEBUG
    assert(arena_owns((Arena *)a->head, ptr, a->stride));
    assert(allocator_freed(a, ptr) == 0);
#endif

    ALLOC_POISON_FREE(ptr, a->stride);

    *(void **)ptr = a->freelist;
    a->freelist = ptr;
}

static void arena_stats_adpt(const Allocator *a, AllocatorStats *out) {
    memset(out, 0, sizeof(*out));
    out->capacity = a->capacity / a->stride;
    out->used = a->allocations;
    out->freelist = out->capacity - out->used;
    out->segments = allocator_numlist(a);
}

static size_t arena_numlist_adpt(const Allocator *a) { return arena_numlist((Arena *)a->head); }

static void arena_reset_adpt(Allocator *a) {
    arena_reset((Arena *)a->head);
    a->freelist = NULL;
}

static void arena_destroy_adpt(Allocator *a) { arena_destroy((Arena *)a->head); }

static size_t arena_available_adpt(const Allocator *a) {
    Arena *arena = (Arena *)a->tail;
    return (arena->capacity - arena->used) / a->stride;
}

static size_t arena_reusable_adpt(const Allocator *a) {
    size_t freeblocks = 0;
    void *curr = a->freelist;
    while (curr) {
        freeblocks++;
        curr = *(void **)curr;
    }
    return freeblocks;
}

static const AllocatorOps arena_ops = {
    .alloc = arena_alloc_adpt,
    .free  = arena_free_adpt,
    .reset = arena_reset_adpt,
    .stats = arena_stats_adpt,
    .destroy = arena_destroy_adpt,
    .numlist = arena_numlist_adpt,
    .available = arena_available_adpt,
    .reusable  = arena_reusable_adpt
};

/* ===================== POOL ===================== */

static bool pool_validate_params(uintptr_t addr, size_t header_size, size_t block_size, size_t nblocks, size_t align) {
    size_t pad = align - 1;

    if (block_size == 0) {
        ALLOC_DEBUG_PRINT("pool: block_size must be > 0");
        return false;
    }

    if (nblocks == 0) {
        ALLOC_DEBUG_PRINT("pool: nblocks must be > 0");
        return false;
    }

    if (align == 0 || (align & pad) != 0) {
        ALLOC_DEBUG_PRINT("pool: align must be power of two");
        return false;
    }

    if (addr > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("pool: address alignment overflow");
        return false;
    }

    if (block_size > SIZE_MAX / nblocks) {
        ALLOC_DEBUG_PRINT("pool: block_size * nblocks overflow");
        return false;
    }

    size_t stride = block_size < sizeof(void*) ? sizeof(void*) : block_size;

    if (stride > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("pool: overflow during block alignment rounding");
        return false;
    }

    stride = (stride + pad) & ~pad;

    if (stride == 0) {
        ALLOC_DEBUG_PRINT("pool: stride became zero");
        return false;
    }

    if (nblocks > SIZE_MAX / stride) {
        ALLOC_DEBUG_PRINT("pool: stride * nblocks overflow");
        return false;
    }

    size_t blocks_total = stride * nblocks;

    if (header_size > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("pool: header alignment overflow");
        return false;
    }

    size_t aligned_header = (header_size + pad) & ~pad;

    if (aligned_header > SIZE_MAX - blocks_total) {
        ALLOC_DEBUG_PRINT("pool: total allocation overflow");
        return false;
    }

    return true;
}

static Pool *pool_create_impl(size_t block_size, size_t nblocks, size_t align) {
    if (!pool_validate_params(0, sizeof(Pool), block_size, nblocks, align))
        return NULL;

    size_t pad = align - 1;

    if (block_size < sizeof(void *))
        block_size = sizeof(void *);

    size_t stride = (block_size + pad) & ~pad;
    size_t blocks_total = stride * nblocks;
    size_t aligned_header = (sizeof(Pool) + pad) & ~pad;

    size_t total = aligned_header + blocks_total;

    Pool *p = (Pool *)malloc(total);
    if (!p) {
        ALLOC_DEBUG_PRINT("pool_create failed: malloc failed");
        return NULL;
    }

    uintptr_t start = (uintptr_t)(p + 1);
    uintptr_t aligned = (start + pad) & ~(uintptr_t)pad;

    p->block_size = stride;
    p->capacity = nblocks;
    p->next = NULL;

    ALLOC_POISON_ALLOC(aligned, stride * nblocks);

    p->base = (uint8_t *)aligned;
    p->freelist = (uint8_t *)aligned;

    uint8_t *block = (uint8_t *)p->freelist;

    for (size_t i = 0; i < nblocks - 1; ++i) {
        *(void **)block = block + stride;
        block += stride;
    }

    *(void **)block = NULL;

    return p;
}

static Pool *pool_from_impl(void *buf, size_t block_size, size_t nblocks, size_t align) {
    if (!pool_validate_params((uintptr_t)buf, 0, block_size, nblocks, align))
        return NULL;

    size_t pad = align - 1;

    if (block_size < sizeof(void *))
        block_size = sizeof(void *);

    size_t stride = (block_size + pad) & ~pad;

    size_t raw_capacity = stride * nblocks;

    uintptr_t start = (uintptr_t)buf;
    uintptr_t aligned = (start + pad) & ~(uintptr_t)pad;

    size_t alignment_shift = (size_t)(aligned - start);

    size_t usable = raw_capacity - alignment_shift;
    size_t effective_blocks = usable / stride;

    if (effective_blocks == 0) {
        ALLOC_DEBUG_PRINT("pool_from failed: no blocks fit in provided buffer");
        return NULL;
    }

    Pool *p = (Pool *)malloc(sizeof(Pool));
    if (!p) {
        ALLOC_DEBUG_PRINT("pool_from failed: malloc failed");
        return NULL;
    }

    p->block_size = stride;
    p->capacity = effective_blocks;
    p->next = NULL;

    ALLOC_POISON_ALLOC((void *)aligned, stride * effective_blocks);

    p->base = (uint8_t *)aligned;
    p->freelist = (uint8_t *)aligned;

    uint8_t *block = (uint8_t *)p->freelist;

    for (size_t i = 0; i < effective_blocks - 1; ++i) {
        *(void **)block = block + stride;
        block += stride;
    }

    *(void **)block = NULL;

    return p;
}

Pool *pool_create(size_t block_size, size_t nblocks) {
    if (block_size == 0 || nblocks == 0) return NULL;
    return pool_create_impl(block_size, nblocks, 1);
}

Pool *pool_create_aligned(size_t block_size, size_t nblocks, size_t align) {
    if (block_size == 0 || nblocks == 0) return NULL;
    return pool_create_impl(block_size, nblocks, align);
}

Pool *pool_from(void *buf, size_t block_size, size_t nblocks) {
    if (!buf || block_size == 0 || nblocks == 0) return NULL;
    return pool_from_impl(buf, block_size, nblocks, 1);
}

Pool *pool_from_aligned(void *buf, size_t block_size, size_t nblocks, size_t align) {
    if (!buf || block_size == 0 || nblocks == 0) return NULL;
    return pool_from_impl(buf, block_size, nblocks, align);
}

void *pool_alloc_impl(Pool *pool) {
    if (!pool->freelist) return NULL;
    void *block = pool->freelist;
    pool->freelist = *(void **)block;

    ALLOC_POISON_ALLOC(block, pool->block_size);
    
    return block;
}

void *pool_alloc(Pool *pool) {
    if (!pool) return NULL;
    return pool_alloc_impl(pool);
}

void pool_free(Pool *pool, void *ptr) {
    if (!pool || !ptr) return;

#ifdef ALLOC_DEBUG
    assert(pool_owns(pool, ptr, pool->block_size));
    assert(pool_freed(pool, ptr) == 0); 
#endif

    ALLOC_POISON_FREE(ptr, pool->block_size);

    *(void **)ptr = pool->freelist;
    pool->freelist = ptr;
}

void pool_reset(Pool *pool) {
    while (pool) {
        ALLOC_POISON_ALLOC(pool->base, pool->block_size * pool->capacity);
    
        pool->freelist = pool->base;
        uint8_t *block = (uint8_t *)pool->freelist;
        for (size_t i = 0; i < pool->capacity - 1; ++i) {
            *(void **)block = block + pool->block_size;
            block += pool->block_size;
        }
        *(void **)block = NULL;
        pool = pool->next;
    }
}

void pool_destroy(Pool *pool) {
    while (pool) {
        Pool *next = pool->next;
        free(pool);
        pool = next;
    }
}

size_t pool_numlist(const Pool *pool) {
    size_t count = 0;
    while (pool) {
        count++;
        pool = pool->next;
    }
    return count;
}

static void *pool_alloc_adpt(Allocator *a) {
    Pool *curr = (Pool *)a->tail;

    while (1) {
        void *block = pool_alloc_impl(curr);
        if (block) return block;

        if (curr->next) {
            curr = curr->next;
            continue;
        }

        if (a->grow == GROW_NONE) return NULL;

        size_t next_nblocks = allocator_grow_capacity(curr->capacity, curr->capacity, a->grow);
        if (!next_nblocks) return NULL;

        Pool *next = pool_create_impl(curr->block_size, next_nblocks, a->alignment);
        if (!next) {
            ALLOC_DEBUG_PRINT("pool_alloc failed: allocator failed to create next pool");
            return NULL;
        }

        curr->next = next;
        a->tail = next;
        a->capacity += next->block_size * next_nblocks;
        curr = next;
    }
}

static void pool_free_adpt(Allocator *a, void *ptr) {
    if (!ptr) return;

#ifdef ALLOC_DEBUG
    assert(pool_owns((Pool *)a->head, ptr, a->stride));
    assert(allocator_freed(a, ptr) == 0);
#endif
    
    ALLOC_POISON_FREE(ptr, a->stride);

    *(void **)ptr = a->freelist;
    a->freelist = ptr;
    a->allocations--;
}


static void pool_stats_adpt(const Allocator *a, AllocatorStats *out) {
    memset(out, 0, sizeof(*out));
    out->capacity = a->capacity / a->stride;
    out->used = a->allocations;
    out->freelist = out->capacity - out->used;
    out->segments = allocator_numlist(a);
}

static void pool_reset_adpt(Allocator *a) { pool_reset((Pool *)a->head); }

static size_t pool_numlist_adpt(const Allocator *a) { return pool_numlist((Pool *)a->head); }

static void pool_destroy_adpt(Allocator *a) { pool_destroy((Pool *)a->head); }

static size_t pool_available_adpt(const Allocator *a) {
    Pool *pool = (Pool *)a->tail;
    size_t freeblocks = 0;
    void *curr = pool->freelist;
    while (curr) {
        freeblocks++;
        curr = *(void **)curr;
    }
    return freeblocks;
}

static size_t pool_reusable_adpt(const Allocator *a) {
    size_t freeblocks = 0;
    void *curr = a->freelist;
    while (curr) {
        freeblocks++;
        curr = *(void **)curr;
    }
    return freeblocks;
}

static const AllocatorOps pool_ops = {
    .alloc = pool_alloc_adpt,
    .free  = pool_free_adpt,
    .reset = pool_reset_adpt,
    .stats = pool_stats_adpt,
    .destroy = pool_destroy_adpt,
    .numlist = pool_numlist_adpt,
    .available = pool_available_adpt,
    .reusable  = pool_reusable_adpt
};

/* ===================== BUFFER ===================== */

static bool buffer_validate_params(uintptr_t addr, size_t capacity, size_t align, size_t header_size) {
    size_t pad = align - 1;

    if (align == 0 || (align & pad) != 0) {
        ALLOC_DEBUG_PRINT("buffer: align must be power of two");
        return false;
    }

    if (header_size > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("buffer: header alignment overflow");
        return false;
    }

    if (addr > SIZE_MAX - pad) {
        ALLOC_DEBUG_PRINT("buffer: address alignment overflow");
        return false;
    }

    if (capacity > SIZE_MAX - header_size - pad) {
        ALLOC_DEBUG_PRINT("buffer: total allocation overflow");
        return false;
    }

    return true;
}

static Buffer *buffer_create_impl(size_t capacity, size_t align) {
    if (!buffer_validate_params(0, capacity, align, sizeof(Buffer)))
        return NULL;

    size_t pad = align - 1;
    size_t total = sizeof(Buffer) + capacity + pad;

    Buffer *b = (Buffer *)malloc(total);
    if (!b) {
        ALLOC_DEBUG_PRINT("buffer_create failed: malloc failed");
        return NULL;
    }

    uintptr_t start = (uintptr_t)b + sizeof(*b);
    uintptr_t aligned = (start + pad) & ~((uintptr_t)pad);
    
    ALLOC_POISON_ALLOC(aligned, capacity);

    b->buffer = (uint8_t *)aligned;
    b->capacity = capacity;
    b->used = 0;

    return b;
}

static Buffer *buffer_from_impl(void *buf, size_t capacity, size_t align) {
    if (!buffer_validate_params((uintptr_t)buf, capacity, align, 0))
        return NULL;

    size_t pad = align - 1;
    uintptr_t start = (uintptr_t)buf;
    uintptr_t aligned = (start + pad) & ~((uintptr_t)pad);
    size_t diff = (size_t)(aligned - start);

    if (capacity <= diff) {
        ALLOC_DEBUG_PRINT("buffer_from failed: buffer too small after alignment");
        return NULL;
    }

    size_t usable_capacity = capacity - diff;

    Buffer *b = (Buffer *)malloc(sizeof(Buffer));
    if (!b) {
        ALLOC_DEBUG_PRINT("buffer_from failed: malloc failed");
        return NULL;
    }
    
    ALLOC_POISON_ALLOC(aligned, usable_capacity);

    b->buffer = (uint8_t *)aligned;
    b->capacity = usable_capacity;
    b->used = 0;

    return b;
}

static void *buffer_alloc_impl(Buffer *buf, size_t size, size_t align) {
    size_t pad = align - 1;
    if (align == 0 || (align & pad) != 0) {
        ALLOC_DEBUG_PRINT("buffer_alloc failed: align must be power of two");
        return NULL;
    }

    uintptr_t curr = (uintptr_t)buf->buffer + buf->used;

    if (pad > SIZE_MAX - curr) {
        ALLOC_DEBUG_PRINT("buffer_alloc failed: alignment too large");
        return NULL;
    }
    
    uintptr_t aligned = (curr + pad) & ~(uintptr_t)pad;
    size_t padding = (size_t)(aligned - curr);

    size_t remaining = buf->capacity - buf->used;

    if (padding > remaining || size > remaining - padding)
        return NULL;

    buf->used += padding + size;
    
    ALLOC_POISON_ALLOC(aligned, size);

    return (void *)aligned;
}

static void *buffer_stride_alloc(Buffer *buf, size_t stride) {
    if (stride > buf->capacity - buf->used)
        return NULL;
    void *ptr = buf->buffer + buf->used;

    ALLOC_POISON_ALLOC(ptr, stride);

    buf->used += stride;
    return ptr;
}

Buffer *buffer_create(size_t capacity) {
    if (capacity == 0) return NULL;
    return buffer_create_impl(capacity, 1);
}

Buffer *buffer_create_aligned(size_t capacity, size_t align) {
    if (capacity == 0) return NULL;
    return buffer_create_impl(capacity, align);
}

Buffer *buffer_from(void *buf, size_t capacity) {
    if (!buf || capacity == 0) return NULL;
    return buffer_from_impl(buf, capacity, 1);
}

Buffer *buffer_from_aligned(void *buf, size_t capacity, size_t align) {
    if (!buf || capacity == 0) return NULL;
    return buffer_from_impl(buf, capacity, align);
}

void *buffer_alloc(Buffer *buf, size_t size) {
    if (!buf || size == 0) return NULL;
    return buffer_alloc_impl(buf, size, 1);
}

void *buffer_alloc_aligned(Buffer *buf, size_t size, size_t align) {
    if (!buf || size == 0) return NULL;
    return buffer_alloc_impl(buf, size, align);
}

void buffer_reset(Buffer *buf) {
    if (!buf) return;
    ALLOC_POISON_ALLOC(buf->buffer, buf->capacity);
    buf->used = 0;
}

void buffer_destroy(Buffer *buf) { free(buf); }

size_t buffer_numlist(const Buffer *buf) { return buf ? 1 : 0; }

static void *buffer_alloc_adpt(Allocator *a) {
    void *ptr = buffer_stride_alloc((Buffer *)a->head, a->stride);
    if (ptr) return ptr;
    ALLOC_DEBUG_PRINT("buffer allocator out of memory: allocation failed");
    return NULL;
}

static void buffer_free_adpt(Allocator *a, void *ptr) {
    if (!ptr || a->stride < sizeof(void *)) return;

#ifdef ALLOC_DEBUG
    assert(buffer_owns((Buffer *)a->head, ptr, a->stride));
    assert(allocator_freed(a, ptr) == 0);
#endif

    ALLOC_POISON_FREE(ptr, a->stride);

    *(void **)ptr = a->freelist;
    a->freelist = ptr;
}

static void buffer_stats_adpt(const Allocator *a, AllocatorStats *out) {
    memset(out, 0, sizeof(*out));
    out->capacity = a->capacity / a->stride;
    out->used = a->allocations;
    out->freelist = out->capacity - out->used;
    out->segments = 1;
}

static void buffer_reset_adpt(Allocator *a) {
    buffer_reset((Buffer *)a->head);
    a->freelist = NULL;
}

static size_t buffer_numlist_adpt(const Allocator *a) { return buffer_numlist((Buffer *)a->head); }

static void buffer_destroy_adpt(Allocator *a) { buffer_destroy((Buffer *)a->head); }

static size_t buffer_available_adpt(const Allocator *a) {
    Buffer *buf = (Buffer *)a->head;
    return (buf->capacity - buf->used) / a->stride;
}

static size_t buffer_reusable_adpt(const Allocator *a) {
    size_t freeblocks = 0;
    void *curr = a->freelist;
    while (curr) {
        freeblocks++;
        curr = *(void **)curr;
    }
    return freeblocks;
}

static const AllocatorOps buffer_ops = {
    .alloc = buffer_alloc_adpt,
    .free  = buffer_free_adpt,
    .reset = buffer_reset_adpt,
    .stats = buffer_stats_adpt,
    .destroy = buffer_destroy_adpt,
    .numlist = buffer_numlist_adpt,
    .available = buffer_available_adpt,
    .reusable  = buffer_reusable_adpt
};

/* ===================== FACTORIES ===================== */

Allocator *arena_allocator_create(size_t stride, size_t capacity, size_t align, GrowthPolicy policy) {
    assert((align & (align - 1)) == 0);
    if (stride == 0 || capacity == 0) return NULL;
   
    if (align - 1 > SIZE_MAX - stride) {
        ALLOC_DEBUG_PRINT("arena_allocator_create failed: stride alignment overflow");
        return NULL;
    }

    size_t aligned_stride = (stride + align - 1) & ~(align - 1);

    if (aligned_stride > SIZE_MAX / capacity) {
        ALLOC_DEBUG_PRINT("arena_allocator_create fail: aligned_stride * capacity overflow");
        return NULL;
    }

    Allocator *a = (Allocator *)malloc(sizeof(*a));
    if (!a) return NULL;

    Arena *arena = arena_create_impl(aligned_stride * capacity, align);
    if (!arena) {
        free(a);
        return NULL;
    }

    a->head = arena;
    a->tail = arena;
    a->stride = aligned_stride;
    a->capacity = arena->capacity;
    a->alignment = align;
    a->allocations = 0;
    a->grow = policy;
    a->refcount = 1;
    a->freelist = NULL;
    a->ops = &arena_ops;

    return a;
}

Allocator *pool_allocator_create(size_t block_size, size_t nblocks, size_t align, GrowthPolicy policy) {
    assert((align & (align - 1)) == 0);
    if (block_size == 0 || nblocks == 0) return NULL;

    Allocator *a = (Allocator *)malloc(sizeof(*a));
    if (!a) return NULL;

    Pool *pool = pool_create_impl(block_size, nblocks, align);
    if (!pool) {
        free(a);
        return NULL;
    }

    a->head = pool;
    a->tail = pool;
    a->stride = pool->block_size;
    a->capacity = pool->block_size * nblocks;
    a->alignment = align;
    a->allocations = 0;
    a->grow = policy;
    a->refcount = 1;
    a->freelist = NULL;
    a->ops = &pool_ops;

    return a;
}

Allocator *buffer_allocator_create(size_t stride, size_t capacity, size_t align) {
    assert((align & (align - 1)) == 0);
    if (stride == 0 || capacity == 0) return NULL;
     
    if (align - 1 > SIZE_MAX - stride) {
        ALLOC_DEBUG_PRINT("buffer_allocator_create failed: stride alignment overflow");
        return NULL;
    }

    size_t aligned_stride = (stride + align - 1) & ~(align - 1);

    if (aligned_stride > SIZE_MAX / capacity) {
        ALLOC_DEBUG_PRINT("buffer_allocator_create failed: aligned_stride * capacity overflow");
        return NULL;
    }

    Allocator *a = (Allocator *)malloc(sizeof(*a));
    if (!a) return NULL;

    Buffer *buf = buffer_create_impl(aligned_stride * capacity, align);
    if (!buf) {
        free(a);
        return NULL;
    }

    a->head = buf;
    a->tail = buf;
    a->stride = aligned_stride;
    a->capacity = buf->capacity;
    a->alignment = align;
    a->allocations = 0;
    a->grow = GROW_NONE;
    a->refcount = 1;
    a->freelist = NULL;
    a->ops = &buffer_ops;

    return a;
}

Allocator *allocator_ref(Allocator *a) {
    if (a) {
        a->refcount++;
    }
    return a;
}

static void allocator_unref(Allocator *a) {
    if (!a || !a->ops) return;
    if (a->refcount > 0) {
        a->refcount--;
        if (a->refcount == 0) {
            a->ops->destroy(a);
            free(a);
        }
    }   
}

void *allocator_alloc(Allocator *a) {
    if (!a) return NULL;
    
    void *ptr;
    if (a->freelist) {
        ptr = a->freelist;
        a->freelist = *(void **)ptr;
        ALLOC_POISON_ALLOC(ptr, a->stride);
    } else {
        ptr = a->ops->alloc(a);
    }
    
    if (ptr) a->allocations++;
    return ptr;
}

void allocator_free(Allocator *a, void *ptr) {
    if (!a || !ptr) return;
    
    if (a->allocations == 0) {
        ALLOC_DEBUG_PRINT("FREE WITH ZERO ACTIVE ALLOCATIONS - CORRUPTION");
        return;
    }
    
    ALLOC_POISON_FREE(ptr, a->stride);
    
    *(void **)ptr = a->freelist;
    a->freelist = ptr;
    a->allocations--;
}

void allocator_reset(Allocator *a) {
    if (!a || !a->ops) return;
    
    if (a->allocations == 0) {
        a->ops->reset(a);
    } else {
        ALLOC_DEBUG_PRINT("allocator_reset: active allocations still present");
    }
}

void allocator_destroy(Allocator *a) { allocator_unref(a); }

void allocator_stats(const Allocator *a, AllocatorStats *stats) {
    if (a && stats && a->ops) a->ops->stats(a, stats);
}

size_t allocator_stride(const Allocator *a) { return a ? a->stride : 0; }

size_t allocator_capacity(const Allocator *a) { return a ? a->capacity : 0; }

size_t allocator_alignment(const Allocator *a) { return a ? a->alignment : 0; }

size_t allocator_numlist(const Allocator *a) { return a && a->ops ? a->ops->numlist(a) : 0; }

size_t allocator_available(const Allocator *a) { return a && a->ops ? a->ops->available(a) : 0; }

size_t allocator_reusable(const Allocator *a) { return a && a->ops ? a->ops->reusable(a) : 0; }

static size_t allocator_grow_capacity(size_t curr, size_t need, GrowthPolicy policy) {
    size_t next = 0;

    switch (policy) {
        case GROW_DOUBLE:
            next = curr << 1;
            next = (next < curr) ? SIZE_MAX : next;
            break;

        case GROW_GOLDEN: {
            size_t add = curr >> 1;
            next = curr + add;
            next = (next < curr) ? SIZE_MAX : next;
            break;
        }

        case GROW_HYBRID: {
            size_t add = (curr < (1UL << 20)) ? curr : (1UL << 20);
            next = curr + add;
            next = (next < curr) ? SIZE_MAX : next;
            break;
        }

        case GROW_EXACT:
            next = need;
            break;

        case GROW_NONE:
        default:
            return 0;
    }

    return (next < need) ? need : next;
}
