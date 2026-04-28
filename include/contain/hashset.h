/** @file hashset.h
 * @brief Generic Hash Set with Separate Chaining
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * A type-safe, generic hash set implementation supporting arbitrary item types.
 * Uses separate chaining (linked buckets) for collision resolution with
 * automatic load-factor-based rehashing.
 *
 * @section key_features Key Features
 *   - @ref type_agnostic — stores any POD type via void pointers and stride
 *   - @ref automatic_rehashing — expands at 0.75 load factor, shrinks at 0.25
 *   - @ref custom_allocators — pluggable memory management via Allocator interface
 *   - @ref custom_hash_compare — user-defined hashers and comparators
 *   - @ref string_support — first-class string type (hashset_str)
 *   - @ref alignment_control — per-type item alignment specification
 *   - @ref container_interface — implements the Container vtable for generic use
 *   - @ref set_operations — union, intersection, difference via hashset_merge variants
 *
 * @section load_factor Load Factor Management
 *   - **Expansion**: triggered when len / capacity > 0.75 (len * 4 > cap * 3)
 *   - **Shrinking**: triggered when len / capacity < 0.25 (len * 4 < cap)
 *   - **Minimum capacity**: 16 buckets (HASHSET_MIN_CAPACITY)
 *   - All capacities are powers of two for fast bitmasking
 *
 * @section thread_safety Thread Safety
 *   Not thread-safe. Concurrent reads with one writer are safe only
 *   if no reallocation occurs. External synchronization required for
 *   concurrent writes.
 *
 * @section ownership Ownership
 *   - The HashSet takes ownership of inserted item data — it copies via slot_*.
 *   - For strings, it strdup()s the input; caller still owns the original.
 *   - On destruction, all copied items (and strings) are freed.
 *
 * @section usage Usage Example
 * @code
 *   #include "hashset.h"
 *
 *   HashSet *set = hashset_create(sizeof(int));
 *   int v = 42;
 *   hashset_insert(set, &v);
 *   if (hashset_contains(set, &v)) { ... }
 *   hashset_destroy(set);
 * @endcode
 */

#ifndef HASHSET_PDR_H
#define HASHSET_PDR_H

#include "allocator.h"
#include "container.h"

/**
 * @defgroup hashset_constants HashSet Constants
 * @{
 */

/** @brief Minimum number of buckets for a new hash set (16) */
#define HASHSET_MIN_CAPACITY   16

/** @brief Default allocation capacity for pool allocator */
#define HASHSET_ALLOC_CAPACITY 16

/** @brief Default alignment for pool allocator */
#define HASHSET_ALLOC_ALIGN    8

/** @} */

/**
 * @defgroup hashset_types HashSet Types
 * @brief Core types for the hash set container
 * @{
 */

/**
 * @struct HashSetImpl
 * @brief Opaque implementation structure for hash set internals
 */
typedef struct HashSetImpl HashSetImpl;

/**
 * @struct HashSet
 * @brief Hash set container with separate chaining
 *
 * @var HashSet::container
 * Common container header (items, len, capacity, ops)
 *
 * @var HashSet::alloc
 * Pool allocator for entries (reduces malloc overhead)
 *
 * @var HashSet::cmp
 * Comparator function for elements (NULL = memcmp for fixed-size, strcmp for strings)
 *
 * @var HashSet::hash
 * Hash function for elements (NULL = FNV-1a)
 *
 * @var HashSet::impl
 * Opaque pointer to implementation details
 */
typedef struct HashSet {
    Container  container;       /**< Common container header */
    Allocator *alloc;           /**< Pool allocator for entries */
    lc_Comparator cmp;          /**< Element comparator (NULL = default) */
    lc_Hasher     hash;         /**< Hash function (NULL = FNV-1a) */
    const HashSetImpl *impl;    /**< Opaque implementation data */
} HashSet;

/**
 * @struct HashSetBuilder
 * @brief Fluent configuration builder for hash set creation
 *
 * @var HashSetBuilder::item_size
 * Size of each element in bytes (0 = string mode, stores char*)
 *
 * @var HashSetBuilder::item_align
 * Alignment for items (0 = default, must be power of two)
 *
 * @var HashSetBuilder::capacity
 * Initial bucket count (rounded to power of two)
 *
 * @var HashSetBuilder::alloc
 * Custom allocator (NULL = internal pool allocator)
 *
 * @var HashSetBuilder::cmp
 * Item comparator (NULL = memcmp for fixed-size, strcmp for strings)
 *
 * @var HashSetBuilder::hash
 * Hash function (NULL = FNV-1a)
 */
typedef struct {
    size_t item_size;      /**< 0 = string mode (stores char*) */
    size_t item_align;     /**< Alignment for items (0 = default) */
    size_t capacity;       /**< Initial bucket count (rounded to power of 2) */
    Allocator *alloc;      /**< Custom allocator (NULL = internal pool) */
    lc_Comparator cmp;     /**< Item comparator (NULL = memcmp/strcmp) */
    lc_Hasher hash;        /**< Item hash function (NULL = FNV-1a) */
} HashSetBuilder;

/** @} */

/**
 * @defgroup hashset_creation Creation
 * @brief Functions for creating and destroying hash sets
 * @{
 */

/**
 * @brief Create a new hash set with default capacity
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @return Newly allocated hash set, or NULL on allocation failure
 *
 * @warning The caller owns the returned hash set and must destroy it with hashset_destroy()
 */
HashSet *hashset_create(size_t item_size);

/**
 * @brief Create a new hash set with specified initial bucket count
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param capacity  Initial bucket count (rounded to power of two)
 * @return Newly allocated hash set, or NULL on allocation failure
 */
HashSet *hashset_create_with_capacity(size_t item_size, size_t capacity);

/**
 * @brief Create a new hash set with a custom hash function
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param hasher    Hash function
 * @return Newly allocated hash set, or NULL on allocation failure
 */
HashSet *hashset_create_with_hasher(size_t item_size, lc_Hasher hasher);

/**
 * @brief Create a new hash set with a custom comparator
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param cmp       Element comparator function
 * @return Newly allocated hash set, or NULL on allocation failure
 */
HashSet *hashset_create_with_comparator(size_t item_size, lc_Comparator cmp);

/**
 * @brief Create a new hash set with a custom allocator
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param alloc     Custom allocator
 * @return Newly allocated hash set, or NULL on allocation failure
 */
HashSet *hashset_create_with_allocator(size_t item_size, Allocator *alloc);

/**
 * @brief Create a new hash set with aligned items
 *
 * Useful for SIMD, atomic operations, or cache-line optimisation.
 *
 * @param item_size  Size of each element in bytes (0 for string mode)
 * @param item_align Alignment for items (must be power of two)
 * @return Newly allocated hash set, or NULL on allocation failure
 */
HashSet *hashset_create_aligned(size_t item_size, size_t item_align);

/**
 * @brief Create a new string hash set
 *
 * Stores C strings as char* with automatic strdup on insertion.
 *
 * @return Newly allocated string hash set, or NULL on allocation failure
 */
HashSet *hashset_str(void);

/**
 * @brief Create a new string hash set with specified initial bucket count
 *
 * @param capacity Initial bucket count
 * @return Newly allocated string hash set, or NULL on allocation failure
 */
HashSet *hashset_str_with_capacity(size_t capacity);

/**
 * @brief Create a new string hash set with a custom hash function
 *
 * @param hasher Hash function
 * @return Newly allocated string hash set, or NULL on allocation failure
 */
HashSet *hashset_str_with_hasher(lc_Hasher hasher);

/**
 * @brief Create a new string hash set with a custom comparator
 *
 * @param cmp String comparator function (receives char**)
 * @return Newly allocated string hash set, or NULL on allocation failure
 */
HashSet *hashset_str_with_comparator(lc_Comparator cmp);

/**
 * @brief Create a new string hash set with a custom allocator
 *
 * @param alloc Custom allocator
 * @return Newly allocated string hash set, or NULL on allocation failure
 */
HashSet *hashset_str_with_allocator(Allocator *alloc);

/**
 * @brief Build a hash set using a fluent builder
 *
 * @param b Initialised HashSetBuilder
 * @return Newly allocated hash set, or NULL on configuration or allocation failure
 *
 * @par Example
 * @code
 *   HashSet *set = hashset_builder_build(
 *       hashset_builder_hasher(
 *           hashset_builder_capacity(
 *               hashset_builder(sizeof(int)), 1024),
 *           my_hash_function));
 * @endcode
 */
HashSet *hashset_builder_build(HashSetBuilder b);

/**
 * @brief Destroy a hash set and free all resources
 *
 * Frees all elements (including strings in string mode), buckets, and allocator.
 *
 * @param set Hash set to destroy (can be NULL)
 */
void hashset_destroy(HashSet *set);

/**
 * @brief Set the hash function for a hash set
 *
 * @param set    Hash set to modify
 * @param hasher New hash function
 * @return LC_OK on success, LC_EINVAL if set or hasher is NULL,
 *         LC_EBUSY if hash set is not empty
 *
 * @note Only allowed on empty hash sets.
 */
int hashset_set_hasher(HashSet *set, lc_Hasher hasher);

/**
 * @brief Set the comparator for a hash set
 *
 * @param set Hash set to modify
 * @param cmp New comparator function
 * @return LC_OK on success, LC_EINVAL if set or cmp is NULL,
 *         LC_EBUSY if hash set is not empty
 *
 * @note Only allowed on empty hash sets.
 */
int hashset_set_comparator(HashSet *set, lc_Comparator cmp);

/**
 * @brief Set the allocator for a hash set
 *
 * @param set   Hash set to modify
 * @param alloc New allocator
 * @return LC_OK on success, LC_EINVAL if set or alloc is NULL,
 *         LC_ETYPE if allocator stride is insufficient,
 *         LC_EBUSY if hash set is not empty
 *
 * @note Only allowed on empty hash sets.
 */
int hashset_set_allocator(HashSet *set, Allocator *alloc);

/** @} */

/**
 * @defgroup hashset_core Core Operations
 * @brief Insert, remove, and query elements
 * @{
 */

/**
 * @brief Insert an element into the hash set
 *
 * If the element already exists, this is a no-op.
 *
 * @param set  Hash set to modify
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL if set or item is NULL,
 *         LC_ENOMEM on allocation failure
 *
 * @par Example
 * @code
 *   int value = 42;
 *   hashset_insert(set, &value);
 * @endcode
 */
int hashset_insert(HashSet *set, const void *item);

/**
 * @brief Remove an element from the hash set
 *
 * @param set  Hash set to modify
 * @param item Element to remove
 * @return LC_OK on success, LC_EINVAL if set or item is NULL,
 *         LC_ENOTFOUND if element not found
 */
int hashset_remove(HashSet *set, const void *item);

/**
 * @brief Check if an element exists in the hash set
 *
 * @param set  Hash set to query
 * @param item Element to find
 * @return true if element exists, false otherwise
 */
bool hashset_contains(const HashSet *set, const void *item);

/**
 * @brief Merge another hash set into this one
 *
 * All elements from src are inserted into dst. Duplicates are ignored.
 *
 * @param dst Destination hash set
 * @param src Source hash set
 * @return LC_OK on success, LC_EINVAL if dst or src is NULL,
 *         LC_ETYPE if item sizes differ, LC_ENOMEM on allocation failure
 */
int hashset_merge(HashSet *dst, const HashSet *src);

/**
 * @brief Reserve capacity for expected number of elements
 *
 * Pre-allocates buckets to avoid rehashing.
 *
 * @param set           Hash set to reserve
 * @param expected_size Expected number of elements
 * @return LC_OK on success, LC_EINVAL if set is NULL,
 *         LC_EOVERFLOW on overflow, LC_ENOMEM on allocation failure
 */
int hashset_reserve(HashSet *set, size_t expected_size);

/**
 * @brief Remove all elements from the hash set
 *
 * Capacity (bucket count) is retained.
 *
 * @param set Hash set to clear
 * @return LC_OK on success, LC_EINVAL if set is NULL
 */
int hashset_clear(HashSet *set);

/**
 * @brief Shrink bucket count to fit current load factor
 *
 * Reduces memory usage by rehashing to a smaller bucket array if
 * the load factor drops below 0.25.
 *
 * @param set Hash set to shrink
 * @return LC_OK on success, LC_EINVAL if set is NULL
 */
int hashset_shrink_to_fit(HashSet *set);

/** @} */

/**
 * @defgroup hashset_queries Queries
 * @brief Query hash set properties
 * @{
 */

/**
 * @brief Check if the hash set is empty
 *
 * @param set Hash set to query
 * @return true if set is NULL or contains no elements, false otherwise
 */
bool hashset_is_empty(const HashSet *set);

/**
 * @brief Check if A is a subset of B
 *
 * @param a Potential subset
 * @param b Potential superset
 * @return true if every element in a is also in b, false otherwise
 */
bool hashset_subset(const HashSet *a, const HashSet *b);

/**
 * @brief Check if two hash sets are equal
 *
 * Two hash sets are equal if they contain the same elements,
 * regardless of insertion order.
 *
 * @param a First hash set
 * @param b Second hash set
 * @return true if sets are equal, false otherwise
 */
bool hashset_equals(const HashSet *a, const HashSet *b);

/**
 * @brief Get the number of elements in the hash set
 *
 * @param set Hash set to query
 * @return Number of elements, or 0 if set is NULL
 */
size_t hashset_len(const HashSet *set);

/**
 * @brief Get the current bucket count of the hash set
 *
 * @param set Hash set to query
 * @return Current bucket count, or 0 if set is NULL
 */
size_t hashset_capacity(const HashSet *set);

/**
 * @brief Compute a hash of the hash set's contents
 *
 * The hash is order-independent — equal sets with different insertion
 * orders produce the same hash. The hash is cached and invalidated on
 * any mutation.
 *
 * @param set Hash set to hash
 * @return Hash value, or 0 if set is NULL or empty
 */
size_t hashset_hash(const HashSet *set);

/**
 * @brief Get the allocator used by the hash set
 *
 * @param set Hash set to query
 * @return Allocator pointer, or NULL if set is NULL
 */
Allocator *hashset_allocator(const HashSet *set);

/** @} */

/**
 * @defgroup hashset_copy Copy Operations
 * @brief Create new hash sets from existing ones
 * @{
 */

/**
 * @brief Create a new empty hash set of the same type
 *
 * @param set Source hash set (determines item size, alignment, comparator, hasher)
 * @return New empty hash set, or NULL on allocation failure
 */
HashSet *hashset_instance(const HashSet *set);

/**
 * @brief Create a deep copy of a hash set
 *
 * For string hash sets, all strings are strdup'd.
 *
 * @param set Source hash set
 * @return Deep copy of the hash set, or NULL on allocation failure
 */
HashSet *hashset_clone(const HashSet *set);

/** @} */

/**
 * @defgroup hashset_set_ops Set Operations
 * @brief Mathematical set operations (union, intersection, difference)
 * @{
 */

/**
 * @brief Create a new hash set containing the union of two sets
 *
 * Result contains all elements that appear in either A or B.
 *
 * @param a First hash set
 * @param b Second hash set
 * @return New hash set with the union, or NULL on allocation failure
 *
 * @par Example
 * @code
 *   HashSet *union = hashset_union(set_a, set_b);
 * @endcode
 */
HashSet *hashset_union(const HashSet *a, const HashSet *b);

/**
 * @brief Create a new hash set containing the intersection of two sets
 *
 * Result contains only elements that appear in both A and B.
 *
 * @param a First hash set
 * @param b Second hash set
 * @return New hash set with the intersection, or NULL on allocation failure
 *
 * @par Example
 * @code
 *   HashSet *intersection = hashset_intersection(set_a, set_b);
 * @endcode
 */
HashSet *hashset_intersection(const HashSet *a, const HashSet *b);

/**
 * @brief Create a new hash set containing the difference of two sets
 *
 * Result contains elements that appear in A but not in B.
 *
 * @param a First hash set (minuend)
 * @param b Second hash set (subtrahend)
 * @return New hash set with the difference (A \ B), or NULL on allocation failure
 *
 * @par Example
 * @code
 *   HashSet *diff = hashset_difference(set_a, set_b);
 * @endcode
 */
HashSet *hashset_difference(const HashSet *a, const HashSet *b);

/** @} */

/**
 * @defgroup hashset_conversion Conversion & Iteration
 * @brief Export to Array and create iterators
 * @{
 */

/**
 * @brief Export the hash set to a flat Array snapshot
 *
 * The Array header and data are in a single allocation — free with free(arr).
 * Order is unspecified (iteration order).
 *
 * @param set Hash set to export
 * @return Array snapshot, or NULL if set is NULL or empty or allocation fails
 */
Array *hashset_to_array(const HashSet *set);

/**
 * @brief Create a forward iterator over the hash set
 *
 * Order is unspecified. Only forward iteration is supported
 * (reverse iteration returns no elements).
 *
 * @param set Hash set to iterate
 * @return Forward iterator (stack-allocated)
 */
Iterator hashset_iter(const HashSet *set);

/** @} */

#endif /* HASHSET_PDR_H */

/**
 * @defgroup hashset_implementation Implementation
 * @brief STB-style implementation section
 *
 * To include the implementation, define HASHSET_IMPLEMENTATION in ONE .c file
 * before including this header:
 *
 * @code
 *   #define HASHSET_IMPLEMENTATION
 *   #include "hashset.h"
 * @endcode
 *
 * @note The implementation is intentionally not documented in detail as it is
 *       meant to be included only once per project.
 */

#ifdef HASHSET_IMPLEMENTATION
    #define ALLOCATOR_IMPLEMENTATION
    #include "allocator.h"
#ifndef HASHSET_IMPLEMENTATION_ONCE
#define HASHSET_IMPLEMENTATION_ONCE

/* ============================================================================
 * HashSet — Implementation
 * ============================================================================
 *
 * Important Notes:
 *   • Uses separate chaining with linked buckets for collision resolution
 *   • Load factor triggers: expand at 0.75, shrink at 0.25
 *   • All capacities are powers of two for fast bitmask indexing
 *   • String mode (item_size == 0) stores char* pointers, each strdup'd
 *   • Pool allocator manages entries in contiguous blocks for cache efficiency
 *   • Cached hash invalidated on any mutation
 *   • Set operations (union/intersection/difference) create new sets
 * ============================================================================ */
 
/* -------------------------------------------------------------------------
 * Container vtable
 * ------------------------------------------------------------------------- */
static const void *hashset_next(Iterator *iter);
static Container *hashset_instance_adpt(const Container *set);
static Container *hashset_clone_adpt(const Container *set);
static Array *hashset_to_array_adpt(const Container *set);
static size_t hashset_hash_adpt(const Container *set);
static bool hashset_insert_adpt(Container *set, const void *item);
static void hashset_clear_adpt(Container *set);
static void hashset_destroy_adpt(Container *set);

static const ContainerVTable HASHSET_CONTAINER_OPS = {
    .next     = hashset_next,
    .instance = hashset_instance_adpt,
    .clone    = hashset_clone_adpt,
    .as_array = hashset_to_array_adpt,
    .hash     = hashset_hash_adpt,
    .insert   = hashset_insert_adpt,
    .clear    = hashset_clear_adpt,
    .destroy  = hashset_destroy_adpt
};

/* -------------------------------------------------------------------------
 * Internal structures
 * ------------------------------------------------------------------------- */
struct HashSetImpl {
    uint16_t item_offset;    /* offset from entry start to item data */
    uint16_t item_size;      /* 0 = string mode */
    uint32_t stride;         /* total entry size (including next pointer) */
};

/* Hash entry stored in buckets */
typedef struct HashSetEntry {
    struct HashSetEntry *next;
    uint8_t data[];          /* flexible array for item storage */
} HashSetEntry;

/* Layout calculation result */
typedef struct {
    uint16_t item_offset;
    uint32_t stride;
} HashSetEntryLayout;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Create a new entry, copy item into it */
static HashSetEntry *hashset_entry_create(HashSet *set, const void *item) {
    HashSetEntry *entry = (HashSetEntry *)allocator_alloc(set->alloc);
    if (!entry) return NULL;

    HashSetImpl *impl = (HashSetImpl *)set->impl;
    
    void *item_slot = lc_slot_at(entry->data, impl->item_offset);
    
    if (lc_slot_init(item_slot, item, impl->item_size) != LC_OK) {
        allocator_free(set->alloc, entry);
        return NULL;
    }

    entry->next = NULL;
    set->container.len++;
    
    return entry;
}

/* Free entry and its contained item */
static void hashset_entry_free(HashSet *set, HashSetEntry *entry) {
    HashSetImpl *impl = (HashSetImpl *)set->impl;

    void *item_slot = lc_slot_at(entry->data, impl->item_offset);
    lc_slot_free(item_slot, impl->item_size);

    allocator_free(set->alloc, entry);
    set->container.len--;
}

/* Rehash entire set to new capacity (must be power of two) */
static int hashset_rehash(HashSet *set, size_t new_capacity) {
    HashSetEntry **new_buckets = (HashSetEntry **)calloc(new_capacity, sizeof(void *));
    if (!new_buckets) return LC_ENOMEM;

    HashSetEntry **old_buckets = (HashSetEntry **)set->container.items;
    const HashSetImpl *impl = set->impl;
    
    const size_t old_cap = set->container.capacity;
    const size_t mask = new_capacity - 1;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Hasher hash_fn = set->hash;
    
    /* Move all entries to new buckets */
    for (size_t i = 0; i < old_cap; i++) {
        HashSetEntry *entry = old_buckets[i];
        while (entry) {
            HashSetEntry *next = entry->next;

            void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
            size_t h = lc_slot_hash(item, item_sz, hash_fn);
            size_t b = h & mask;

            entry->next = new_buckets[b];
            new_buckets[b] = entry;
            
            entry = next;
        }
    }

    free(old_buckets);
    set->container.items = (void **)new_buckets;
    set->container.capacity = new_capacity;

    return LC_OK;
}

/* Adjust capacity based on load factor (expand at 0.75, shrink at 0.25) */
static int hashset_adjust_capacity(HashSet *set, bool allow_shrink) {
    const size_t len = set->container.len;
    const size_t cap = set->container.capacity;

    /* Expand: load > 0.75 */
    if (len > SIZE_MAX / 4) return LC_EOVERFLOW;
    if (len * 4 > cap * 3) {
        size_t min_cap = (len * 4 + 2) / 3;
        size_t target = lc_bit_ceil(min_cap);
        if (target < min_cap) return LC_EOVERFLOW;
        return hashset_rehash(set, target);
    }

    /* Shrink: only if allowed (on remove or explicit shrink) */
    if (allow_shrink && cap > HASHSET_MIN_CAPACITY && len * 4 < cap) {
        size_t min_cap = (len * 4 + 2) / 3;
        if (min_cap < HASHSET_MIN_CAPACITY) min_cap = HASHSET_MIN_CAPACITY;
        size_t target = lc_bit_ceil(min_cap);
        if (target < min_cap) return LC_EOVERFLOW;
        if (target < cap) {
            return hashset_rehash(set, target);
        }
    }

    return LC_OK;
}

/* Compute entry layout: item offset and total stride */
static bool hashset_compute_layout(size_t item_sz, size_t item_align, HashSetEntryLayout *out) {
    if (item_align == 0 || (item_align & (item_align - 1)) != 0) return false;

    const size_t ptr_size = sizeof(void *);

    /* String mode: store as char* pointer */
    if (item_sz == 0) {
        item_sz = ptr_size;
        item_align = ptr_size;
    }

    /* Item starts after next pointer, aligned to item_align */
    size_t item_start = (ptr_size + item_align - 1) & ~(item_align - 1);
    
    /* Total stride = item_start + item_size */
    size_t stride = item_start + item_sz;
    
    /* Offset from data start to item (data starts after next pointer) */
    size_t item_offset = item_start - ptr_size;

    if (item_offset > UINT16_MAX || stride > UINT32_MAX) return false;

    out->item_offset = (uint16_t)item_offset;
    out->stride = (uint32_t)stride;
    return true;
}

/* -------------------------------------------------------------------------
 * Creation helpers
 * ------------------------------------------------------------------------- */
typedef struct {
    HashSet *set;
    void **buckets;
    Allocator *alloc;
} HashSetCreateCtx;

static void hashset_create_ctx_cleanup(HashSetCreateCtx *ctx) {
    if (ctx->alloc) allocator_destroy(ctx->alloc);
    if (ctx->buckets) free(ctx->buckets);
    if (ctx->set) free(ctx->set);
}

static HashSet *hashset_create_impl(const HashSetBuilder *cfg, const HashSetEntryLayout *layout) {
    HashSetCreateCtx ctx = {0};
    
    ctx.set = (HashSet *)malloc(sizeof(HashSet) + sizeof(HashSetImpl));
    if (!ctx.set) return NULL;

    size_t capacity = lc_bit_ceil(cfg->capacity);
    ctx.buckets = (void **)calloc(capacity, sizeof(void *));
    if (!ctx.buckets) {
        hashset_create_ctx_cleanup(&ctx);
        return NULL;
    }

    if (cfg->alloc) {
        ctx.alloc = allocator_ref(cfg->alloc);
    } else {
        size_t max_align = cfg->item_align > sizeof(void *) ? cfg->item_align : sizeof(void *);
        size_t alloc_align = max_align > HASHSET_ALLOC_ALIGN ? max_align : HASHSET_ALLOC_ALIGN;
        ctx.alloc = pool_allocator_create(layout->stride, HASHSET_ALLOC_CAPACITY, alloc_align, GROW_GOLDEN);
    }
    
    if (!ctx.alloc) {
        hashset_create_ctx_cleanup(&ctx);
        return NULL;
    }
    
    HashSetImpl *impl = (HashSetImpl *)(ctx.set + 1);
    impl->item_size = (uint16_t)cfg->item_size;
    impl->item_offset = layout->item_offset;
    impl->stride = layout->stride;
    
    ctx.set->container.items = ctx.buckets;
    ctx.set->container.len = 0;
    ctx.set->container.capacity = capacity;
    ctx.set->container.ops = &HASHSET_CONTAINER_OPS;
    ctx.set->alloc = ctx.alloc;
    ctx.set->cmp = cfg->cmp;
    ctx.set->hash = cfg->hash;
    ctx.set->impl = impl;

    return ctx.set;
}

/* Create new set from existing set's type (same layout, different capacity) */
static HashSet *hashset_create_from_impl(const HashSet *src, size_t capacity, bool share_alloc) {
    if (!src) return NULL;

    HashSetCreateCtx ctx = {0};

    ctx.set = (HashSet *)malloc(sizeof(HashSet) + sizeof(HashSetImpl));
    if (!ctx.set) return NULL;

    if (capacity < HASHSET_MIN_CAPACITY) capacity = HASHSET_MIN_CAPACITY;
    capacity = lc_bit_ceil(capacity);

    ctx.buckets = (void **)calloc(capacity, sizeof(void *));
    if (!ctx.buckets) {
        hashset_create_ctx_cleanup(&ctx);
        return NULL;
    }

    const HashSetImpl *src_impl = src->impl;

    if (share_alloc) {
        ctx.alloc = allocator_ref(src->alloc);
    } else {
        ctx.alloc = pool_allocator_create(src_impl->stride, HASHSET_ALLOC_CAPACITY,
                                          allocator_alignment(src->alloc), GROW_GOLDEN);
    }

    if (!ctx.alloc) {
        hashset_create_ctx_cleanup(&ctx);
        return NULL;
    }
    
    HashSetImpl *impl = (HashSetImpl *)(ctx.set + 1);
    impl->item_size = src_impl->item_size;
    impl->item_offset = src_impl->item_offset;
    impl->stride = src_impl->stride;
  
    ctx.set->container.items = ctx.buckets;
    ctx.set->container.len = 0;
    ctx.set->container.capacity = capacity;
    ctx.set->container.ops = &HASHSET_CONTAINER_OPS;
    ctx.set->alloc = ctx.alloc;
    ctx.set->cmp = src->cmp;
    ctx.set->hash = src->hash;
    ctx.set->impl = impl;

    return ctx.set;
}

/* -------------------------------------------------------------------------
 * Builder API
 * ------------------------------------------------------------------------- */
HashSetBuilder hashset_builder(size_t item_size) {
    return (HashSetBuilder){
        .item_size = item_size,
        .item_align = 1,
        .capacity = HASHSET_MIN_CAPACITY,
        .alloc = NULL,
        .cmp = NULL,
        .hash = NULL,
    };
}

HashSetBuilder hashset_builder_str(void) {
    return (HashSetBuilder){
        .item_size = 0,
        .item_align = 1,
        .capacity = HASHSET_MIN_CAPACITY,
        .alloc = NULL,
        .cmp = NULL,
        .hash = NULL,
    };
}

HashSetBuilder hashset_builder_capacity(HashSetBuilder b, size_t capacity) {
    b.capacity = capacity;
    return b;
}

HashSetBuilder hashset_builder_hasher(HashSetBuilder b, lc_Hasher hasher) {
    b.hash = hasher;
    return b;
}

HashSetBuilder hashset_builder_comparator(HashSetBuilder b, lc_Comparator cmp) {
    b.cmp = cmp;
    return b;
}

HashSetBuilder hashset_builder_alignment(HashSetBuilder b, size_t item_align) {
    b.item_align = item_align;
    return b;
}

HashSetBuilder hashset_builder_allocator(HashSetBuilder b, Allocator *alloc) {
    b.alloc = alloc;
    return b;
}

HashSet *hashset_builder_build(HashSetBuilder b) {
    if (b.capacity == 0) return NULL;
    
    HashSetEntryLayout layout;
    if (!hashset_compute_layout(b.item_size, b.item_align, &layout)) return NULL;
    
    if (b.alloc) {
        if (allocator_stride(b.alloc) < layout.stride) return NULL;
    }
    
    return hashset_create_impl(&b, &layout);
}

/* -------------------------------------------------------------------------
 * Public creation functions
 * ------------------------------------------------------------------------- */
HashSet *hashset_create(size_t item_size) {
    return hashset_builder_build(hashset_builder(item_size));
}

HashSet *hashset_create_with_capacity(size_t item_size, size_t capacity) {
    return hashset_builder_build(hashset_builder_capacity(hashset_builder(item_size), capacity));
}

HashSet *hashset_create_with_hasher(size_t item_size, lc_Hasher hasher) {
    return hashset_builder_build(hashset_builder_hasher(hashset_builder(item_size), hasher));
}

HashSet *hashset_create_with_comparator(size_t item_size, lc_Comparator cmp) {
    return hashset_builder_build(hashset_builder_comparator(hashset_builder(item_size), cmp));
}

HashSet *hashset_create_with_allocator(size_t item_size, Allocator *alloc) {
    return hashset_builder_build(hashset_builder_allocator(hashset_builder(item_size), alloc));
}

HashSet *hashset_create_aligned(size_t item_size, size_t item_align) {
    return hashset_builder_build(hashset_builder_alignment(hashset_builder(item_size), item_align));
}

HashSet *hashset_str(void) {
    return hashset_builder_build(hashset_builder_str());
}

HashSet *hashset_str_with_capacity(size_t capacity) {
    return hashset_builder_build(hashset_builder_capacity(hashset_builder_str(), capacity));
}

HashSet *hashset_str_with_hasher(lc_Hasher hasher) {
    return hashset_builder_build(hashset_builder_hasher(hashset_builder_str(), hasher));
}

HashSet *hashset_str_with_comparator(lc_Comparator cmp) {
    return hashset_builder_build(hashset_builder_comparator(hashset_builder_str(), cmp));
}

HashSet *hashset_str_with_allocator(Allocator *alloc) {
    return hashset_builder_build(hashset_builder_allocator(hashset_builder_str(), alloc));
}

/* -------------------------------------------------------------------------
 * Destruction & configuration
 * ------------------------------------------------------------------------- */
void hashset_destroy(HashSet *set) {
    if (!set) return;
    
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    const size_t cap = set->container.capacity;
    
    for (size_t i = 0; i < cap; i++) {
        HashSetEntry *entry = buckets[i];
        while (entry) {
            HashSetEntry *next = entry->next;
            hashset_entry_free(set, entry);
            entry = next;
        }
    }
    
    free(buckets);
    allocator_destroy(set->alloc);
    free(set);
}

int hashset_set_hasher(HashSet *set, lc_Hasher hasher) {
    if (!set || !hasher) return LC_EINVAL;
    if (set->container.len > 0) return LC_EBUSY;
    set->hash = hasher;
    return LC_OK;
}

int hashset_set_comparator(HashSet *set, lc_Comparator cmp) {
    if (!set || !cmp) return LC_EINVAL;
    if (set->container.len > 0) return LC_EBUSY;
    set->cmp = cmp;
    return LC_OK;
}

int hashset_set_allocator(HashSet *set, Allocator *alloc) {
    if (!set || !alloc) return LC_EINVAL;
    if (set->container.len > 0) return LC_EBUSY;
    
    if (allocator_stride(alloc) < set->impl->stride) {
        return LC_ETYPE;
    }

    Allocator *old = set->alloc;
    set->alloc = allocator_ref(alloc);    
    allocator_destroy(old);
    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Core operations
 * ------------------------------------------------------------------------- */

/* Compute bucket index for an item */
static inline size_t hashset_bucket(const HashSet *set, const void *item) {
    const HashSetImpl *impl = set->impl;
    size_t h = lc_slot_hash(item, impl->item_size, set->hash);
    return h & (set->container.capacity - 1);
}

/* Insert into specific bucket (assumes bucket already computed) */
static int hashset_insert_impl(HashSet *set, const void *item, size_t bucket) {
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;

    const HashSetImpl *impl = set->impl;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Comparator cmp = set->cmp;
    
    /* Check for existing item */
    for (HashSetEntry *entry = buckets[bucket]; entry; entry = entry->next) {
        void *existing = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
        if (lc_slot_cmp(existing, item, item_sz, cmp) == 0) return LC_OK;
    }

    /* Create new entry */
    HashSetEntry *new_node = hashset_entry_create(set, item);
    if (!new_node) return LC_ENOMEM;

    new_node->next = buckets[bucket];
    buckets[bucket] = new_node;

    return LC_OK;
}

int hashset_insert(HashSet *set, const void *item) {
    if (!set || !item) return LC_EINVAL;
    int rc = hashset_insert_impl(set, item, hashset_bucket(set, item));
    if (rc != LC_OK) return rc;
    return hashset_adjust_capacity(set, false);
}

/* Remove from specific bucket */
static int hashset_remove_impl(HashSet *set, const void *item, size_t bucket) {
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    
    const HashSetImpl *impl = set->impl;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Comparator cmp = set->cmp;

    HashSetEntry **link = &buckets[bucket];
    HashSetEntry *entry;

    while ((entry = *link) != NULL) {
        void *existing = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
        if (lc_slot_cmp(existing, item, item_sz, cmp) == 0) {
            *link = entry->next;
            hashset_entry_free(set, entry);
            return LC_OK;
        }
        link = &entry->next;
    }

    return LC_ENOTFOUND;
}

int hashset_remove(HashSet *set, const void *item) {
    if (!set || !item) return LC_EINVAL;
    int rc = hashset_remove_impl(set, item, hashset_bucket(set, item));
    if (rc != LC_OK) return rc;
    return hashset_adjust_capacity(set, true);
}

/* Check if item exists in specific bucket */
static bool hashset_contains_impl(const HashSet *set, const void *item, size_t bucket) {
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    
    const HashSetImpl *impl = set->impl;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Comparator cmp = set->cmp;

    for (HashSetEntry *entry = buckets[bucket]; entry; entry = entry->next) {
        void *existing = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
        if (lc_slot_cmp(existing, item, item_sz, cmp) == 0)
            return true;
    }

    return false;
}

bool hashset_contains(const HashSet *set, const void *item) {
    if (!set || !item) return false;
    return hashset_contains_impl(set, item, hashset_bucket(set, item));
}

/* Populate destination set from source set */
static int hashset_populate(HashSet *dst, const HashSet *src, bool check_duplicates) {
    HashSetEntry **dst_buckets = (HashSetEntry **)dst->container.items;
    HashSetEntry **src_buckets = (HashSetEntry **)src->container.items;
    
    const HashSetImpl *impl = src->impl;
    const size_t src_cap = src->container.capacity;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const size_t dst_mask = dst->container.capacity - 1;
    const lc_Hasher hash_fn = dst->hash;

    for (size_t i = 0; i < src_cap; i++) {
        for (HashSetEntry *entry = src_buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
            size_t b = lc_slot_hash(item, item_sz, hash_fn) & dst_mask;
            
            int rc;
            if (check_duplicates) {
                rc = hashset_insert_impl(dst, item, b);
            } else {
                HashSetEntry *new_node = hashset_entry_create(dst, item);
                if (!new_node) return LC_ENOMEM;
                new_node->next = dst_buckets[b];
                dst_buckets[b] = new_node;
                rc = LC_OK;
            }
            
            if (rc != LC_OK) return rc;
        }
    }
    return LC_OK;
}

/* Merge src into dst (internal, swaps buckets on success) */
static int hashset_merge_impl(HashSet *dst, const HashSet *src) {
    size_t total_len = dst->container.len + src->container.len;
    HashSet *tmp = hashset_create_from_impl(dst, total_len, true);
    if (!tmp) return LC_ENOMEM;

    if (hashset_populate(tmp, dst, false) != LC_OK || 
        hashset_populate(tmp, src, true) != LC_OK) {
        hashset_destroy(tmp);
        return LC_ENOMEM;
    }
    
    /* Swap bucket arrays */
    void *tmp_items = tmp->container.items;
    size_t tmp_cap = tmp->container.capacity;
    size_t tmp_len = tmp->container.len;

    tmp->container.items = dst->container.items;
    tmp->container.capacity = dst->container.capacity;
    tmp->container.len = dst->container.len;

    dst->container.items = tmp_items;
    dst->container.capacity = tmp_cap;
    dst->container.len = tmp_len;

    hashset_destroy(tmp);
    return LC_OK;
}

int hashset_merge(HashSet *dst, const HashSet *src) {
    if (!dst || !src) return LC_EINVAL;
    if (dst == src) return LC_OK;
    
    if (dst->impl->item_size != src->impl->item_size) 
        return LC_ETYPE;
    
    return hashset_merge_impl(dst, src);
}

int hashset_reserve(HashSet *set, size_t expected) {
    if (!set) return LC_EINVAL;
    if (expected == 0) return LC_OK;

    if (expected > (SIZE_MAX / 4) * 3) return LC_EOVERFLOW;
    size_t min_cap = (expected * 4 + 2) / 3;

    if (min_cap <= set->container.capacity) return LC_OK;

    size_t target = lc_bit_ceil(min_cap);
    if (target < min_cap) return LC_EOVERFLOW;

    return hashset_rehash(set, target);
}

int hashset_clear(HashSet *set) {
    if (!set) return LC_EINVAL;
    
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    const size_t cap = set->container.capacity;
    
    for (size_t i = 0; i < cap; i++) {
        HashSetEntry *entry = buckets[i];
        while (entry) {
            HashSetEntry *next = entry->next;
            hashset_entry_free(set, entry);
            entry = next;
        }
        buckets[i] = NULL;
    }
    
    return LC_OK;
}

int hashset_shrink_to_fit(HashSet *set) {
    if (!set) return LC_EINVAL;
    return hashset_adjust_capacity(set, true);
}

/* -------------------------------------------------------------------------
 * Queries
 * ------------------------------------------------------------------------- */
size_t hashset_len(const HashSet *set) { return set ? set->container.len : 0; }
size_t hashset_size(const HashSet *set) { return hashset_len(set); }
size_t hashset_capacity(const HashSet *set) { return set ? set->container.capacity : 0; }
bool hashset_is_empty(const HashSet *set) { return !set || set->container.len == 0; }

size_t hashset_hash(const HashSet *set) {
    if (!set || set->container.len == 0) return 0;

    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    HashSetImpl *impl = (HashSetImpl *)set->impl;

    const size_t cap = set->container.capacity;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Hasher hash_fn = set->hash;

    size_t h = 0;
    for (size_t i = 0; i < cap; i++) {
        for (HashSetEntry *entry = buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
            size_t x = lc_slot_hash(item, item_sz, hash_fn);
            h ^= lc_hash_mix(x);
        }
    }

    return lc_hash_mix(h ^ set->container.len);
}

bool hashset_subset(const HashSet *A, const HashSet *B) {
    if (!A || !B) return false;
    if (A->container.len > B->container.len) return false;

    const HashSetImpl *ai = A->impl;
    const HashSetImpl *bi = B->impl;
    
    if (ai->item_size != bi->item_size) return false;

    HashSetEntry **a_buckets = (HashSetEntry **)A->container.items;

    const size_t a_cap = A->container.capacity;
    const size_t b_mask = B->container.capacity - 1;
    const size_t item_off = ai->item_offset;
    const size_t item_sz = ai->item_size;
    const lc_Hasher b_hash = B->hash;

    for (size_t i = 0; i < a_cap; i++) {
        for (HashSetEntry *entry = a_buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
            size_t b = lc_slot_hash(item, item_sz, b_hash) & b_mask;
            if (!hashset_contains_impl(B, item, b)) 
                return false;
        }
    }
    
    return true;
}

bool hashset_equals(const HashSet *A, const HashSet *B) {
    if (!A || !B) return false;
    if (A == B) return true;
    if (A->container.len != B->container.len) return false;
    
    if (A->impl->item_size != B->impl->item_size) return false;

    return hashset_subset(A, B);
}

Allocator *hashset_allocator(const HashSet *set) { 
    return set ? set->alloc : NULL; 
}

/* -------------------------------------------------------------------------
 * Clone
 * ------------------------------------------------------------------------- */
HashSet *hashset_clone(const HashSet *set) {
    if (!set) return NULL;

    HashSet *clone = hashset_create_from_impl(set, set->container.capacity, false);
    if (!clone) return NULL;
    
    if (hashset_populate(clone, set, false) != LC_OK) {
        hashset_destroy(clone);
        return NULL;
    }
    
    return clone;
}

HashSet *hashset_instance(const HashSet *set) {
    if (!set) return NULL;
    return hashset_create_from_impl(set, HASHSET_MIN_CAPACITY, false);
}

/* -------------------------------------------------------------------------
 * To Array
 * ------------------------------------------------------------------------- */
static Array *hashset_collect_fixed(const HashSet *set, size_t item_size) {
    const size_t set_len = set->container.len;
    if (set_len == 0) return NULL;

    if (set_len > SIZE_MAX / item_size) return NULL;

    const size_t set_cap = set->container.capacity;
    const size_t target_off = set->impl->item_offset;
   
    if (set_len > (SIZE_MAX - sizeof(Array)) / item_size) return NULL;
    
    Array *arr = (Array *)malloc(sizeof(Array) + set_len * item_size);
    if (!arr) return NULL;

    arr->items = (uint8_t *)(arr + 1);
    arr->stride = item_size;
    arr->len = set_len;
   
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    
    uint8_t *dst = (uint8_t *)arr->items;
    size_t pos = 0;
    for (size_t i = 0; i < set_cap; i++) {
        for (HashSetEntry *entry = buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_at(entry->data, target_off);
            memcpy(dst + (pos++ * item_size), item, item_size);
        }
    }

    return arr;
}

static Array *hashset_collect_strings(const HashSet *set) {
    const size_t set_len = set->container.len;
    if (set_len == 0) return NULL;

    if (set_len > SIZE_MAX / sizeof(char *)) return NULL;
    
    const size_t set_cap = set->container.capacity;
    const size_t target_off = set->impl->item_offset;

    size_t total_chars = 0;
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
    for (size_t b = 0; b < set_cap; b++) {
        for (HashSetEntry *entry = buckets[b]; entry; entry = entry->next) {
            const char *str = *(const char **)lc_slot_at(entry->data, target_off);
            total_chars += strlen(str) + 1;
        }
    }

    size_t ptrs_size = set_len * sizeof(char *);
    if (total_chars > SIZE_MAX - ptrs_size - sizeof(Array)) return NULL;
    
    Array *arr = (Array *)malloc(sizeof(Array) + ptrs_size + total_chars);
    if (!arr) return NULL;

    arr->items = (uint8_t *)(arr + 1);
    arr->stride = 0;
    arr->len = set_len;

    char **table = (char **)arr->items;
    char *pool = (char *)(arr->items + ptrs_size);
    
    size_t pos = 0;
    for (size_t i = 0; i < set_cap; i++) {
        for (HashSetEntry *entry = buckets[i]; entry; entry = entry->next) {
            const char *str = *(const char **)lc_slot_at(entry->data, target_off);
            size_t slen = strlen(str) + 1;
            table[pos++] = pool;
            memcpy(pool, str, slen);
            pool += slen;
        }
    }

    return arr;
}

Array *hashset_to_array(const HashSet *set) {
    if (!set) return NULL;

    if (set->impl->item_size) {
        return hashset_collect_fixed(set, set->impl->item_size);
    } 
    return hashset_collect_strings(set);
}

/* -------------------------------------------------------------------------
 * Set operations
 * ------------------------------------------------------------------------- */
HashSet *hashset_union(const HashSet *A, const HashSet *B) {
    if (!A || !B) return NULL;

    if (A == B) return hashset_clone(A);

    if (A->impl->item_size != B->impl->item_size) return NULL;

    size_t expected_size = hashset_len(A) + hashset_len(B);

    HashSet *new_set = hashset_create_from_impl(A, expected_size, false);
    if (!new_set) return NULL;
    
    if (hashset_populate(new_set, A, false) != LC_OK || 
        hashset_populate(new_set, B, true) != LC_OK) {
        hashset_destroy(new_set);
        return NULL;
    }

    hashset_adjust_capacity(new_set, true);
    return new_set;
}

HashSet *hashset_intersection(const HashSet *A, const HashSet *B) {
    if (!A || !B) return NULL;
    if (A == B) return hashset_clone(A);

    const HashSetImpl *ai = A->impl;
    const HashSetImpl *bi = B->impl;

    if (ai->item_size != bi->item_size) return NULL;
   
    HashSet *new_set = hashset_create_from_impl(A, HASHSET_MIN_CAPACITY, false);
    if (!new_set) return NULL;
    
    const size_t item_off = ai->item_offset;
    const size_t item_sz = ai->item_size;
    const size_t a_cap = A->container.capacity;
    const size_t b_mask = B->container.capacity - 1;
    const size_t n_mask = new_set->container.capacity - 1;
    const lc_Hasher b_hash = B->hash;
    const lc_Hasher n_hash = new_set->hash;
    
    HashSetEntry **a_buckets = (HashSetEntry **)A->container.items;
    
    for (size_t i = 0; i < a_cap; i++) {
        for (HashSetEntry *entry = a_buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);

            if (hashset_contains_impl(B, item, lc_slot_hash(item, item_sz, b_hash) & b_mask)) {
                int rc = hashset_insert_impl(new_set, item, lc_slot_hash(item, item_sz, n_hash) & n_mask);
                if (rc != LC_OK) {
                    hashset_destroy(new_set);
                    return NULL;
                }
            }
        }
    }

    hashset_adjust_capacity(new_set, true);
    return new_set;
}

HashSet *hashset_difference(const HashSet *A, const HashSet *B) {
    if (!A || !B) return NULL;

    const HashSetImpl *ai = A->impl;
    const HashSetImpl *bi = B->impl;

    if (ai->item_size != bi->item_size) return NULL;

    HashSet *new_set = hashset_create_from_impl(A, HASHSET_MIN_CAPACITY, false);
    if (!new_set) return NULL;
    
    const size_t item_off = ai->item_offset;
    const size_t item_sz = ai->item_size;
    const size_t a_cap = A->container.capacity;
    const size_t b_mask = B->container.capacity - 1;
    const size_t n_mask = new_set->container.capacity - 1;
    const lc_Hasher b_hash = B->hash;
    const lc_Hasher n_hash = new_set->hash;
    
    HashSetEntry **a_buckets = (HashSetEntry **)A->container.items;
    
    for (size_t i = 0; i < a_cap; i++) {
        for (HashSetEntry *entry = a_buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);

            if (!hashset_contains_impl(B, item, lc_slot_hash(item, item_sz, b_hash) & b_mask)) {
                int rc = hashset_insert_impl(new_set, item, lc_slot_hash(item, item_sz, n_hash) & n_mask);
                if (rc != LC_OK) {
                    hashset_destroy(new_set);
                    return NULL;
                }
            }
        }
    }

    hashset_adjust_capacity(new_set, true);
    return new_set;
}

/* -------------------------------------------------------------------------
 * Iterator
 * ------------------------------------------------------------------------- */
static const void *hashset_next(Iterator *it) {
    if (it->direction == ITER_REVERSE) return NULL;
    
    HashSet *set = (HashSet *)it->container;
    HashSetEntry **buckets = (HashSetEntry **)set->container.items;
   
    HashSetEntry *entry = (HashSetEntry *)it->state;

    if (entry && entry->next) {
        entry = entry->next;
    } else {
        entry = NULL;
        while (it->pos < set->container.capacity) {
            entry = buckets[it->pos++];
            if (entry) break;
        }
    }

    it->state = entry;
    return entry ? lc_slot_get(lc_slot_at(entry->data, set->impl->item_offset), set->impl->item_size) : NULL;
}

Iterator hashset_iter(const HashSet *set) {
    return Iter((const Container *)set);
}

/* -------------------------------------------------------------------------
 * Container adapter functions
 * ------------------------------------------------------------------------- */
static Container *hashset_instance_adpt(const Container *c) {
    return (Container *)hashset_instance((const HashSet *)c); 
}

static Container *hashset_clone_adpt(const Container *c) {
    return (Container *)hashset_clone((const HashSet *)c); 
}

static Array *hashset_to_array_adpt(const Container *c) {
    return hashset_to_array((const HashSet *)c); 
}

static size_t hashset_hash_adpt(const Container *c) {
    return hashset_hash((const HashSet *)c); 
}

static bool hashset_insert_adpt(Container *set, const void *item) {
    return hashset_insert((HashSet *)set, item) == LC_OK;
}

static void hashset_clear_adpt(Container *set) {
    hashset_clear((HashSet *)set);
}

static void hashset_destroy_adpt(Container *c) {
    hashset_destroy((HashSet *)c); 
}

#endif /* HASHSET_IMPLEMENTATION_ONCE */
#endif /* HASHSET_IMPLEMENTATION */
