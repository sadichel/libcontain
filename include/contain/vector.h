/** @file vector.h
 * @brief Dynamic Array with Automatic Resizing
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * A type‑generic dynamic array (vector) that stores any POD type or C strings.
 * Provides O(1) amortised push, O(1) random access, automatic capacity
 * management, and a rich set of operations.
 *
 * @section key_features Key Features
 *   - @ref type_agnostic — stores any data via item_size
 *   - @ref automatic_resizing — expands by ×2, minimum 16 elements
 *   - @ref base_alignment — first element can be over-aligned (SIMD, atomics)
 *   - @ref string_support — first‑class strings with vector_str()
 *   - @ref container_interface — implements the generic Container vtable
 *
 * @section alignment_note Alignment Note
 *   Only the base pointer (first element) is aligned to base_align.
 *   Subsequent elements are at natural stride (item_size). For SIMD,
 *   this is sufficient as you typically load from the start of the array.
 *
 * @section usage Usage Example
 * @code
 *   #include "vector.h"
 *
 *   Vector *vec = vector_create(sizeof(int));
 *   int v = 42;
 *   vector_push(vec, &v);
 *   // ...
 *   vector_destroy(vec);
 * @endcode
 */

#ifndef VECTOR_PDR_H
#define VECTOR_PDR_H

#include "container.h"

/**
 * @defgroup vector_constants Vector Constants
 * @{
 */

/** @brief Minimum capacity for a new vector (16 elements) */
#define VECTOR_MIN_CAPACITY 16

/** @brief Return value for "not found" in find operations */
#define VEC_NPOS            SIZE_MAX

/** @} */

/**
 * @defgroup vector_types Vector Types
 * @brief Core types for the vector container
 * @{
 */

/**
 * @struct VectorImpl
 * @brief Opaque implementation structure for vector internals
 */
typedef struct VectorImpl VectorImpl;

/**
 * @struct Vector
 * @brief Dynamic array container
 *
 * @var Vector::container
 * Common container header (items, len, capacity, ops)
 *
 * @var Vector::cmp
 * Comparator function for elements (NULL = memcmp for fixed-size, strcmp for strings)
 *
 * @var Vector::impl
 * Opaque pointer to implementation details (item_size, stride, alignment, cached_hash)
 */
typedef struct Vector {
    Container container;            /**< Common container header */
    lc_Comparator cmp;                 /**< Element comparator (NULL = default) */
    const VectorImpl *impl;         /**< Opaque implementation data */
} Vector;

/**
 * @struct VectorBuilder
 * @brief Fluent configuration builder for vector creation
 *
 * @var VectorBuilder::item_size
 * Size of each element in bytes (0 = string mode, stores char*)
 *
 * @var VectorBuilder::base_align
 * Alignment for the first element (0 = default, must be power of two)
 *
 * @var VectorBuilder::capacity
 * Initial capacity (rounded up to VECTOR_MIN_CAPACITY)
 *
 * @var VectorBuilder::cmp
 * Item comparator (NULL = memcmp for fixed-size, strcmp for strings)
 */
typedef struct {
    size_t item_size;      /**< 0 = string mode (stores char*) */
    size_t base_align;     /**< Alignment for first element (0 = default) */
    size_t capacity;       /**< Initial capacity (rounded up to MIN) */
    lc_Comparator cmp;        /**< Item comparator (NULL = memcmp for POD and strcmp for string) */
} VectorBuilder;

/** @} */

/**
 * @defgroup vector_creation Creation
 * @brief Functions for creating and destroying vectors
 * @{
 */

/**
 * @brief Create a new vector with default capacity
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @return Newly allocated vector, or NULL on allocation failure
 *
 * @warning The caller owns the returned vector and must destroy it with vector_destroy()
 */
Vector *vector_create(size_t item_size);

/**
 * @brief Create a new vector with specified initial capacity
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param capacity  Initial capacity (will be rounded up to VECTOR_MIN_CAPACITY)
 * @return Newly allocated vector, or NULL on allocation failure
 */
Vector *vector_create_with_capacity(size_t item_size, size_t capacity);

/**
 * @brief Create a new vector with a custom comparator
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param cmp       Element comparator function
 * @return Newly allocated vector, or NULL on allocation failure
 */
Vector *vector_create_with_comparator(size_t item_size, lc_Comparator cmp);

/**
 * @brief Create a new vector with aligned first element
 *
 * Useful for SIMD, atomic operations, or cache-line optimisation.
 *
 * @param item_size  Size of each element in bytes (0 for string mode)
 * @param base_align Alignment for the first element (must be power of two)
 * @return Newly allocated vector, or NULL on allocation failure
 *
 * @note Only the first element is aligned to base_align. Subsequent elements
 *       are at natural stride (item_size).
 */
Vector *vector_create_aligned(size_t item_size, size_t base_align);

/**
 * @brief Create a new string vector
 *
 * Stores C strings as char* with automatic strdup on insertion.
 *
 * @return Newly allocated string vector, or NULL on allocation failure
 */
Vector *vector_str(void);

/**
 * @brief Create a new string vector with specified initial capacity
 *
 * @param capacity Initial capacity
 * @return Newly allocated string vector, or NULL on allocation failure
 */
Vector *vector_str_with_capacity(size_t capacity);

/**
 * @brief Create a new string vector with a custom comparator
 *
 * @param cmp String comparator function (receives char**)
 * @return Newly allocated string vector, or NULL on allocation failure
 */
Vector *vector_str_with_comparator(lc_Comparator cmp);

/**
 * @brief Build a vector using a fluent builder
 *
 * @param b Initialised VectorBuilder
 * @return Newly allocated vector, or NULL on configuration or allocation failure
 *
 * @par Example
 * @code
 *   Vector *vec = vector_builder_build(
 *       vector_builder_alignment(
 *           vector_builder_capacity(
 *               vector_builder(sizeof(int)), 64),
 *           64));
 * @endcode
 */
Vector *vector_builder_build(VectorBuilder b);

/**
 * @brief Destroy a vector and free all resources
 *
 * Frees all elements (including strings in string mode) and the vector itself.
 *
 * @param vec Vector to destroy (can be NULL)
 */
void vector_destroy(Vector *vec);

/**
 * @brief Set the comparator for a vector
 *
 * @param vec Vector to modify
 * @param cmp New comparator function
 * @return LC_OK on success, LC_EINVAL if vec or cmp is NULL,
 *         LC_EBUSY if vector is not empty
 *
 * @note Only allowed on empty vectors.
 */
int vector_set_comparator(Vector *vec, lc_Comparator cmp);

/** @} */

/**
 * @defgroup vector_core Core Operations
 * @brief Insert, remove, and modify elements
 * @{
 */

/**
 * @brief Append an element to the end of the vector
 *
 * @param vec  Vector to modify
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL if vec or item is NULL,
 *         LC_ENOMEM on allocation failure
 *
 * @par Example
 * @code
 *   int value = 42;
 *   vector_push(vec, &value);
 * @endcode
 */
int vector_push(Vector *vec, const void *item);

/**
 * @brief Insert an element at the specified position
 *
 * Shifts elements from pos onward to the right.
 *
 * @param vec Vector to modify
 * @param pos Position to insert at (0 = front, len = back)
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_EBOUNDS if pos > len, LC_ENOMEM on allocation failure
 */
int vector_insert(Vector *vec, size_t pos, const void *item);

/**
 * @brief Insert a range of elements from another vector
 *
 * @param dst  Destination vector
 * @param pos  Position to insert at
 * @param src  Source vector
 * @param from Start index in src (inclusive)
 * @param to   End index in src (exclusive)
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_EBOUNDS on invalid indices, LC_ENOMEM on allocation failure
 */
int vector_insert_range(Vector *dst, size_t pos, const Vector *src, size_t from, size_t to);

/**
 * @brief Append all elements from another vector
 *
 * @param dst Destination vector
 * @param src Source vector
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_ENOMEM on allocation failure
 */
int vector_append(Vector *dst, const Vector *src);

/**
 * @brief Append a range of elements from another vector
 *
 * @param dst  Destination vector
 * @param src  Source vector
 * @param from Start index in src (inclusive)
 * @param to   End index in src (exclusive)
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_EBOUNDS on invalid indices, LC_ENOMEM on allocation failure
 */
int vector_append_range(Vector *dst, const Vector *src, size_t from, size_t to);

/**
 * @brief Set an element at the specified position
 *
 * @param vec  Vector to modify
 * @param pos  Position to set
 * @param item Pointer to new element value
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_EBOUNDS if pos >= len
 */
int vector_set(Vector *vec, size_t pos, const void *item);

/**
 * @brief Remove and return the last element
 *
 * @param vec Vector to modify
 * @return LC_OK on success, LC_EINVAL if vec is NULL,
 *         LC_EBOUNDS if vector is empty
 */
int vector_pop(Vector *vec);

/**
 * @brief Remove an element at the specified position
 *
 * Shifts subsequent elements left.
 *
 * @param vec Vector to modify
 * @param pos Position to remove
 * @return LC_OK on success, LC_EINVAL if vec is NULL,
 *         LC_EBOUNDS if pos >= len
 */
int vector_remove(Vector *vec, size_t pos);

/**
 * @brief Remove a range of elements
 *
 * @param vec  Vector to modify
 * @param from Start index (inclusive)
 * @param to   End index (exclusive)
 * @return LC_OK on success, LC_EINVAL if vec is NULL,
 *         LC_EBOUNDS if from >= to or to > len
 */
int vector_remove_range(Vector *vec, size_t from, size_t to);

/**
 * @brief Splice operation: remove and insert in one pass
 *
 * Removes remove_count elements at pos, then inserts elements from
 * src[src_from:src_to] at the same position.
 *
 * @param dst         Destination vector
 * @param pos         Position to splice at
 * @param remove_count Number of elements to remove
 * @param src         Source vector (can be NULL if insert_count == 0)
 * @param src_from    Start index in src (inclusive)
 * @param src_to      End index in src (exclusive)
 * @return LC_OK on success, various error codes on failure
 */
int vector_splice(Vector *dst, size_t pos, size_t remove_count, const Vector *src, size_t src_from, size_t src_to);

/**
 * @brief Remove duplicate adjacent elements (requires sorted vector)
 *
 * @param sorted_vec Vector that must be sorted (behaviour undefined if unsorted)
 * @return LC_OK on success, LC_EINVAL if vec is NULL
 */
int vector_unique(Vector *sorted_vec);

/**
 * @brief Trim excess capacity (alias for vector_shrink_to_fit)
 *
 * @param vec Vector to trim
 * @return LC_OK on success, LC_EINVAL if vec is NULL
 */
int vector_trim(Vector *vec);

/**
 * @brief Shrink capacity to fit current length
 *
 * Reduces memory usage by reallocating to exactly fit the current elements.
 * Failure is non-fatal; original buffer is kept.
 *
 * @param vec Vector to shrink
 * @return LC_OK on success (or if shrink fails but vector remains valid),
 *         LC_EINVAL if vec is NULL
 */
int vector_shrink_to_fit(Vector *vec);

/**
 * @brief Reserve capacity for expected number of elements
 *
 * Pre-allocates memory to avoid repeated reallocations.
 *
 * @param vec               Vector to reserve
 * @param expected_capacity Desired capacity
 * @return LC_OK on success, LC_EINVAL if vec is NULL,
 *         LC_EOVERFLOW on overflow, LC_ENOMEM on allocation failure
 */
int vector_reserve(Vector *vec, size_t expected_capacity);

/**
 * @brief Resize the vector to a new length
 *
 * For fixed-size vectors:
 *   - Growing adds zero-initialized elements
 *   - Shrinking removes elements from the end
 *
 * For string vectors:
 *   - Only shrinking is supported (returns LC_ENOTSUP for growth)
 *
 * @param vec     Vector to resize
 * @param new_len Desired length
 * @return LC_OK on success, LC_ENOTSUP if growing a string vector,
 *         LC_ENOMEM on allocation failure
 */
int vector_resize(Vector *vec, size_t new_len);

/**
 * @brief Remove all elements from the vector
 *
 * Capacity is retained. For string vectors, all strings are freed.
 *
 * @param vec Vector to clear
 * @return LC_OK on success, LC_EINVAL if vec is NULL
 */
int vector_clear(Vector *vec);

/** @} */

/**
 * @defgroup vector_inplace In-place Operations
 * @brief Transform the vector in place
 * @{
 */

/**
 * @brief Reverse the order of elements in place
 *
 * @param vec Vector to reverse
 * @return LC_OK on success, LC_EINVAL if vec is NULL,
 *         LC_ENOMEM on allocation failure (only for large stride)
 */
int vector_reverse_inplace(Vector *vec);

/**
 * @brief Sort the vector in place
 *
 * Uses qsort() internally.
 *
 * @param vec Vector to sort
 * @param cmp Comparator (if NULL, uses vector's stored comparator)
 * @return LC_OK on success, LC_EINVAL if vec is NULL or no comparator available
 */
int vector_sort(Vector *vec, lc_Comparator cmp);

/** @} */

/**
 * @defgroup vector_queries Queries
 * @brief Read element data and query properties
 * @{
 */

/**
 * @brief Get a pointer to an element (read-only)
 *
 * @param vec Vector to query
 * @param pos Position to access
 * @return Pointer to the element, or NULL if vec is NULL or pos out of bounds
 *
 * @warning The returned pointer points directly into the vector's storage.
 *          It may be invalidated by any operation that modifies the vector.
 */
const void *vector_at(const Vector *vec, size_t pos);

/**
 * @brief Get a pointer to the first element (read-only)
 *
 * @param vec Vector to query
 * @return Pointer to the first element, or NULL if vector is empty or NULL
 */
const void *vector_front(const Vector *vec);

/**
 * @brief Get a pointer to the last element (read-only)
 *
 * @param vec Vector to query
 * @return Pointer to the last element, or NULL if vector is empty or NULL
 */
const void *vector_back(const Vector *vec);

/**
 * @brief Get a mutable pointer to an element
 *
 * @param vec Vector to query
 * @param pos Position to access
 * @return Mutable pointer to the element, or NULL if vec is NULL or pos out of bounds
 *
 * @warning Modifying the element via this pointer does not invalidate the
 *          cached hash. Call vector_hash() again if needed.
 */
void *vector_at_mut(const Vector *vec, size_t pos);

/**
 * @brief Get a mutable pointer to the first element
 *
 * @param vec Vector to query
 * @return Mutable pointer to the first element, or NULL if vector is empty or NULL
 */
void *vector_front_mut(const Vector *vec);

/**
 * @brief Get a mutable pointer to the last element
 *
 * @param vec Vector to query
 * @return Mutable pointer to the last element, or NULL if vector is empty or NULL
 */
void *vector_back_mut(const Vector *vec);

/**
 * @brief Find the first occurrence of an element
 *
 * @param vec  Vector to search
 * @param item Element to find
 * @return Index of the element, or VEC_NPOS if not found
 */
size_t vector_find(const Vector *vec, const void *item);

/**
 * @brief Find the last occurrence of an element (reverse find)
 *
 * @param vec  Vector to search
 * @param item Element to find
 * @return Index of the element, or VEC_NPOS if not found
 */
size_t vector_rfind(const Vector *vec, const void *item);

/**
 * @brief Check if an element exists in the vector
 *
 * @param vec  Vector to search
 * @param item Element to find
 * @return true if element exists, false otherwise
 */
bool vector_contains(const Vector *vec, const void *item);

/**
 * @brief Check if the vector is empty
 *
 * @param vec Vector to query
 * @return true if vec is NULL or contains no elements, false otherwise
 */
bool vector_is_empty(const Vector *vec);

/**
 * @brief Check if two vectors are equal
 *
 * Compares length, element sizes, and each element using the comparator.
 *
 * @param A First vector
 * @param B Second vector
 * @return true if vectors are equal, false otherwise
 */
bool vector_equals(const Vector *A, const Vector *B);

/**
 * @brief Get the number of elements in the vector
 *
 * @param vec Vector to query
 * @return Number of elements, or 0 if vec is NULL
 */
size_t vector_len(const Vector *vec);

/**
 * @brief Get the current capacity of the vector
 *
 * @param vec Vector to query
 * @return Current capacity, or 0 if vec is NULL
 */
size_t vector_capacity(const Vector *vec);

/**
 * @brief Compute a hash of the vector's contents
 *
 * The hash is cached and invalidated on any mutation.
 *
 * @param vec Vector to hash
 * @return Hash value, or 0 if vec is NULL or empty
 */
size_t vector_hash(const Vector *vec);

/** @} */

/**
 * @defgroup vector_copy Copy & View Operations
 * @brief Create new vectors from existing ones
 * @{
 */

/**
 * @brief Create a new vector with elements in reverse order
 *
 * @param vec Source vector
 * @return New vector with reversed elements, or NULL on allocation failure
 */
Vector *vector_reverse(const Vector *vec);

/**
 * @brief Create a new empty vector of the same type
 *
 * @param vec Source vector (determines item size, alignment, comparator)
 * @return New empty vector, or NULL on allocation failure
 */
Vector *vector_instance(const Vector *vec);

/**
 * @brief Create a deep copy of a vector
 *
 * For string vectors, all strings are strdup'd.
 *
 * @param vec Source vector
 * @return Deep copy of the vector, or NULL on allocation failure
 */
Vector *vector_clone(const Vector *vec);

/**
 * @brief Extract a slice of the vector as a new vector
 *
 * @param vec  Source vector
 * @param from Start index (inclusive)
 * @param to   End index (exclusive)
 * @return New vector containing the slice, or NULL on invalid range or allocation failure
 */
Vector *vector_slice(const Vector *vec, size_t from, size_t to);

/** @} */

/**
 * @defgroup_vector_conversion Conversion & Iteration
 * @brief Export to Array and create iterators
 * @{
 */

/**
 * @brief Export the vector to a flat Array snapshot
 *
 * The Array header and data are in a single allocation — free with free(arr).
 *
 * @param vec Vector to export
 * @return Array snapshot, or NULL if vec is NULL or empty or allocation fails
 */
Array *vector_to_array(const Vector *vec);

/**
 * @brief Create a forward iterator over the vector
 *
 * @param vec Vector to iterate
 * @return Forward iterator (stack-allocated)
 */
Iterator vector_iter(const Vector *vec);

/**
 * @brief Create a reverse iterator over the vector
 *
 * @param vec Vector to iterate
 * @return Reverse iterator (stack-allocated)
 */
Iterator vector_iter_reversed(const Vector *vec);

/** @} */

#endif /* VECTOR_PDR_H */

/**
 * @defgroup vector_implementation Implementation
 * @brief STB-style implementation section
 *
 * To include the implementation, define VECTOR_IMPLEMENTATION in ONE .c file
 * before including this header:
 *
 * @code
 *   #define VECTOR_IMPLEMENTATION
 *   #include "vector.h"
 * @endcode
 *
 * @note The implementation is intentionally not documented in detail as it is
 *       meant to be included only once per project.
 */

#ifdef VECTOR_IMPLEMENTATION
#ifndef VECTOR_IMPLEMENTATION_ONCE
#define VECTOR_IMPLEMENTATION_ONCE

/* ============================================================================
 * Vector — Implementation
 * ============================================================================
 *
 * Important Notes:
 *   • All functions assume valid parameters unless checked
 *   • String mode (item_size == 0) stores char* pointers, each strdup'd
 *   • Alignment support allows SIMD, atomic, and cache-line optimizations
 *   • The container vtable enables generic iteration and collection
 *   • Cached hash is invalidated on any mutation
 * ============================================================================ */

/* -------------------------------------------------------------------------
 * Container vtable declarations
 * ------------------------------------------------------------------------- */
static const void *vector_next(Iterator *iter);
static Container *vector_instance_adpt(const Container *vec);
static Container *vector_clone_adpt(const Container *vec);
static Array *vector_to_array_adpt(const Container *vec);
static size_t vector_hash_adpt(const Container *vec);
static bool vector_insert_adpt(Container *vec, const void *item);
static void vector_clear_adpt(Container *vec);
static void vector_destroy_adpt(Container *vec);

/* VTABLE for generic container interface */
static const ContainerVTable VECTOR_CONTAINER_OPS = {
    .next     = vector_next,
    .instance = vector_instance_adpt,
    .clone    = vector_clone_adpt,
    .as_array = vector_to_array_adpt,
    .hash     = vector_hash_adpt,
    .insert   = vector_insert_adpt,
    .clear    = vector_clear_adpt,
    .destroy  = vector_destroy_adpt
};

/* -------------------------------------------------------------------------
 * Internal structures
 * ------------------------------------------------------------------------- */

struct VectorImpl {
    uint16_t item_size;      /* 0 = string mode */
    uint16_t base_align;     /* alignment for first element */
    uint32_t stride;         /* = item_size (no padding) */
};

/* Layout calculation result */
typedef struct {
    uint16_t base_align;
    uint32_t stride;
} VectorEntryLayout;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Calculate new capacity with exponential growth (×2) */
static size_t vector_grow(size_t stride, size_t current_cap, size_t min_cap) {
    const size_t max_cap = SIZE_MAX / stride;
    if (min_cap > max_cap) return 0;

    size_t new_cap = current_cap;
    if (new_cap < max_cap / 2) {
        new_cap *= 2;
    } else {
        new_cap = max_cap;
    }

    if (new_cap < min_cap) new_cap = min_cap;
    return (new_cap > current_cap) ? new_cap : 0;
}

/* Expand vector capacity, using aligned allocation if needed */
static int vector_expand_capacity(Vector *vec, size_t stride, size_t min_capacity) {
    size_t new_cap = vector_grow(stride, vec->container.capacity, min_capacity);
    if (!new_cap) return LC_ENOMEM;

    const size_t base_align = vec->impl->base_align;
    const size_t total = new_cap * stride;
    const size_t nat_align = lc_alloc_max_align();

    void *new_base;
    if (base_align > nat_align) {
        new_base = lc_alloc_realloc_aligned(vec->container.items, total, base_align);
    } else {
        new_base = realloc(vec->container.items, total);
    }

    if (!new_base) return LC_ENOMEM;

    vec->container.items = new_base;
    vec->container.capacity = new_cap;
    return LC_OK;
}

/* Insert a slot range, shifting tail if needed. Returns pointer to first slot */
static void *vector_insert_slot(Vector *vec, size_t pos, size_t count) {
    if (count == 0 || pos > vec->container.len) return NULL;

    VectorImpl *impl = (VectorImpl *)vec->impl;
    size_t stride = impl->stride;
    size_t old_len = vec->container.len;
    size_t needed = old_len + count;

    if (needed > vec->container.capacity) {
        if (vector_expand_capacity(vec, stride, needed) != LC_OK) return NULL;
    }

    uint8_t *base = (uint8_t *)vec->container.items;

    /* Shift tail elements to make room */
    if (pos < old_len) {
        memmove(base + (pos + count) * stride,
                base + pos * stride,
                (old_len - pos) * stride);
    }

    vec->container.len += count;
    return base + pos * stride;
}

/* Free a slot range, shifting tail forward. Returns error code */
static int vector_free_slot(Vector *vec, size_t from, size_t to) {
    if (from >= to || to > vec->container.len) return LC_EBOUNDS;

    VectorImpl *impl = (VectorImpl *)vec->impl;
    size_t stride = impl->stride;
    size_t gap = to - from;

    uint8_t *base = (uint8_t *)vec->container.items;
    size_t tail = vec->container.len - to;

    /* For strings, free each string before removal */
    if (vec->impl->item_size == 0) {
        for (size_t i = from; i < to; i++) {
            free(*(char **)(base + (i * stride)));
        }
    }

    /* Shift tail elements forward */
    if (tail > 0) {
        memmove(base + (from * stride), base + (to * stride), tail * stride);
    }

    vec->container.len -= gap;
    return LC_OK;
}

/* Get pointer to slot at position (no bounds check for speed) */
static void *vector_slot_at(Vector *vec, size_t pos) {
    if (pos >= vec->container.len) return NULL;
    return (uint8_t *)vec->container.items + (pos * vec->impl->stride);
}

/* Compute item layout: stride and alignment */
static bool vector_compute_layout(size_t isize, size_t ialign, VectorEntryLayout *out) {
    /* Alignment must be a power of two */
    if (ialign == 0 || (ialign & (ialign - 1)) != 0) return false;

    /* String mode: items are char* pointers */
    if (isize == 0) isize = sizeof(void *);
    if (ialign > UINT16_MAX) return false;

    size_t stride = isize;
    if (stride > UINT32_MAX) return false;

    out->base_align = (uint16_t)ialign;
    out->stride = (uint32_t)stride;
    return true;
}

/* -------------------------------------------------------------------------
 * Creation helpers
 * ------------------------------------------------------------------------- */

/* Internal vector creation with precomputed layout */
static Vector *vector_create_impl(VectorBuilder *cfg, VectorEntryLayout *layout) {
    size_t capacity = cfg->capacity;
    size_t stride = layout->stride;
    size_t base_align = layout->base_align;
    const size_t nat_align = lc_alloc_max_align();

    if (capacity > SIZE_MAX / stride) return NULL;
    size_t total = capacity * stride;

    /* Allocate vector header + implementation block */
    Vector *vec = (Vector *)malloc(sizeof(Vector) + sizeof(VectorImpl));
    if (!vec) return NULL;

    /* Allocate aligned item buffer */
    void *buffer;
    if (base_align > nat_align) {
        buffer = lc_alloc_malloc_aligned(total, base_align);
    } else {
        buffer = malloc(total);
    }

    if (!buffer) {
        free(vec);
        return NULL;
    }

    /* Initialize implementation block */
    VectorImpl *impl = (VectorImpl *)(vec + 1);
    impl->item_size = (uint16_t)cfg->item_size;
    impl->base_align = (uint16_t)base_align;
    impl->stride = (uint32_t)stride;

    /* Initialize container header */
    vec->container.items = buffer;
    vec->container.len = 0;
    vec->container.capacity = capacity;
    vec->container.ops = &VECTOR_CONTAINER_OPS;
    vec->cmp = cfg->cmp;
    vec->impl = impl;

    return vec;
}

/* Create a new vector from an existing one's type (same layout, different capacity) */
static Vector *vector_create_from_impl(const Vector *src, size_t capacity) {
    if (!src || capacity == 0) return NULL;

    const size_t stride = src->impl->stride;
    const uint16_t base_align = src->impl->base_align;
    const uint16_t item_size = src->impl->item_size;
    const size_t nat_align = lc_alloc_max_align();

    if (capacity > SIZE_MAX / stride) return NULL;
    size_t total = capacity * stride;

    Vector *vec = (Vector *)malloc(sizeof(Vector) + sizeof(VectorImpl));
    if (!vec) return NULL;

    void *buffer;
    if (base_align > nat_align) {
        buffer = lc_alloc_malloc_aligned(total, base_align);
    } else {
        buffer = malloc(total);
    }

    if (!buffer) {
        free(vec);
        return NULL;
    }

    VectorImpl *impl = (VectorImpl *)(vec + 1);
    impl->item_size = item_size;
    impl->base_align = base_align;
    impl->stride = (uint32_t)stride;

    vec->container.items = buffer;
    vec->container.len = 0;
    vec->container.capacity = capacity;
    vec->container.ops = &VECTOR_CONTAINER_OPS;
    vec->cmp = src->cmp;
    vec->impl = impl;

    return vec;
}

/* -------------------------------------------------------------------------
 * Builder API
 * ------------------------------------------------------------------------- */
VectorBuilder vector_builder(size_t item_size) {
    return (VectorBuilder){
        .item_size = item_size,
        .base_align = 1,
        .capacity = VECTOR_MIN_CAPACITY,
        .cmp = NULL
    };
}

VectorBuilder vector_builder_str(void) {
    return (VectorBuilder){
        .item_size = 0,
        .base_align = 1,
        .capacity = VECTOR_MIN_CAPACITY,
        .cmp = NULL
    };
}

VectorBuilder vector_builder_capacity(VectorBuilder b, size_t capacity) {
    b.capacity = capacity;
    return b;
}

VectorBuilder vector_builder_comparator(VectorBuilder b, lc_Comparator cmp) {
    b.cmp = cmp;
    return b;
}

VectorBuilder vector_builder_alignment(VectorBuilder b, size_t base_align) {
    b.base_align = base_align;
    return b;
}

Vector *vector_builder_build(VectorBuilder b) {
    if (b.capacity == 0) return NULL;

    VectorEntryLayout layout;
    if (!vector_compute_layout(b.item_size, b.base_align, &layout)) return NULL;

    return vector_create_impl(&b, &layout);
}

/* -------------------------------------------------------------------------
 * Public creation functions
 * ------------------------------------------------------------------------- */
Vector *vector_create(size_t item_size) {
    return vector_builder_build(vector_builder(item_size));
}

Vector *vector_create_with_capacity(size_t item_size, size_t capacity) {
    return vector_builder_build(vector_builder_capacity(vector_builder(item_size), capacity));
}

Vector *vector_create_with_comparator(size_t item_size, lc_Comparator cmp) {
    return vector_builder_build(vector_builder_comparator(vector_builder(item_size), cmp));
}

Vector *vector_create_aligned(size_t item_size, size_t base_align) {
    return vector_builder_build(vector_builder_alignment(vector_builder(item_size), base_align));
}

Vector *vector_str(void) {
    return vector_builder_build(vector_builder_str());
}

Vector *vector_str_with_capacity(size_t capacity) {
    return vector_builder_build(vector_builder_capacity(vector_builder_str(), capacity));
}

Vector *vector_str_with_comparator(lc_Comparator cmp) {
    return vector_builder_build(vector_builder_comparator(vector_builder_str(), cmp));
}

/* -------------------------------------------------------------------------
 * Destruction & configuration
 * ------------------------------------------------------------------------- */

/* Destroy vector and free all resources */
void vector_destroy(Vector *vec) {
    if (!vec) return;

    /* In string mode, free each string */
    if (vec->impl->item_size == 0 && vec->container.len > 0) {
        size_t len = vec->container.len;
        size_t stride = vec->impl->stride;
        uint8_t *base = (uint8_t *)vec->container.items;

        for (size_t i = 0; i < len; i++) {
            free(*(char **)(base + (i * stride)));
        }
    }

    /* Free item buffer with appropriate alignment */
    if (vec->impl->base_align > lc_alloc_max_align())
        lc_alloc_free(vec->container.items);
    else
        free(vec->container.items);

    free(vec);
}

/* Set comparator (only allowed on empty vector) */
int vector_set_comparator(Vector *vec, lc_Comparator cmp) {
    if (!vec || !cmp) return LC_EINVAL;
    if (vec->container.len > 0) return LC_EBUSY;
    vec->cmp = cmp;
    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Core operations (implementations)
 * ------------------------------------------------------------------------- */

/* Append strings from src to dst at position pos */
static int vector_append_strings(Vector *dst, size_t pos, const Vector *src, size_t from, size_t to) {
    size_t count = to - from;
    char **temp = (char **)malloc(count * sizeof(char *));
    if (!temp) return LC_ENOMEM;

    /* Copy and strdup each string */
    for (size_t i = 0; i < count; i++) {
        char **src_slot = (char **)vector_slot_at((Vector *)src, from + i);
        temp[i] = strdup(*src_slot);
        if (!temp[i]) {
            while (i--) free(temp[i]);
            free(temp);
            return LC_ENOMEM;
        }
    }

    /* Insert into destination */
    char **slots = (char **)vector_insert_slot(dst, pos, count);
    if (!slots) {
        for (size_t i = 0; i < count; i++) free(temp[i]);
        free(temp);
        return LC_ENOMEM;
    }

    memcpy(slots, temp, count * sizeof(char *));
    free(temp);
    return LC_OK;
}

/* Append fixed-size items from src to dst at position pos */
static int vector_append_fixed(Vector *dst, size_t pos, const Vector *src, size_t from, size_t to) {
    size_t count = to - from;
    size_t stride = dst->impl->stride;

    uint8_t *data = (uint8_t *)src->container.items + (from * stride);
    uint8_t *temp_buf = NULL;

    /* If src == dst, we need a temporary copy to avoid overwriting */
    if (src == dst) {
        temp_buf = (uint8_t *)malloc(count * stride);
        if (!temp_buf) return LC_ENOMEM;
        memcpy(temp_buf, data, count * stride);
        data = temp_buf;
    }

    uint8_t *dst_slots = (uint8_t *)vector_insert_slot(dst, pos, count);
    if (!dst_slots) {
        free(temp_buf);
        return LC_ENOMEM;
    }

    memcpy(dst_slots, data, count * stride);
    free(temp_buf);
    return LC_OK;
}

/* Generic append implementation */
static int vector_append_impl(Vector *dst, size_t pos, const Vector *src, size_t from, size_t to) {
    if (dst->impl->item_size != src->impl->item_size) return LC_ETYPE;
    if (pos > dst->container.len) return LC_EBOUNDS;
    if (to > src->container.len) return LC_EBOUNDS;
    if (from >= to) return LC_OK;

    if (dst->impl->item_size == 0) {
        return vector_append_strings(dst, pos, src, from, to);
    }
    return vector_append_fixed(dst, pos, src, from, to);
}

/* Insert single item at position */
static int vector_insert_impl(Vector *vec, size_t pos, const void *item) {
    if (pos > vec->container.len) return LC_EBOUNDS;

    if (vec->impl->item_size == 0) {
        /* String mode: strdup the string */
        char *str = strdup((const char *)item);
        if (!str) return LC_ENOMEM;

        void *slot = vector_insert_slot(vec, pos, 1);
        if (!slot) {
            free(str);
            return LC_ENOMEM;
        }
        *(char **)slot = str;
    } else {
        /* Fixed-size mode: memcpy */
        void *slot = vector_insert_slot(vec, pos, 1);
        if (!slot) return LC_ENOMEM;
        memcpy(slot, item, vec->impl->item_size);
    }

    return LC_OK;
}

int vector_push(Vector *vec, const void *item) {
    if (!vec || !item) return LC_EINVAL;
    
    VectorImpl *impl = (VectorImpl *)vec->impl;
    size_t len = vec->container.len;
    size_t cap = vec->container.capacity;
    size_t stride = impl->stride;
    size_t isize = impl->item_size;
    
    if (len >= cap) {
        if (vector_expand_capacity(vec, stride, len + 1) != LC_OK)
            return LC_ENOMEM;
        cap = vec->container.capacity;
    }
    
    void *slot = (uint8_t *)vec->container.items + (len * stride);
    
    if (isize == 0) {
        char *str = strdup((const char *)item);
        if (!str) return LC_ENOMEM;
        *(char **)slot = str;
    } else {
        memcpy(slot, item, isize);
    }
    
    vec->container.len = len + 1;
    
    return LC_OK;
}

int vector_insert(Vector *vec, size_t pos, const void *item) {
    if (!vec || !item) return LC_EINVAL;
    return vector_insert_impl(vec, pos, item);
}

int vector_insert_range(Vector *dst, size_t pos, const Vector *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    return vector_append_impl(dst, pos, src, from, to);
}

int vector_append(Vector *dst, const Vector *src) {
    if (!dst || !src) return LC_EINVAL;
    return vector_append_impl(dst, dst->container.len, src, 0, src->container.len);
}

int vector_append_range(Vector *dst, const Vector *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    return vector_append_impl(dst, dst->container.len, src, from, to);
}

int vector_set(Vector *vec, size_t pos, const void *item) {
    if (!vec || !item) return LC_EINVAL;
    if (pos >= vec->container.len) return LC_EBOUNDS;

    void *slot = vector_slot_at(vec, pos);
    int rc = lc_slot_set(slot, item, vec->impl->item_size);

    return rc;
}

int vector_remove(Vector *vec, size_t pos) {
    if (!vec) return LC_EINVAL;
    if (pos >= vec->container.len) return LC_EBOUNDS;
    return vector_free_slot(vec, pos, pos + 1);
}

int vector_remove_range(Vector *vec, size_t from, size_t to) {
    if (!vec) return LC_EINVAL;
    return vector_free_slot(vec, from, to);
}

int vector_pop(Vector *vec) {
    if (!vec) return LC_EINVAL;
    if (vec->container.len == 0) return LC_EBOUNDS;
    return vector_free_slot(vec, vec->container.len - 1, vec->container.len);
}

int vector_shrink_to_fit(Vector *vec) {
    if (!vec) return LC_EINVAL;

    const size_t len = vec->container.len;
    const size_t cap = vec->container.capacity;

    if (cap <= VECTOR_MIN_CAPACITY) return LC_OK;

    const size_t new_cap = (len > VECTOR_MIN_CAPACITY) ? len : VECTOR_MIN_CAPACITY;
    if (new_cap == cap) return LC_OK;

    const size_t stride = vec->impl->stride;
    const size_t base_align = vec->impl->base_align;
    const size_t nat_align = lc_alloc_max_align();
    const size_t total = new_cap * stride;

    void *new_base;
    if (base_align > nat_align) {
        new_base = lc_alloc_realloc_aligned(vec->container.items, total, base_align);
    } else {
        new_base = realloc(vec->container.items, total);
    }

    /* Shrink failure is non-fatal; keep old buffer */
    if (!new_base) return LC_OK;

    vec->container.items = new_base;
    vec->container.capacity = new_cap;
    return LC_OK;
}

int vector_trim(Vector *vec) {
    return vector_shrink_to_fit(vec);
}

int vector_reserve(Vector *vec, size_t new_cap) {
    if (!vec) return LC_EINVAL;

    size_t old_cap = vec->container.capacity;
    if (new_cap <= old_cap) return LC_OK;

    const size_t stride = vec->impl->stride;
    const size_t base_align = vec->impl->base_align;
    const size_t nat_align = lc_alloc_max_align();

    if (new_cap > SIZE_MAX / stride) return LC_EOVERFLOW;
    const size_t total = new_cap * stride;

    void *new_base;
    if (base_align > nat_align) {
        new_base = lc_alloc_realloc_aligned(vec->container.items, total, base_align);
    } else {
        new_base = realloc(vec->container.items, total);
    }

    if (!new_base) return LC_ENOMEM;

    vec->container.items = new_base;
    vec->container.capacity = new_cap;
    return LC_OK;
}

/**
 * Resize vector to new length.
 * 
 * For fixed-size vectors: 
 *   - Growing adds zero-initialized elements
 *   - Shrinking removes elements from the end
 * 
 * For string vectors:
 *   - Only shrinking is supported
 *   - Use vector_push() or vector_insert() to add elements
 * 
 * Returns LC_ENOTSUP if attempting to grow a string vector.
 */
 
int vector_resize(Vector *vec, size_t new_len) {
    if (!vec) return LC_EINVAL;

    size_t old_len = vec->container.len;
    if (new_len == old_len) return LC_OK;

    if (new_len < old_len) {
        return vector_free_slot(vec, new_len, old_len);
    }

    /* Prevent upward resize for string vectors */
    if (vec->impl->item_size == 0) {
        return LC_ENOTSUP;  
    }

    size_t count = new_len - old_len;
    void *slots = vector_insert_slot(vec, old_len, count);
    if (!slots) return LC_ENOMEM;

    memset(slots, 0, count * vec->impl->stride);
    return LC_OK;
}

int vector_clear(Vector *vec) {
    if (!vec) return LC_EINVAL;
    if (vec->container.len == 0) return LC_OK;

    /* Free all strings in string mode */
    if (vec->impl->item_size == 0) {
        size_t len = vec->container.len;
        size_t stride = vec->impl->stride;
        uint8_t *base = (uint8_t *)vec->container.items;
        for (size_t i = 0; i < len; i++) {
            free(*(char **)(base + (i * stride)));
        }
    }

    vec->container.len = 0;

    return LC_OK;
}

/* -------------------------------------------------------------------------
 * In-place operations
 * ------------------------------------------------------------------------- */

int vector_reverse_inplace(Vector *vec) {
    if (!vec || vec->container.len < 2) return LC_OK;

    const size_t len = vec->container.len;
    const size_t stride = vec->impl->stride;
    uint8_t *base = (uint8_t *)vec->container.items;

    /* Use stack buffer for small items to avoid malloc */
    uint8_t stack_temp[128];
    uint8_t *temp = (stride <= sizeof(stack_temp)) ? stack_temp : (uint8_t *)malloc(stride);
    if (!temp) return LC_ENOMEM;

    for (size_t i = 0; i < len / 2; i++) {
        uint8_t *left = base + (i * stride);
        uint8_t *right = base + ((len - 1 - i) * stride);

        memcpy(temp, left, stride);
        memcpy(left, right, stride);
        memcpy(right, temp, stride);
    }

    if (temp != stack_temp) free(temp);
    return LC_OK;
}

int vector_splice(Vector *dst, size_t pos, size_t remove_count, const Vector *src, size_t src_from, size_t src_to) {
    if (!dst) return LC_EINVAL;
    if (pos > dst->container.len) return LC_EBOUNDS;
    if (remove_count > dst->container.len - pos) return LC_EBOUNDS;

    const size_t insert_count = (src && src_from < src_to) ? (src_to - src_from) : 0;

    if (insert_count > 0) {
        if (!src) return LC_EINVAL;
        if (dst->impl->item_size != src->impl->item_size) return LC_ETYPE;
        if (src_to > src->container.len) return LC_EBOUNDS;
    }

    const size_t stride = dst->impl->stride;
    const size_t isize = dst->impl->item_size;

    /* Reserve final capacity first */
    const size_t final_len = dst->container.len - remove_count + insert_count;
    if (final_len > dst->container.capacity) {
        int rc = vector_reserve(dst, final_len);
        if (rc != LC_OK) return rc;
    }

    /* Snapshot insert data (in case src == dst) */
    uint8_t stack_buf[256];
    uint8_t *temp = NULL;
    size_t temp_len = insert_count * stride;

    if (insert_count > 0) {
        const uint8_t *src_base = (const uint8_t *)src->container.items + (src_from * stride);

        temp = (temp_len <= sizeof(stack_buf)) ? stack_buf : (uint8_t *)malloc(temp_len);
        if (!temp) return LC_ENOMEM;

        if (isize == 0) {
            /* String mode: duplicate each string */
            for (size_t i = 0; i < insert_count; i++) {
                const char *s = *(const char **)(src_base + (i * stride));
                char *dup = s ? strdup(s) : NULL;
                if (s && !dup) {
                    for (size_t j = 0; j < i; j++)
                        free(*(char **)(temp + (j * stride)));
                    if (temp != stack_buf) free(temp);
                    return LC_ENOMEM;
                }
                *(char **)(temp + (i * stride)) = dup;
            }
        } else {
            memcpy(temp, src_base, temp_len);
        }
    }

    /* Remove old items */
    if (remove_count > 0)
        vector_free_slot(dst, pos, pos + remove_count);

    /* Insert new items */
    if (insert_count > 0) {
        void *slots = vector_insert_slot(dst, pos, insert_count);
        memcpy(slots, temp, temp_len);
    }

    if (temp && temp != stack_buf) free(temp);
    return LC_OK;
}

int vector_unique(Vector *vec) {
    if (!vec || vec->container.len < 2) return LC_OK;

    const size_t len = vec->container.len;
    const size_t stride = vec->impl->stride;
    const size_t isize = vec->impl->item_size;
    uint8_t *base = (uint8_t *)vec->container.items;
    
    if (vec->cmp) {
        const lc_Comparator cmp = vec->cmp;
        size_t write = 0;
        
        for (size_t read = 1; read < len; read++) {
            void *last = base + (write * stride);
            void *curr = base + (read * stride);
            
            if (cmp(last, curr) != 0) {
                write++;
                if (read != write) {
                    if (isize == 0) {
                        char **dst = (char **)(base + (write * stride));
                        char **src = (char **)curr;
                        *dst = *src;
                    } else {
                        memcpy(base + (write * stride), curr, stride);
                    }
                }
            } else if (isize == 0) {
                free(*(char **)curr);
                *(char **)curr = NULL;
            }
        }
        
        vec->container.len = write + 1;
    } else if (isize == 0) {
        /* String mode, default strcmp */
        size_t write = 0;
        
        for (size_t read = 1; read < len; read++) {
            char **last = (char **)(base + (write * stride));
            char **curr = (char **)(base + (read * stride));
            
            if (strcmp(*last, *curr) != 0) {
                write++;
                if (read != write) {
                    char **dst = (char **)(base + (write * stride));
                    *dst = *curr;
                }
            } else {
                free(*curr);
                *curr = NULL;
            }
        }
        
        vec->container.len = write + 1;
    } else {
        /* Fixed-size mode, default memcmp */
        size_t write = 0;
        
        for (size_t read = 1; read < len; read++) {
            void *last = base + (write * stride);
            void *curr = base + (read * stride);
            
            if (memcmp(last, curr, isize) != 0) {
                write++;
                if (read != write) {
                    memcpy(base + (write * stride), curr, stride);
                }
            }
        }
        
        vec->container.len = write + 1;
    }

    return LC_OK;
}

int vector_sort(Vector *vec, lc_Comparator cmp) {
    if (!vec) return LC_EINVAL;
    if (vec->container.len < 2) return LC_OK;

    lc_Comparator target_cmp = cmp ? cmp : vec->cmp;
    if (!target_cmp) return LC_EINVAL;

    qsort(vec->container.items, vec->container.len, vec->impl->stride, target_cmp);

    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Queries & utilities
 * ------------------------------------------------------------------------- */

const void *vector_at(const Vector *vec, size_t pos) {
    if (!vec) return NULL;
    void *slot = vector_slot_at((Vector *)vec, pos);
    return slot ? lc_slot_get(slot, vec->impl->item_size) : NULL;
}

const void *vector_front(const Vector *vec) {
    if (!vec) return NULL;
    void *slot = vector_slot_at((Vector *)vec, 0);
    return slot ? lc_slot_get(slot, vec->impl->item_size) : NULL;
}

const void *vector_back(const Vector *vec) {
    if (!vec || vec->container.len == 0) return NULL;
    void *slot = vector_slot_at((Vector *)vec, vec->container.len - 1);
    return slot ? lc_slot_get(slot, vec->impl->item_size) : NULL;
}

void *vector_at_mut(const Vector *vec, size_t pos) {
    if (!vec) return NULL;
    return vector_slot_at((Vector *)vec, pos);
}

void *vector_front_mut(const Vector *vec) {
    if (!vec) return NULL;
    return vector_slot_at((Vector *)vec, 0);
}

void *vector_back_mut(const Vector *vec) {
    if (!vec || vec->container.len == 0) return NULL;
    return vector_slot_at((Vector *)vec, vec->container.len - 1);
}

size_t vector_find(const Vector *vec, const void *item) {
    if (!vec || !item || vec->container.len == 0) return VEC_NPOS;

    const VectorImpl *impl = vec->impl;
    const size_t len = vec->container.len;
    const size_t stride = impl->stride;
    const size_t isize = impl->item_size;
    const lc_Comparator cmp = vec->cmp;
    const uint8_t *base = (const uint8_t *)vec->container.items;

    /* Fast path: fixed-size, no custom comparator */
    if (isize > 0 && !cmp) {
        for (size_t i = 0; i < len; i++) {
            if (memcmp(base + (i * stride), item, isize) == 0) {
                return i;
            }
        }
        return VEC_NPOS;
    }

    /* Slow path: strings or custom comparator */
    for (size_t i = 0; i < len; i++) {
        void *current_slot = (void *)(base + (i * stride));
        if (lc_slot_cmp(lc_slot_get(current_slot, isize), item, isize, cmp) == 0) {
            return i;
        }
    }

    return VEC_NPOS;
}

size_t vector_rfind(const Vector *vec, const void *item) {
    if (!vec || !item || vec->container.len == 0) return VEC_NPOS;

    const VectorImpl *impl = vec->impl;
    const size_t len = vec->container.len;
    const size_t stride = impl->stride;
    const size_t isize = impl->item_size;
    const lc_Comparator cmp = vec->cmp;
    const uint8_t *base = (const uint8_t *)vec->container.items;

    /* Fast path: fixed-size, no custom comparator */
    if (isize > 0 && !cmp) {
        for (size_t i = len; i > 0; i--) {
            if (memcmp(base + ((i - 1) * stride), item, isize) == 0) {
                return i - 1;
            }
        }
        return VEC_NPOS;
    }

    /* Slow path: strings or custom comparator */
    for (size_t i = len; i > 0; i--) {
        size_t idx = i - 1;
        void *current_slot = (void *)(base + (idx * stride));
        if (lc_slot_cmp(lc_slot_get(current_slot, isize), item, isize, cmp) == 0) {
            return idx;
        }
    }

    return VEC_NPOS;
}

bool vector_contains(const Vector *vec, const void *item) {
    return vector_find(vec, item) != VEC_NPOS;
}

bool vector_is_empty(const Vector *vec) {
    return !vec || vec->container.len == 0;
}

size_t vector_len(const Vector *vec) {
    return vec ? vec->container.len : 0;
}

size_t vector_size(const Vector *vec) {
    return vector_len(vec);
}

size_t vector_capacity(const Vector *vec) {
    return vec ? vec->container.capacity : 0;
}

size_t vector_hash(const Vector *vec) {
    if (!vec || vec->container.len == 0) return 0;

    VectorImpl *impl = (VectorImpl *)vec->impl;
    const size_t len = vec->container.len;
    const size_t stride = impl->stride;
    const size_t isize = impl->item_size;
    const uint8_t *base = (const uint8_t *)vec->container.items;

    size_t h = len;

    for (size_t i = 0; i < len; i++) {
        void *current_slot = (void *)(base + i * stride);
        size_t item_hash = lc_slot_hash(lc_slot_get(current_slot, isize), isize, NULL);
        /* Golden ratio mix to distribute entropy */
        h ^= item_hash + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return lc_hash_mix(h);
}

bool vector_equals(const Vector *A, const Vector *B) {
    if (A == B) return true;
    if (!A || !B) return false;

    const VectorImpl *impl_a = A->impl;
    const VectorImpl *impl_b = B->impl;

    /* Structural parity check */
    if (A->container.len != B->container.len ||
        impl_a->item_size != impl_b->item_size) {
        return false;
    }

    const size_t n = A->container.len;
    if (n == 0) return true;

    const size_t stride = impl_a->stride;
    const size_t isize = impl_a->item_size;
    const lc_Comparator cmp = A->cmp;

    const uint8_t *base_a = (const uint8_t *)A->container.items;
    const uint8_t *base_b = (const uint8_t *)B->container.items;

    /* Fast path: fixed-size, no custom comparator, contiguous memory */
    if (isize > 0 && !cmp) {
        if (impl_a->stride == impl_a->item_size) {
            return memcmp(base_a, base_b, n * stride) == 0;
        }
        for (size_t i = 0; i < n; i++) {
            if (memcmp(base_a + i * stride, base_b + i * stride, isize) != 0)
                return false;
        }
        return true;
    }

    /* Slow path: strings or custom comparator */
    for (size_t i = 0; i < n; i++) {
        void *a_slot = (void *)(base_a + i * stride);
        void *b_slot = (void *)(base_b + i * stride);

        if (lc_slot_cmp(lc_slot_get(a_slot, isize), lc_slot_get(b_slot, isize), isize, cmp) != 0) {
            return false;
        }
    }

    return true;
}

/* -------------------------------------------------------------------------
 * Copy & view operations
 * ------------------------------------------------------------------------- */

Vector *vector_reverse(const Vector *vec) {
    if (!vec) return NULL;

    Vector *rev = vector_create_from_impl(vec, vec->container.len);
    if (!rev) return NULL;

    const size_t n = vec->container.len;
    const size_t stride = vec->impl->stride;
    const size_t isize = vec->impl->item_size;

    uint8_t *dst_base = (uint8_t *)rev->container.items;
    const uint8_t *src_base = (const uint8_t *)vec->container.items;

    /* Copy items in reverse order */
    for (size_t i = 0; i < n; i++) {
        const void *src_item = src_base + ((n - 1 - i) * stride);
        void *dst_slot = dst_base + (i * stride);

        if (lc_slot_copy(dst_slot, src_item, isize) != LC_OK) {
            vector_destroy(rev);
            return NULL;
        }

        rev->container.len++;
    }

    return rev;
}

Vector *vector_clone(const Vector *src) {
    if (!src) return NULL;

    Vector *clone = vector_create_from_impl(src, src->container.capacity);
    if (!clone) return NULL;

    const size_t len = src->container.len;
    const size_t stride = src->impl->stride;
    const size_t isize = src->impl->item_size;
    uint8_t *dst_base = (uint8_t *)clone->container.items;
    const uint8_t *src_base = (const uint8_t *)src->container.items;

    if (isize == 0) {
        /* String mode: deep copy each string */
        for (size_t i = 0; i < len; i++) {
            void *src_slot = (uint8_t *)src_base + (i * stride);
            void *dst_slot = dst_base + (i * stride);

            if (lc_slot_copy(dst_slot, src_slot, 0) != LC_OK) {
                vector_destroy(clone);
                return NULL;
            }
            clone->container.len++;
        }
    } else if (len > 0) {
        /* Fixed-size mode: memcpy entire block */
        memcpy(dst_base, src_base, len * stride);
        clone->container.len = len;
    }

    return clone;
}

Vector *vector_slice(const Vector *vec, size_t start, size_t end) {
    if (!vec || start >= end || start >= vec->container.len) return NULL;

    const size_t count = end - start;
    Vector *slice = vector_create_from_impl(vec, count);
    if (!slice) return NULL;

    const size_t stride = vec->impl->stride;
    const size_t isize = vec->impl->item_size;
    uint8_t *dst_base = (uint8_t *)slice->container.items;
    const uint8_t *src_base = (const uint8_t *)vec->container.items + (start * stride);

    if (isize == 0) {
        /* String mode: deep copy each string */
        for (size_t i = 0; i < count; i++) {
            void *src_slot = (uint8_t *)src_base + (i * stride);
            void *dst_slot = dst_base + (i * stride);

            if (lc_slot_copy(dst_slot, src_slot, 0) != LC_OK) {
                vector_destroy(slice);
                return NULL;
            }
            slice->container.len++;
        }
    } else {
        /* Fixed-size mode: memcpy entire block */
        memcpy(dst_base, src_base, count * stride);
        slice->container.len = count;
    }

    return slice;
}

Vector *vector_instance(const Vector *vec) {
    if (!vec) return NULL;
    return vector_create_from_impl(vec, VECTOR_MIN_CAPACITY);
}

/* Collect fixed-size items into array */
static Array *vector_collect_fixed(const Vector *vec) {
    const size_t vec_len = vec->container.len;
    if (vec_len == 0) return NULL;

    const size_t stride = vec->impl->stride;
    const size_t base_align = vec->impl->base_align;
    const size_t total_items = vec_len * stride;
    
    /* Check for item overflow */
    if (vec_len > 0 && total_items / vec_len != stride) return NULL;
    
    size_t align = base_align > 16 ? base_align : 16;
    size_t header_size = sizeof(Array);
    
    /* Check for offset overflow */
    if (header_size > SIZE_MAX - align) return NULL;
    size_t items_offset = (header_size + align - 1) & ~(align - 1);
    
    /* Check for total overflow */
    if (items_offset > SIZE_MAX - total_items) return NULL;
    size_t total = items_offset + total_items;
    
    /* Check for allocation size overflow */
    if (total > SIZE_MAX) return NULL;
    
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return NULL;
    
    uint8_t *items = buf + items_offset;
    
    Array *arr = (Array *)buf;
    arr->items = items;
    arr->stride = stride;
    arr->len = vec_len;
    
    memcpy(items, vec->container.items, total_items);
    
    return arr;
}

/* Collect strings into array with pointer table + string pool */
static Array *vector_collect_strings(const Vector *vec) {
    const size_t vec_len = vec->container.len;
    if (vec_len == 0) return NULL;

    /* Check pointer table overflow */
    if (vec_len > SIZE_MAX / sizeof(char *)) return NULL;
    
    const size_t stride = vec->impl->stride;
    const uint8_t *base = (const uint8_t *)vec->container.items;
    
    /* First pass: measure total string pool size */
    size_t total_chars = 0;
    size_t valid_count = 0;
    
    for (size_t i = 0; i < vec_len; i++) {
        const char *str = *(const char **)(base + (i * stride));
        if (!str) continue;  /* Skip NULL (shouldn't happen, but safe) */
        
        size_t len = strlen(str) + 1;
        if (total_chars > SIZE_MAX - len) return NULL;
        total_chars += len;
        valid_count++;
    }
    
    /* If all strings were NULL (unlikely), return empty array */
    if (valid_count == 0) return NULL;
    
    /* Calculate sizes */
    size_t ptrs_size = vec_len * sizeof(char *);
    size_t data_size = ptrs_size + total_chars;
    
    if (sizeof(Array) > SIZE_MAX - data_size) return NULL;
    size_t total = sizeof(Array) + data_size;
    
    /* Single allocation */
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return NULL;
    
    /* Setup Array header */
    Array *arr = (Array *)buf;
    arr->items = buf + sizeof(Array);
    arr->stride = 0;
    arr->len = 0;  /* Start at 0, increment as we add */
    
    /* Pointer table */
    char **table = (char **)arr->items;
    char *pool = (char *)(arr->items + ptrs_size);
    char *pool_start = pool;
    
    /* Second pass: fill array */
    for (size_t i = 0; i < vec_len; i++) {
        const char *str = *(const char **)(base + (i * stride));
        
        if (!str) {
            table[i] = NULL;
            continue;
        }
        
        size_t len = strlen(str) + 1;
        
        /* Safety check */
        if (pool > pool_start + total_chars) {
            free(buf);
            return NULL;
        }
        
        table[i] = pool;
        memcpy(pool, str, len);
        pool += len;
        arr->len++;  /* Increment only for valid entries */
    }
    
    return arr;
}

Array *vector_to_array(const Vector *vec) {
    if (!vec) return NULL;

    if (vec->impl->item_size == 0) {
        return vector_collect_strings(vec);
    }
    return vector_collect_fixed(vec);
}

/* -------------------------------------------------------------------------
 * Iterator implementation
 * ------------------------------------------------------------------------- */
 
static const void *vector_next(Iterator *it) {
    Vector *vec = (Vector *)it->container;

    if (it->direction == ITER_FORWARD) {
        if (it->pos >= vec->container.len) return NULL;
        void *slot = (uint8_t *)vec->container.items + (vec->impl->stride * it->pos);
        it->pos++;
        return lc_slot_get(slot, vec->impl->item_size);
    } else {
        if (it->pos == 0) return NULL;
        it->pos--;
        void *slot = (uint8_t *)vec->container.items + (vec->impl->stride * it->pos);
        return lc_slot_get(slot, vec->impl->item_size);
    }
}

Iterator vector_iter(const Vector *vec) {
    return Iter((const Container *)vec);
}

Iterator vector_iter_reversed(const Vector *vec) {
    return IterReverse((const Container *)vec);
}

/* -------------------------------------------------------------------------
 * Container adapter functions
 * ------------------------------------------------------------------------- */

static Container *vector_instance_adpt(const Container *vec) {
    return (Container *)vector_instance((const Vector *)vec);
}

static Container *vector_clone_adpt(const Container *vec) {
    return (Container *)vector_clone((const Vector *)vec);
}

static Array *vector_to_array_adpt(const Container *vec) {
    return vector_to_array((const Vector *)vec);
}

static size_t vector_hash_adpt(const Container *vec) {
    return vector_hash((const Vector *)vec);
}

static bool vector_insert_adpt(Container *vec, const void *item) {
    return vector_push((Vector *)vec, item) == LC_OK;
}

static void vector_clear_adpt(Container *vec) {
    vector_clear((Vector *)vec);
}

static void vector_destroy_adpt(Container *vec) {
    vector_destroy((Vector *)vec);
}

#endif /* VECTOR_IMPLEMENTATION_ONCE */
#endif /* VECTOR_IMPLEMENTATION */
