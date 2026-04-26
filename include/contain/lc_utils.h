/** @file lc_utils.h
 * @brief Internal utilities for container implementations
 *
 * Hashing, bit operations, comparisons, slot operations, aligned allocation.
 * All functions are prefixed with `lc_` to avoid symbol collisions.
 *
 * @warning This header is intended for internal use by libcontain containers.
 *          Public API users should not rely on these functions directly.
 */

#ifndef LC_UTILS_PDR_H
#define LC_UTILS_PDR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* C11 feature detection */
#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 199901L
#endif

/* Fallback for max_align_t if C11 not available */
#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L) 
typedef union {
    long double ld;
    long long ll;
    void *p;
} max_align_t;
#endif

/* C11 _Static_assert or C++ static_assert */
#if defined(__cplusplus)
#define LC_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define LC_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#else
#define LC_STATIC_ASSERT(expr, msg)  extern int lc_static_assert_dummy /* no static assert */
#endif

/* Enable debug output by defining CONTAINER_DEBUG before including this header */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_DEBUG_PRINT(msg) fprintf(stderr, "%s\n", msg)

#define LC_DEBUG_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "libcontain panic: " msg "\n"); \
            abort(); \
        } \
    } while(0)

#else
#define LC_DEBUG_PRINT(msg) ((void)0)
#define LC_DEBUG_CHECK(cond, msg) ((void)0)
#endif

/**
 * @defgroup lc_error Error Codes
 * @brief Unified error codes for libcontain
 * @{
 */

/**
 * @enum lc_Error
 * @brief Return codes for libcontain functions
 *
 * @var LC_OK                 Success
 * @var LC_ERROR              Generic error
 * @var LC_EINVAL             Invalid argument
 * @var LC_ENOMEM             Out of memory
 * @var LC_EBOUNDS            Index out of bounds
 * @var LC_EBUSY              Operation not allowed on non-empty container
 * @var LC_ENOTFOUND          Element not found
 * @var LC_ETYPE              Type mismatch
 * @var LC_EOVERFLOW          Overflow occurred
 * @var LC_EFULL              Container is full
 * @var LC_ENOTSUP            Operation not supported
 */
typedef enum lc_Error {
    LC_OK        = 0,
    LC_ERROR     = -1,
    LC_EINVAL    = -2,
    LC_ENOMEM    = -3,
    LC_EBOUNDS   = -4,
    LC_EBUSY     = -5,
    LC_ENOTFOUND = -6,
    LC_ETYPE     = -7,
    LC_EOVERFLOW = -8,
    LC_EFULL     = -9,
    LC_ENOTSUP   = -10
} lc_Error;

/**
 * @brief Get human-readable error string
 * @param err Error code
 * @return String describing the error
 */
static inline const char *lc_error_str(lc_Error err) {
    switch (err) {
        case LC_OK:         return "success";
        case LC_ERROR:      return "generic error";
        case LC_EINVAL:     return "invalid argument";
        case LC_ENOMEM:     return "out of memory";
        case LC_EBOUNDS:    return "index out of bounds";
        case LC_EBUSY:      return "container not empty";
        case LC_ENOTFOUND:  return "element not found";
        case LC_ETYPE:      return "type mismatch";
        case LC_EOVERFLOW:  return "overflow";
        case LC_EFULL:      return "container full";
        case LC_ENOTSUP:    return "operation not supported";
        default:            return "unknown error";
    }
}

/** @} */ /* end of lc_error group */

/**
 * @defgroup lc_hash Hashing Functions
 * @brief Hash functions for container elements
 * @{
 */

/** @brief Hash function type */
typedef size_t (*lc_Hasher)(const void *key, size_t len);

/**
 * @brief DJB2 hash algorithm (simple, good for short strings)
 * @param key Data to hash
 * @param len Length in bytes
 * @return Hash value
 */
static inline size_t lc_hash_djb2(const void *key, size_t len) {
    const uint8_t *bytes = (const uint8_t *)key;
    size_t hash = 5381;
    for (size_t i = 0; i < len; i++) hash = ((hash << 5) + hash) + bytes[i];
    return hash;
}

/**
 * @brief DJB2 hash for null-terminated strings
 * @param str String to hash
 * @return Hash value
 */
static inline size_t lc_hash_djb2_str(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

/**
 * @brief FNV-1a hash algorithm (good for arbitrary binary data)
 * @param key Data to hash
 * @param len Length in bytes
 * @return Hash value
 */
static inline size_t lc_hash_fnv_1a(const void *key, size_t len) {
    const uint8_t *bytes = (const uint8_t *)key;
    size_t hash = (sizeof(size_t) == 8) ? 0xcbf29ce484222325ULL : 2166136261U;
    size_t prime = (sizeof(size_t) == 8) ? 0x100000001b3ULL : 16777619U;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= prime;
    }
    return hash;
}

/**
 * @brief Mix a hash value to improve distribution
 * @param h Input hash
 * @return Mixed hash value
 */
static inline size_t lc_hash_mix(size_t h) {
    h ^= h >> 23;
    h *= 0x2127599bf4325c37ULL;
    h ^= h >> 47;
    return h;
}

/** @} */

/**
 * @defgroup lc_bit Bit Operations
 * @brief Bit manipulation utilities
 * @{
 */

/**
 * @brief Round up to the next power of two
 * @param x Input value (0 returns 1)
 * @return Next power of two >= x
 */
static inline size_t lc_bit_ceil(size_t x) {
    if (x == 0) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

/**
 * @brief Round down to the previous power of two
 * @param x Input value (0 returns 0)
 * @return Largest power of two <= x
 */
static inline size_t lc_bit_floor(size_t x) {
    if (x == 0) return 0;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x - (x >> 1);
}

/**
 * @brief Check if a value is a power of two
 * @param x Input value
 * @return true if x is a power of two (or zero)
 */
static inline bool lc_is_power_of_two(size_t x) { return x != 0 && (x & (x - 1)) == 0; }

/** @} */

/**
 * @defgroup lc_compare Comparison Functions
 * @brief Element comparison utilities
 * @{
 */

/** @brief Comparator function type */
typedef int (*lc_Comparator)(const void *, const void *);

/**
 * @brief String comparator for pointer-to-string (char**)
 * @param a Pointer to first string pointer
 * @param b Pointer to second string pointer
 * @return strcmp result
 */
static inline int lc_compare_str(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    if (!sa && !sb) return 0;
    if (!sa) return -1;
    if (!sb) return 1;
    return strcmp(sa, sb);
}

/**
 * @brief Direct string comparator (char*)
 * @param a First string
 * @param b Second string
 * @return strcmp result
 */
static inline int lc_compare_str_direct(const void *a, const void *b) {
    const char *sa = (const char *)a;
    const char *sb = (const char *)b;
    if (!sa && !sb) return 0;
    if (!sa) return -1;
    if (!sb) return 1;
    return strcmp(sa, sb);
}

/**
 * @brief Integer comparator
 * @param a First int
 * @param b Second int
 * @return -1, 0, or 1
 */
static inline int lc_compare_int(const void *a, const void *b) {
    int ia = *(const int *)a, ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

/**
 * @brief Float comparator
 * @param a First float
 * @param b Second float
 * @return -1, 0, or 1
 */
static inline int lc_compare_float(const void *a, const void *b) {
    float fa = *(const float *)a, fb = *(const float *)b;
    return (fa > fb) - (fa < fb);
}

/**
 * @brief Double comparator
 * @param a First double
 * @param b Second double
 * @return -1, 0, or 1
 */
static inline int lc_compare_double(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

/**
 * @brief Char comparator
 * @param a First char
 * @param b Second char
 * @return -1, 0, or 1
 */
static inline int lc_compare_char(const void *a, const void *b) {
    char ca = *(const char *)a, cb = *(const char *)b;
    return (ca > cb) - (ca < cb);
}

/** @} */

/**
 * @defgroup lc_slot Slot Operations
 * @brief Memory slot management for containers
 * @{
 */

/**
 * @brief Get pointer to slot at offset
 * @param base Base pointer
 * @param offset Byte offset
 * @return Pointer to slot
 */
static inline void *lc_slot_at(void *base, size_t offset) { return (uint8_t *)base + offset; }

/**
 * @brief Compare two items (already dereferenced by slot_get)
 * @param a     First item
 * @param b     Second item
 * @param size  Item size (0 = string mode)
 * @param cmp   Comparator function (NULL = memcmp/strcmp)
 * @return Comparison result
 */
static inline int lc_slot_cmp(const void *a, const void *b, size_t size, lc_Comparator cmp) {
    if (cmp) return cmp(a, b);
    if (size == 0) return strcmp((const char *)a, (const char *)b);
    return memcmp(a, b, size);
}

/**
 * @brief Hash an item (already dereferenced by slot_get)
 * @param item    Item to hash
 * @param size    Item size (0 = string mode)
 * @param hash_fn Hash function (NULL = use default)
 * @return Hash value
 * 
 * Default hash behavior:
 * - size == 0: DJB2 string hash
 * - size 1-8: Direct integer hash (identity)
 * - otherwise: FNV-1a
 */
static inline size_t lc_slot_hash(const void *item, size_t size, lc_Hasher hash_fn) {
    if (hash_fn) return hash_fn(item, size);
    if (size == 0) return lc_hash_djb2_str((const char *)item);  // String mode
    
    switch (size) {
        case 1:  return *(const uint8_t *)item;
        case 2:  return *(const uint16_t *)item;
        case 4:  return *(const uint32_t *)item;
        case 8:  return *(const uint64_t *)item;
        default: return lc_hash_fnv_1a(item, size);
    }
}

/**
 * @brief Fast 64-bit to 64-bit integer hash (Thomas Wang)
 */
static inline size_t lc_mix(uint64_t key) {
    key = (~key) + (key << 21);
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8);
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return (size_t)key;
}

/**
 * @brief Get length of item (for strings, returns strlen)
 * @param item Item pointer
 * @param size Item size (0 = string mode)
 * @return Length in bytes
 */
static inline size_t lc_slot_len(const void *item, size_t size) {
    if (size == 0) {
        const char *str = (const char *)item;
        return str ? strlen(str) : 0;
    }
    return size;
}

/**
 * @brief Resolve slot to actual data pointer (dereferences for strings)
 * @param slot Slot pointer
 * @param size Item size (0 = string mode)
 * @return Pointer to actual data
 */
static inline void *lc_slot_get(void *slot, size_t size) {
    if (size == 0) return *(void **)slot;
    return slot;
}

/**
 * @brief Set slot data (val is raw input, not dereferenced)
 * @param slot Destination slot
 * @param val  Source value
 * @param size Item size (0 = string mode)
 * @return LC_OK on success, error code on failure
 */
static inline lc_Error lc_slot_set(void *slot, const void *val, size_t size) {
    if (size == 0) {
        char *dup = strdup((const char *)val);
        if (!dup) return LC_ENOMEM;
        free(*(void **)slot);
        *(void **)slot = dup;
    } else {
        memcpy(slot, val, size);
    }
    return LC_OK;
}

/**
 * @brief Initialize a new slot (val is raw input)
 * @param slot Destination slot
 * @param val  Source value
 * @param size Item size (0 = string mode)
 * @return LC_OK on success, error code on failure
 */
static inline lc_Error lc_slot_init(void *slot, const void *val, size_t size) {
    if (size == 0) {
        char *dup = strdup((const char *)val);
        if (!dup) return LC_ENOMEM;
        *(void **)slot = dup;
    } else {
        memcpy(slot, val, size);
    }
    return LC_OK;
}

/**
 * @brief Copy slot data (src is raw slot, not dereferenced)
 * @param dst Destination slot
 * @param src Source slot
 * @param size Item size (0 = string mode)
 * @return LC_OK on success, error code on failure
 */
static inline lc_Error lc_slot_copy(void *dst, const void *src, size_t size) {
    if (size == 0) {
        const char *src_str = *(const char **)src;
        if (!src_str) {
            *(char **)dst = NULL;
            return LC_OK;
        }
        char *new_str = strdup(src_str);
        if (!new_str) return LC_ENOMEM;
        *(char **)dst = new_str;
    } else {
        memcpy(dst, src, size);
    }
    return LC_OK;
}

/**
 * @brief Free slot data (slot is raw slot pointer)
 * @param slot Slot to free
 * @param size Item size (0 = string mode)
 */
static inline void lc_slot_free(void *slot, size_t size) {
    if (size == 0) {
        void *ptr = *(void **)slot;
        if (ptr) free(ptr);
    }
}

/** @} */

/**
 * @defgroup lc_alloc Aligned Memory Allocation
 * @brief Memory allocation with alignment support
 * @{
 */

/** @brief Internal allocation header */
typedef struct {
    void *base;
    size_t capacity;
} lc_AllocHeader;

/**
 * @brief Free memory allocated by lc_alloc_malloc
 * @param ptr Pointer to free
 */
static inline void lc_alloc_free(void *ptr) {
    if (ptr) free(((lc_AllocHeader *)ptr - 1)->base);
}

/**
 * @brief Allocate memory with tracking header
 * @param size Size in bytes
 * @return Pointer to usable memory, or NULL on failure
 */
static inline void *lc_alloc_malloc(size_t size) {
    const size_t overhead = sizeof(lc_AllocHeader);
    if (size > (SIZE_MAX - overhead)) return NULL;

    uint8_t *raw = (uint8_t *)malloc(size + overhead);
    if (!raw) return NULL;

    lc_AllocHeader *hdr = (lc_AllocHeader *)raw;
    hdr->base = raw;
    hdr->capacity = size;

    return (void *)(raw + overhead);
}

/**
 * @brief Allocate and zero-initialize memory
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to usable memory, or NULL on failure
 */
static inline void *lc_alloc_calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (nmemb != 0 && total / nmemb != size) return NULL;

    void *ptr = lc_alloc_malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

/**
 * @brief Reallocate memory
 * @param ptr      Existing pointer (can be NULL)
 * @param new_size New size in bytes
 * @return Pointer to usable memory, or NULL on failure
 */
static inline void *lc_alloc_realloc(void *ptr, size_t new_size) {
    if (!ptr) return lc_alloc_malloc(new_size);
    if (new_size == 0) {
        lc_alloc_free(ptr);
        return NULL;
    }

    lc_AllocHeader *old_hdr = (lc_AllocHeader *)ptr - 1;
    void *old_base = old_hdr->base;
    const size_t overhead = sizeof(lc_AllocHeader);

    void *new_raw = realloc(old_base, new_size + overhead);
    if (!new_raw) return NULL;

    lc_AllocHeader *new_hdr = (lc_AllocHeader *)new_raw;
    new_hdr->base = new_raw;
    new_hdr->capacity = new_size;
    return (void *)((uint8_t *)new_raw + overhead);
}

/**
 * @brief Allocate aligned memory
 * @param size  Size in bytes
 * @param align Alignment (must be power of two)
 * @return Aligned pointer, or NULL on failure
 */
static inline void *lc_alloc_malloc_aligned(size_t size, size_t align) {
    const size_t overhead = align + sizeof(lc_AllocHeader);
    if (size > (SIZE_MAX - overhead)) return NULL;

    uint8_t *raw = (uint8_t *)malloc(size + overhead);
    if (!raw) return NULL;

    uintptr_t aligned = ((uintptr_t)raw + overhead - 1) & ~(uintptr_t)(align - 1);
    lc_AllocHeader *hdr = (lc_AllocHeader *)aligned - 1;
    hdr->base = raw;
    hdr->capacity = size;

    return (void *)aligned;
}

/**
 * @brief Reallocate aligned memory
 * @param ptr      Existing pointer (can be NULL)
 * @param new_size New size in bytes
 * @param align    Alignment (must be power of two)
 * @return Aligned pointer, or NULL on failure
 */
static inline void *lc_alloc_realloc_aligned(void *ptr, size_t new_size, size_t align) {
    if (!ptr) return lc_alloc_malloc_aligned(new_size, align);
    if (new_size == 0) {
        lc_alloc_free(ptr);
        return NULL;
    }

    lc_AllocHeader *old_hdr = (lc_AllocHeader *)ptr - 1;
    void *old_base = old_hdr->base;
    size_t old_cap = old_hdr->capacity;
    size_t old_off = (size_t)((uintptr_t)ptr - (uintptr_t)old_base);
    const size_t overhead = align + sizeof(lc_AllocHeader);

    if (new_size > (SIZE_MAX - overhead)) return NULL;
    void *new_raw = realloc(old_base, new_size + overhead);
    if (!new_raw) return NULL;

    uintptr_t new_ptr = ((uintptr_t)new_raw + overhead - 1) & ~(uintptr_t)(align - 1);
    size_t new_off = (size_t)(new_ptr - (uintptr_t)new_raw);

    if (new_off != old_off) {
        memmove((void *)new_ptr, (uint8_t *)new_raw + old_off, old_cap);
    }

    lc_AllocHeader *new_hdr = (lc_AllocHeader *)new_ptr - 1;
    new_hdr->base = new_raw;
    new_hdr->capacity = new_size;
    return (void *)new_ptr;
}

/**
 * @brief Get the maximum natural alignment for the platform
 * @return Alignment in bytes
 */
static inline size_t lc_alloc_max_align(void) {
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    return alignof(max_align_t);
#else
    return 16;
#endif
}

/** @} */

#endif /* LC_UTILS_PDR_H */