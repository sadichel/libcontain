/** @file deque.h
 * @brief Double-Ended Queue with Contiguous Storage
 * libcontain - https://github.com/sadichel/libcontain 
 * 
 * A type-safe, generic double-ended queue (deque) implementation supporting
 * arbitrary item types. Uses a circular buffer for O(1) amortized operations
 * at both ends.
 *
 * @section key_features Key Features
 *   - @ref type_agnostic — stores any POD type or C strings
 *   - @ref double_ended — O(1) push/pop at front and back
 *   - @ref automatic_resizing — expands by ×2 when full
 *   - @ref circular_buffer — contiguous memory with wrap-around
 *   - @ref string_support — first-class strings with deque_str()
 *   - @ref base_alignment — first element can be over-aligned (SIMD)
 *   - @ref container_interface — implements the generic Container vtable
 *
 * @section alignment_note Alignment Note
 *   Only the base pointer (first element) is aligned to base_align.
 *   The circular buffer maintains this alignment at the start.
 *
 * 
 * @section usage Usage Example
 * @code
 *   #include "deque.h"
 *
 *   Deque *deq = deque_create(sizeof(int));
 *   deque_push_back(deq, &(int){1});
 *   deque_push_front(deq, &(int){0});
 *   int *front = deque_front(deq);  // 0
 *   deque_destroy(deq);
 * @endcode
 * 
 * @note Unlike LinkedList, Deque uses a ring buffer (contiguous storage).
 *       This provides O(1) random access and better cache locality,
 *       but insert/remove in the middle requires shifting elements.
 *       For frequent middle insertions, consider LinkedList instead.
 */

#ifndef DEQUE_PDR_H
#define DEQUE_PDR_H

#include "container.h"

/**
 * @defgroup deque_constants Deque Constants
 * @{
 */

/** @brief Minimum capacity for a new deque (16 elements) */
#define DEQUE_MIN_CAPACITY 16

/** @brief Return value for "not found" in find operations */
#define DEQ_NPOS           SIZE_MAX

/** @} */

/**
 * @defgroup deque_types Deque Types
 * @brief Core types for the deque container
 * @{
 */

/**
 * @struct DequeImpl
 * @brief Opaque implementation structure for deque internals
 */
typedef struct DequeImpl DequeImpl;

/**
 * @struct Deque
 * @brief Double-ended queue container
 *
 * Uses a circular buffer for O(1) amortized operations at both ends.
 *
 * @var Deque::container
 * Common container header (items, len, capacity, ops)
 *
 * @var Deque::cmp
 * Comparator function for elements (NULL = memcmp for fixed-size, strcmp for strings)
 *
 * @var Deque::impl
 * Opaque pointer to implementation details (item_size, stride, head, alignment, cached_hash)
 */
typedef struct Deque {
    Container container;            /**< Common container header */
    lc_Comparator cmp;              /**< Element comparator (NULL = default) */
    size_t head;
    const DequeImpl *impl;          /**< Opaque implementation data */
} Deque;

/**
 * @struct DequeBuilder
 * @brief Fluent configuration builder for deque creation
 *
 * @var DequeBuilder::item_size
 * Size of each element in bytes (0 = string mode, stores char*)
 *
 * @var DequeBuilder::base_align
 * Alignment for the first element (0 = default, must be power of two)
 *
 * @var DequeBuilder::capacity
 * Initial capacity (rounded up to DEQUE_MIN_CAPACITY)
 *
 * @var DequeBuilder::cmp
 * Item comparator (NULL = memcmp for fixed-size, strcmp for strings)
 */
typedef struct {
    size_t item_size;      /**< 0 = string mode (stores char*) */
    size_t base_align;     /**< Alignment for first element (0 = default) */
    size_t capacity;       /**< Initial capacity (rounded up) */
    lc_Comparator cmp;        /**< Item comparator (NULL = default) */
} DequeBuilder;

/** @} */

/**
 * @defgroup deque_creation Creation
 * @brief Functions for creating and destroying deques
 * @{
 */

/**
 * @brief Create a new deque with default capacity
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @return Newly allocated deque, or NULL on allocation failure
 *
 * @warning The caller owns the returned deque and must destroy it with deque_destroy()
 */
Deque *deque_create(size_t item_size);

/**
 * @brief Create a new deque with specified initial capacity
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param capacity  Initial capacity (will be rounded up to DEQUE_MIN_CAPACITY)
 * @return Newly allocated deque, or NULL on allocation failure
 */
Deque *deque_create_with_capacity(size_t item_size, size_t capacity);

/**
 * @brief Create a new deque with a custom comparator
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param cmp       Element comparator function
 * @return Newly allocated deque, or NULL on allocation failure
 */
Deque *deque_create_with_comparator(size_t item_size, lc_Comparator cmp);

/**
 * @brief Create a new deque with aligned first element
 *
 * Useful for SIMD, atomic operations, or cache-line optimisation.
 *
 * @param item_size  Size of each element in bytes (0 for string mode)
 * @param base_align Alignment for the first element (must be power of two)
 * @return Newly allocated deque, or NULL on allocation failure
 *
 * @note Only the first element is aligned to base_align. The circular buffer
 *       maintains this alignment at the start.
 */
Deque *deque_create_aligned(size_t item_size, size_t base_align);

/**
 * @brief Create a new string deque
 *
 * Stores C strings as char* with automatic strdup on insertion.
 *
 * @return Newly allocated string deque, or NULL on allocation failure
 */
Deque *deque_str(void);

/**
 * @brief Create a new string deque with specified initial capacity
 *
 * @param capacity Initial capacity
 * @return Newly allocated string deque, or NULL on allocation failure
 */
Deque *deque_str_with_capacity(size_t capacity);

/**
 * @brief Build a deque using a fluent builder
 *
 * @param b Initialised DequeBuilder
 * @return Newly allocated deque, or NULL on configuration or allocation failure
 *
 * @par Example
 * @code
 *   Deque *deq = deque_builder_build(
 *       deque_builder_alignment(
 *           deque_builder_capacity(
 *               deque_builder(sizeof(int)), 64),
 *           64));
 * @endcode
 */
Deque *deque_builder_build(DequeBuilder b);

/**
 * @brief Destroy a deque and free all resources
 *
 * Frees all elements (including strings in string mode) and the deque itself.
 *
 * @param deq Deque to destroy (can be NULL)
 */
void deque_destroy(Deque *deq);

/**
 * @brief Set the comparator for a deque
 *
 * @param deq Deque to modify
 * @param cmp New comparator function
 * @return LC_OK on success, LC_EINVAL if deq or cmp is NULL,
 *         LC_EBUSY if deque is not empty
 *
 * @note Only allowed on empty deques.
 */
int deque_set_comparator(Deque *deq, lc_Comparator cmp);

/** @} */

/**
 * @defgroup deque_core Core Operations
 * @brief Insert, remove, and modify elements
 * @{
 */

/**
 * @brief Append an element to the back of the deque
 *
 * O(1) amortized.
 *
 * @param deq  Deque to modify
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL if deq or item is NULL,
 *         LC_ENOMEM on allocation failure
 *
 * @par Example
 * @code
 *   int value = 42;
 *   deque_push_back(deq, &value);
 * @endcode
 */
int deque_push_back(Deque *deq, const void *item);

/**
 * @brief Prepend an element to the front of the deque
 *
 * O(1) amortized.
 *
 * @param deq  Deque to modify
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL if deq or item is NULL,
 *         LC_ENOMEM on allocation failure
 */
int deque_push_front(Deque *deq, const void *item);

/**
 * @brief Insert an element at the specified position
 *
 * Shifts elements from pos onward to the right. O(n) in the worst case.
 *
 * @param deq  Deque to modify
 * @param pos  Position to insert at (0 = front, len = back)
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_EBOUNDS if pos > len, LC_ENOMEM on allocation failure
 */
int deque_insert(Deque *deq, size_t pos, const void *item);

/**
 * @brief Insert a range of elements from another deque
 *
 * @param dst  Destination deque
 * @param pos  Position to insert at
 * @param src  Source deque
 * @param from Start index in src (inclusive)
 * @param to   End index in src (exclusive)
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_EBOUNDS on invalid indices, LC_ENOMEM on allocation failure
 */
int deque_insert_range(Deque *dst, size_t pos, const Deque *src, size_t from, size_t to);

/**
 * @brief Append all elements from another deque
 *
 * @param dst Destination deque
 * @param src Source deque
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_ENOMEM on allocation failure
 */
int deque_append(Deque *dst, const Deque *src);

/**
 * @brief Append a range of elements from another deque
 *
 * @param dst  Destination deque
 * @param src  Source deque
 * @param from Start index in src (inclusive)
 * @param to   End index in src (exclusive)
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_EBOUNDS on invalid indices, LC_ENOMEM on allocation failure
 */
int deque_append_range(Deque *dst, const Deque *src, size_t from, size_t to);

/**
 * @brief Set an element at the specified position
 *
 * @param deq  Deque to modify
 * @param pos  Position to set
 * @param item Pointer to new element value
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_EBOUNDS if pos >= len
 */
int deque_set(Deque *deq, size_t pos, const void *item);

/**
 * @brief Remove and return the first element
 *
 * O(1) amortized.
 *
 * @param deq Deque to modify
 * @return LC_OK on success, LC_EINVAL if deq is NULL,
 *         LC_EBOUNDS if deque is empty
 */
int deque_pop_front(Deque *deq);

/**
 * @brief Remove and return the last element
 *
 * O(1) amortized.
 *
 * @param deq Deque to modify
 * @return LC_OK on success, LC_EINVAL if deq is NULL,
 *         LC_EBOUNDS if deque is empty
 */
int deque_pop_back(Deque *deq);

/**
 * @brief Remove an element at the specified position
 *
 * Shifts subsequent elements left. O(n) in the worst case.
 *
 * @param deq Deque to modify
 * @param pos Position to remove
 * @return LC_OK on success, LC_EINVAL if deq is NULL,
 *         LC_EBOUNDS if pos >= len
 */
int deque_remove(Deque *deq, size_t pos);

/**
 * @brief Remove a range of elements
 *
 * @param deq  Deque to modify
 * @param from Start index (inclusive)
 * @param to   End index (exclusive)
 * @return LC_OK on success, LC_EINVAL if deq is NULL,
 *         LC_EBOUNDS if from >= to or to > len
 */
int deque_remove_range(Deque *deq, size_t from, size_t to);

/**
 * @brief Splice operation: remove and insert in one pass
 *
 * Removes remove_count elements at pos, then inserts elements from
 * src[src_from:src_to] at the same position.
 *
 * @param dst          Destination deque
 * @param pos          Position to splice at
 * @param remove_count Number of elements to remove
 * @param src          Source deque (can be NULL if insert_count == 0)
 * @param src_from     Start index in src (inclusive)
 * @param src_to       End index in src (exclusive)
 * @return LC_OK on success, various error codes on failure
 */
int deque_splice(Deque *dst, size_t pos, size_t remove_count, const Deque *src, size_t src_from, size_t src_to);

/**
 * @brief Remove duplicate adjacent elements (requires sorted deque)
 *
 * @param deq Deque that must be sorted (behaviour undefined if unsorted)
 * @return LC_OK on success, LC_EINVAL if deq is NULL
 */
int deque_unique(Deque *deq);

/**
 * @brief Trim excess capacity (alias for deque_shrink_to_fit)
 *
 * @param deq Deque to trim
 * @return LC_OK on success, LC_EINVAL if deq is NULL
 */
int deque_trim(Deque *deq);

/**
 * @brief Shrink capacity to fit current length
 *
 * Reduces memory usage by reallocating to exactly fit the current elements.
 * Failure is non-fatal; original buffer is kept.
 *
 * @param deq Deque to shrink
 * @return LC_OK on success (or if shrink fails but deque remains valid),
 *         LC_EINVAL if deq is NULL
 */
int deque_shrink_to_fit(Deque *deq);

/**
 * @brief Reserve capacity for expected number of elements
 *
 * Pre-allocates memory to avoid repeated reallocations.
 *
 * @param deq      Deque to reserve
 * @param capacity Desired capacity
 * @return LC_OK on success, LC_EINVAL if deq is NULL,
 *         LC_EOVERFLOW on overflow, LC_ENOMEM on allocation failure
 */
int deque_reserve(Deque *deq, size_t capacity);

/**
 * @brief Resize the deque to a new length
 *
 * For fixed-size deques:
 *   - Growing adds zero-initialized elements
 *   - Shrinking removes elements from the end
 *
 * For string deques:
 *   - Only shrinking is supported (returns LC_ENOTSUP for growth)
 *
 * @param deq     Deque to resize
 * @param new_len Desired length
 * @return LC_OK on success, LC_ENOTSUP if growing a string deque,
 *         LC_ENOMEM on allocation failure
 */
int deque_resize(Deque *deq, size_t new_len);

/**
 * @brief Remove all elements from the deque
 *
 * Capacity is retained. For string deques, all strings are freed.
 *
 * @param deq Deque to clear
 * @return LC_OK on success, LC_EINVAL if deq is NULL
 */
int deque_clear(Deque *deq);

/** @} */

/**
 * @defgroup deque_inplace In-place Operations
 * @brief Transform the deque in place
 * @{
 */

/**
 * @brief Reverse the order of elements in place
 *
 * O(n) time.
 *
 * @param deq Deque to reverse
 * @return LC_OK on success, LC_EINVAL if deq is NULL,
 *         LC_ENOMEM on allocation failure (only for large stride)
 */
int deque_reverse_inplace(Deque *deq);

/**
 * @brief Sort the deque in place
 *
 * Uses qsort() internally. Requires normalization (handled automatically).
 *
 * @param deq Deque to sort
 * @param cmp Comparator (if NULL, uses deque's stored comparator)
 * @return LC_OK on success, LC_EINVAL if deq is NULL or no comparator available
 */
int deque_sort(Deque *deq, lc_Comparator cmp);

/** @} */

/**
 * @defgroup deque_queries Queries
 * @brief Read element data and query properties
 * @{
 */

/**
 * @brief Get a pointer to an element (read-only)
 *
 * @param deq Deque to query
 * @param pos Position to access
 * @return Pointer to the element, or NULL if deq is NULL or pos out of bounds
 *
 * @warning The returned pointer points directly into the deque's storage.
 *          It may be invalidated by any operation that modifies the deque.
 */
const void *deque_at(const Deque *deq, size_t pos);

/**
 * @brief Get a pointer to the first element (read-only)
 *
 * @param deq Deque to query
 * @return Pointer to the first element, or NULL if deque is empty or NULL
 */
const void *deque_front(const Deque *deq);

/**
 * @brief Get a pointer to the last element (read-only)
 *
 * @param deq Deque to query
 * @return Pointer to the last element, or NULL if deque is empty or NULL
 */
const void *deque_back(const Deque *deq);

/**
 * @brief Get a mutable pointer to an element
 *
 * @param deq Deque to query
 * @param pos Position to access
 * @return Mutable pointer to the element, or NULL if deq is NULL or pos out of bounds
 *
 * @warning Modifying the element via this pointer does not invalidate the
 *          cached hash. Call deque_hash() again if needed.
 */
void *deque_at_mut(const Deque *deq, size_t pos);

/**
 * @brief Get a mutable pointer to the first element
 *
 * @param deq Deque to query
 * @return Mutable pointer to the first element, or NULL if deque is empty or NULL
 */
void *deque_front_mut(const Deque *deq);

/**
 * @brief Get a mutable pointer to the last element
 *
 * @param deq Deque to query
 * @return Mutable pointer to the last element, or NULL if deque is empty or NULL
 */
void *deque_back_mut(const Deque *deq);

/**
 * @brief Find the first occurrence of an element
 *
 * @param deq  Deque to search
 * @param item Element to find
 * @return Index of the element, or DEQ_NPOS if not found
 */
size_t deque_find(const Deque *deq, const void *item);

/**
 * @brief Find the last occurrence of an element (reverse find)
 *
 * @param deq  Deque to search
 * @param item Element to find
 * @return Index of the element, or DEQ_NPOS if not found
 */
size_t deque_rfind(const Deque *deq, const void *item);

/**
 * @brief Check if an element exists in the deque
 *
 * @param deq  Deque to search
 * @param item Element to find
 * @return true if element exists, false otherwise
 */
bool deque_contains(const Deque *deq, const void *item);

/**
 * @brief Check if the deque is empty
 *
 * @param deq Deque to query
 * @return true if deq is NULL or contains no elements, false otherwise
 */
bool deque_is_empty(const Deque *deq);

/**
 * @brief Check if two deques are equal
 *
 * Compares length, element sizes, and each element using the comparator.
 *
 * @param A First deque
 * @param B Second deque
 * @return true if deques are equal, false otherwise
 */
bool deque_equals(const Deque *A, const Deque *B);

/**
 * @brief Get the number of elements in the deque
 *
 * @param deq Deque to query
 * @return Number of elements, or 0 if deq is NULL
 */
size_t deque_len(const Deque *deq);

/**
 * @brief Get the current capacity of the deque
 *
 * @param deq Deque to query
 * @return Current capacity, or 0 if deq is NULL
 */
size_t deque_capacity(const Deque *deq);

/**
 * @brief Compute a hash of the deque's contents
 *
 * The hash is cached and invalidated on any mutation.
 *
 * @param deq Deque to hash
 * @return Hash value, or 0 if deq is NULL or empty
 */
size_t deque_hash(const Deque *deq);

/** @} */

/**
 * @defgroup deque_copy Copy & View Operations
 * @brief Create new deques from existing ones
 * @{
 */

/**
 * @brief Create a new deque with elements in reverse order
 *
 * @param deq Source deque
 * @return New deque with reversed elements, or NULL on allocation failure
 */
Deque *deque_reverse(const Deque *deq);

/**
 * @brief Create a new empty deque of the same type
 *
 * @param deq Source deque (determines item size, alignment, comparator)
 * @return New empty deque, or NULL on allocation failure
 */
Deque *deque_instance(const Deque *deq);

/**
 * @brief Create a deep copy of a deque
 *
 * For string deques, all strings are strdup'd.
 *
 * @param deq Source deque
 * @return Deep copy of the deque, or NULL on allocation failure
 */
Deque *deque_clone(const Deque *deq);

/**
 * @brief Extract a slice of the deque as a new deque
 *
 * @param deq  Source deque
 * @param from Start index (inclusive)
 * @param to   End index (exclusive)
 * @return New deque containing the slice, or NULL on invalid range or allocation failure
 */
Deque *deque_slice(const Deque *deq, size_t from, size_t to);

/** @} */

/**
 * @defgroup deque_conversion Conversion & Iteration
 * @brief Export to Array and create iterators
 * @{
 */

/**
 * @brief Export the deque to a flat Array snapshot
 *
 * The Array header and data are in a single allocation — free with free(arr).
 *
 * @param deq Deque to export
 * @return Array snapshot, or NULL if deq is NULL or empty or allocation fails
 */
Array *deque_to_array(const Deque *deq);

/**
 * @brief Create a forward iterator over the deque
 *
 * @param deq Deque to iterate
 * @return Forward iterator (stack-allocated)
 */
Iterator deque_iter(const Deque *deq);

/**
 * @brief Create a reverse iterator over the deque
 *
 * @param deq Deque to iterate
 * @return Reverse iterator (stack-allocated)
 */
Iterator deque_iter_reversed(const Deque *deq);

/** @} */

#endif /* DEQUE_PDR_H */

/**
 * @defgroup deque_implementation Implementation
 * @brief STB-style implementation section
 *
 * To include the implementation, define DEQUE_IMPLEMENTATION in ONE .c file
 * before including this header:
 *
 * @code
 *   #define DEQUE_IMPLEMENTATION
 *   #include "deque.h"
 * @endcode
 *
 * @note The implementation is intentionally not documented in detail as it is
 *       meant to be included only once per project.
 */

#ifdef DEQUE_IMPLEMENTATION
#ifndef DEQUE_IMPLEMENTATION_ONCE
#define DEQUE_IMPLEMENTATION_ONCE

/* ============================================================================
 * Deque — Implementation
 * ============================================================================
 *
 * Important Notes:
 *   • Circular buffer with head index tracking start position
 *   • Normalization flattens buffer for reallocation and sorting
 *   • String mode (item_size == 0) stores char* pointers, each strdup'd
 *   • Base alignment applies to the start of the circular buffer
 *   • Cached hash invalidated on any mutation
 * ============================================================================ */

/* -------------------------------------------------------------------------
 * Container vtable
 * ------------------------------------------------------------------------- */
static const void *deque_next(Iterator *iter);
static Container *deque_instance_adpt(const Container *deq);
static Container *deque_clone_adpt(const Container *deq);
static Array *deque_to_array_adpt(const Container *deq);
static size_t deque_hash_adpt(const Container *deq);
static bool deque_insert_adpt(Container *deq, const void *item);
static void deque_clear_adpt(Container *deq);
static void deque_destroy_adpt(Container *deq);

static const ContainerVTable DEQUE_CONTAINER_OPS = {
    .next     = deque_next,
    .instance = deque_instance_adpt,
    .clone    = deque_clone_adpt,
    .as_array = deque_to_array_adpt,
    .hash     = deque_hash_adpt,
    .insert   = deque_insert_adpt,
    .clear    = deque_clear_adpt,
    .destroy  = deque_destroy_adpt
};

/* -------------------------------------------------------------------------
 * Internal structures
 * ------------------------------------------------------------------------- */
struct DequeImpl {
    uint16_t item_size;      /* 0 = string mode */
    uint16_t base_align;     /* alignment for first element */
    uint32_t stride;         /* = item_size (no padding) */
};

/* Layout calculation result */
typedef struct {
    uint16_t base_align;
    uint32_t stride;
} DequeEntryLayout;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Single-step wrap — only valid when result < 2*cap */
static inline size_t deque_phys_step(size_t head, size_t pos, size_t cap) {
    size_t idx = head + pos;
    return idx >= cap ? idx - cap : idx;
}

/* General wrap — safe for any value */
static inline size_t deque_phys_raw(size_t head, size_t pos, size_t cap) {
    return (head + pos) % cap;
}

/* Calculate new capacity with exponential growth (×2) */
static inline size_t deque_grow(size_t stride, size_t current_cap, size_t min_cap) {
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

/* Flatten circular buffer so logical index 0 is at physical index 0 */
static int deque_normalize(Deque *deq) {
    size_t head = deq->head;
    size_t len = deq->container.len;
    
    if (head == 0 || len == 0) return LC_OK;
    
    DequeImpl *impl = (DequeImpl *)deq->impl;

    size_t cap = deq->container.capacity;
    size_t stride = impl->stride;
    uint8_t *base = (uint8_t *)deq->container.items;

    /* Case 1: Contiguous but offset (linear shift) */
    if (head + len <= cap) {
        memmove(base, base + (head * stride), len * stride);
    }
    /* Case 2: Wrapped (two segments) */
    else {
        size_t seg1_len = cap - head;       /* from head to buffer end */
        size_t seg2_len = len - seg1_len;   /* from buffer start to tail */

        /* Move the smaller segment to temp, then reorganize */
        if (seg1_len <= seg2_len) {
            uint8_t *tmp = (uint8_t *)malloc(seg1_len * stride);
            if (!tmp) return LC_ENOMEM;

            memcpy(tmp, base + (head * stride), seg1_len * stride);
            memmove(base + (seg1_len * stride), base, seg2_len * stride);
            memcpy(base, tmp, seg1_len * stride);
            free(tmp);
        } else {
            uint8_t *tmp = (uint8_t *)malloc(seg2_len * stride);
            if (!tmp) return LC_ENOMEM;

            memcpy(tmp, base, seg2_len * stride);
            memmove(base, base + (head * stride), seg1_len * stride);
            memcpy(base + (seg1_len * stride), tmp, seg2_len * stride);
            free(tmp);
        }
    }

    deq->head = 0;
    return LC_OK;
}

/* Expand deque capacity, normalizing first */
static int deque_expand_capacity(Deque *deq, size_t min_capacity) {
    size_t stride = deq->impl->stride;
    size_t new_cap = deque_grow(stride, deq->container.capacity, min_capacity);
    if (!new_cap) return LC_ENOMEM;

    /* Flatten before reallocation */
    int rc = deque_normalize(deq);
    if (rc != LC_OK) return rc;

    const size_t base_align = deq->impl->base_align;
    const size_t total = new_cap * stride;
    const size_t nat_align = lc_alloc_max_align();

    void *new_base;
    if (base_align > nat_align) {
        new_base = lc_alloc_realloc_aligned(deq->container.items, total, base_align);
    } else {
        new_base = realloc(deq->container.items, total);
    }

    if (!new_base) return LC_ENOMEM;

    deq->container.items = new_base;
    deq->container.capacity = new_cap;
    return LC_OK;
}

/* Insert slot range, shifting elements as needed */
static void *deque_insert_slot(Deque *deq, size_t pos, size_t count) {
    if (count == 0 || pos > deq->container.len) return NULL;

    DequeImpl *impl = (DequeImpl *)deq->impl;
    size_t stride = impl->stride;
    size_t old_len = deq->container.len;
    size_t needed = old_len + count;

    if (needed > deq->container.capacity) {
        if (deque_expand_capacity(deq, needed) != LC_OK) return NULL;
    }

    size_t cap = deq->container.capacity;
    uint8_t *base = (uint8_t *)deq->container.items;
    size_t old_head = deq->head;

    /* Choose shift direction based on insertion position */
    if (pos < old_len / 2) {
        /* Shift prefix head-ward (left) */
        size_t new_head = (old_head + cap - count) % cap;
        deq->head = new_head;

        for (size_t i = 0; i < pos; i++) {
            size_t src_phys = deque_phys_step(old_head, i, cap);
            size_t dst_phys = deque_phys_step(new_head, i, cap);
            memcpy(base + (dst_phys * stride), base + (src_phys * stride), stride);
        }
    } else {
        /* Shift suffix tail-ward (right) */
        for (size_t i = old_len; i > pos; i--) {
            size_t src_phys = deque_phys_step(old_head, i - 1, cap);
            size_t dst_phys = deque_phys_raw(old_head, i - 1 + count, cap);
            memcpy(base + (dst_phys * stride), base + (src_phys * stride), stride);
        }
    }

    deq->container.len += count;

    return base + (deque_phys_step(deq->head, pos, cap) * stride);
}

/* Free slot range, shifting remaining elements */
static int deque_free_slot(Deque *deq, size_t from, size_t to) {
    if (from >= to || to > deq->container.len) return LC_EBOUNDS;

    DequeImpl *impl = (DequeImpl *)deq->impl;
    size_t head = deq->head;
    size_t cap = deq->container.capacity;
    size_t stride = impl->stride;
    size_t old_len = deq->container.len;
    size_t gap = to - from;
    uint8_t *base = (uint8_t *)deq->container.items;

    /* Free strings if in string mode */
    if (impl->item_size == 0) {
        for (size_t i = from; i < to; i++) {
            size_t idx = deque_phys_step(head, i, cap);
            free(*(char **)(base + idx * stride));
        }
    }

    /* Shift elements to fill the gap */
    if (from < (old_len - to)) {
        /* Shift prefix tail-ward */
        for (size_t i = from; i > 0; i--) {
            size_t src = deque_phys_step(head, i - 1, cap);
            size_t dst = deque_phys_step(head, i - 1 + gap, cap);
            memcpy(base + dst * stride, base + src * stride, stride);
        }
        deq->head = deque_phys_step(head, gap, cap);
    } else {
        /* Shift suffix head-ward */
        for (size_t i = to; i < old_len; i++) {
            size_t src = deque_phys_step(head, i, cap);
            size_t dst = deque_phys_step(head, i - gap, cap);
            memcpy(base + dst * stride, base + src * stride, stride);
        }
    }

    deq->container.len -= gap;
    return LC_OK;
}

/* Get pointer to slot at logical position */
static inline void *deque_slot_at(const Deque *deq, size_t pos) {
    if (pos >= deq->container.len) return NULL;
    size_t phys = deque_phys_step(deq->head, pos, deq->container.capacity);
    return (uint8_t *)deq->container.items + (phys * deq->impl->stride);
}

/* Compute item layout */
static bool deque_compute_layout(size_t isize, size_t ialign, DequeEntryLayout *out) {
    if (ialign == 0 || (ialign & (ialign - 1)) != 0) return false;

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
static Deque *deque_create_impl(DequeBuilder *cfg, DequeEntryLayout *layout) {
    size_t capacity = cfg->capacity;
    size_t stride = layout->stride;
    size_t base_align = layout->base_align;
    const size_t nat_align = lc_alloc_max_align();

    if (capacity < DEQUE_MIN_CAPACITY) capacity = DEQUE_MIN_CAPACITY;
    if (capacity > SIZE_MAX / stride) return NULL;
    size_t total = capacity * stride;

    Deque *deq = (Deque *)malloc(sizeof(Deque) + sizeof(DequeImpl));
    if (!deq) return NULL;

    void *buffer;
    if (base_align > nat_align) {
        buffer = lc_alloc_malloc_aligned(total, base_align);
    } else {
        buffer = malloc(total);
    }

    if (!buffer) {
        free(deq);
        return NULL;
    }

    DequeImpl *impl = (DequeImpl *)(deq + 1);
    impl->item_size = (uint16_t)cfg->item_size;
    impl->base_align = (uint16_t)base_align;
    impl->stride = (uint32_t)stride;
    deq->head = 0;

    deq->container.items = buffer;
    deq->container.len = 0;
    deq->container.capacity = capacity;
    deq->container.ops = &DEQUE_CONTAINER_OPS;
    deq->cmp = cfg->cmp;
    deq->impl = impl;

    return deq;
}

static Deque *deque_create_from_impl(const Deque *src, size_t capacity) {
    if (!src || capacity == 0) return NULL;

    size_t stride = src->impl->stride;
    size_t base_align = src->impl->base_align;
    size_t item_size = src->impl->item_size;
    const size_t nat_align = lc_alloc_max_align();

    if (capacity < DEQUE_MIN_CAPACITY) capacity = DEQUE_MIN_CAPACITY;
    if (capacity > SIZE_MAX / stride) return NULL;
    size_t total = capacity * stride;

    Deque *deq = (Deque *)malloc(sizeof(Deque) + sizeof(DequeImpl));
    if (!deq) return NULL;

    void *buffer;
    if (base_align > nat_align) {
        buffer = lc_alloc_malloc_aligned(total, base_align);
    } else {
        buffer = malloc(total);
    }

    if (!buffer) {
        free(deq);
        return NULL;
    }

    DequeImpl *impl = (DequeImpl *)(deq + 1);
    impl->item_size = (uint16_t)item_size;
    impl->base_align = (uint16_t)base_align;
    impl->stride = (uint32_t)stride;
    deq->head = 0;

    deq->container.items = buffer;
    deq->container.len = 0;
    deq->container.capacity = capacity;
    deq->container.ops = &DEQUE_CONTAINER_OPS;
    deq->cmp = src->cmp;
    deq->impl = impl;

    return deq;
}

/* -------------------------------------------------------------------------
 * Builder API
 * ------------------------------------------------------------------------- */
DequeBuilder deque_builder(size_t item_size) {
    return (DequeBuilder){
        .item_size = item_size,
        .base_align = 1,
        .capacity = DEQUE_MIN_CAPACITY,
        .cmp = NULL
    };
}

DequeBuilder deque_builder_str(void) {
    return (DequeBuilder){
        .item_size = 0,
        .base_align = 1,
        .capacity = DEQUE_MIN_CAPACITY,
        .cmp = NULL
    };
}

DequeBuilder deque_builder_capacity(DequeBuilder b, size_t capacity) {
    b.capacity = capacity;
    return b;
}

DequeBuilder deque_builder_comparator(DequeBuilder b, lc_Comparator cmp) {
    b.cmp = cmp;
    return b;
}

DequeBuilder deque_builder_alignment(DequeBuilder b, size_t base_align) {
    b.base_align = base_align;
    return b;
}

Deque *deque_builder_build(DequeBuilder b) {
    if (b.capacity == 0) return NULL;

    DequeEntryLayout layout;
    if (!deque_compute_layout(b.item_size, b.base_align, &layout)) return NULL;

    return deque_create_impl(&b, &layout);
}

/* -------------------------------------------------------------------------
 * Public creation functions
 * ------------------------------------------------------------------------- */
Deque *deque_create(size_t item_size) {
    return deque_builder_build(deque_builder(item_size));
}

Deque *deque_create_with_capacity(size_t item_size, size_t capacity) {
    return deque_builder_build(deque_builder_capacity(deque_builder(item_size), capacity));
}

Deque *deque_create_with_comparator(size_t item_size, lc_Comparator cmp) {
    return deque_builder_build(deque_builder_comparator(deque_builder(item_size), cmp));
}

Deque *deque_create_aligned(size_t item_size, size_t base_align) {
    return deque_builder_build(deque_builder_alignment(deque_builder(item_size), base_align));
}

Deque *deque_str(void) {
    return deque_builder_build(deque_builder_str());
}

Deque *deque_str_with_capacity(size_t capacity) {
    return deque_builder_build(deque_builder_capacity(deque_builder_str(), capacity));
}

/* -------------------------------------------------------------------------
 * Destruction & configuration
 * ------------------------------------------------------------------------- */
void deque_destroy(Deque *deq) {
    if (!deq) return;

    /* Free strings in string mode */
    if (deq->impl->item_size == 0 && deq->container.len > 0) {
        size_t len = deq->container.len;
        size_t cap = deq->container.capacity;
        size_t head = deq->head;
        size_t stride = deq->impl->stride;
        uint8_t *base = (uint8_t *)deq->container.items;

        for (size_t i = 0; i < len; i++) {
            size_t idx = deque_phys_raw(head, i, cap);
            free(*(char **)(base + (idx * stride)));
        }
    }

    /* Free buffer with appropriate alignment */
    if (deq->impl->base_align > lc_alloc_max_align())
        lc_alloc_free(deq->container.items);
    else
        free(deq->container.items);

    free(deq);
}

int deque_set_comparator(Deque *deq, lc_Comparator cmp) {
    if (!deq || !cmp) return LC_EINVAL;
    if (deq->container.len > 0) return LC_EBUSY;
    deq->cmp = cmp;
    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Core operations
 * ------------------------------------------------------------------------- */
static int deque_append_strings(Deque *dst, size_t pos, const Deque *src, size_t from, size_t to) {
    size_t count = to - from;
    char **temp = (char **)malloc(count * sizeof(char *));
    if (!temp) return LC_ENOMEM;

    for (size_t i = 0; i < count; i++) {
        char **src_slot = (char **)deque_slot_at((Deque *)src, from + i);
        temp[i] = strdup(*src_slot);
        if (!temp[i]) {
            while (i--) free(temp[i]);
            free(temp);
            return LC_ENOMEM;
        }
    }

    char **slots = (char **)deque_insert_slot(dst, pos, count);
    if (!slots) {
        for (size_t i = 0; i < count; i++) free(temp[i]);
        free(temp);
        return LC_ENOMEM;
    }

    memcpy(slots, temp, count * sizeof(char *));
    free(temp);
    return LC_OK;
}

static int deque_append_fixed(Deque *dst, size_t pos, const Deque *src, size_t from, size_t to) {
    size_t count = to - from;
    size_t stride = dst->impl->stride;
    uint8_t *tmp = NULL;

    /* Handle self-append: snapshot before dst layout changes */
    if (dst == src) {
        tmp = (uint8_t *)malloc(count * stride);
        if (!tmp) return LC_ENOMEM;

        for (size_t i = 0; i < count; i++) {
            memcpy(tmp + (i * stride), deque_slot_at(src, from + i), stride);
        }
    }

    uint8_t *dst_slots = (uint8_t *)deque_insert_slot(dst, pos, count);
    if (!dst_slots) {
        free(tmp);
        return LC_ENOMEM;
    }

    if (tmp) {
        memcpy(dst_slots, tmp, count * stride);
        free(tmp);
    } else {
        const uint8_t *src_base = (const uint8_t *)src->container.items;
        size_t src_cap = src->container.capacity;
        size_t src_head = src->head;
        size_t phys_start = deque_phys_raw(src_head, from, src_cap);
        size_t len1 = (phys_start + count <= src_cap) ? count : (src_cap - phys_start);

        memcpy(dst_slots, src_base + (phys_start * stride), len1 * stride);
        if (len1 < count) {
            memcpy(dst_slots + (len1 * stride), src_base, (count - len1) * stride);
        }
    }

    return LC_OK;
}

static int deque_append_impl(Deque *dst, size_t pos, const Deque *src, size_t from, size_t to) {
    if (dst->impl->item_size != src->impl->item_size) return LC_ETYPE;
    if (pos > dst->container.len) return LC_EBOUNDS;
    if (to > src->container.len) return LC_EBOUNDS;
    if (from >= to) return LC_OK;

    if (dst->impl->item_size == 0) {
        return deque_append_strings(dst, pos, src, from, to);
    }
    return deque_append_fixed(dst, pos, src, from, to);
}

static int deque_insert_impl(Deque *deq, size_t pos, const void *item) {
    if (pos > deq->container.len) return LC_EBOUNDS;

    if (deq->impl->item_size == 0) {
        char *str = strdup((const char *)item);
        if (!str) return LC_ENOMEM;

        void *slot = deque_insert_slot(deq, pos, 1);
        if (!slot) {
            free(str);
            return LC_ENOMEM;
        }
        *(char **)slot = str;
    } else {
        void *slot = deque_insert_slot(deq, pos, 1);
        if (!slot) return LC_ENOMEM;
        memcpy(slot, item, deq->impl->stride);
    }

    return LC_OK;
}

int deque_push_back(Deque *deq, const void *item) {
    if (!deq || !item) return LC_EINVAL;
    return deque_insert_impl(deq, deq->container.len, item);
}

int deque_push_front(Deque *deq, const void *item) {
    if (!deq || !item) return LC_EINVAL;
    return deque_insert_impl(deq, 0, item);
}

int deque_insert(Deque *deq, size_t pos, const void *item) {
    if (!deq || !item) return LC_EINVAL;
    return deque_insert_impl(deq, pos, item);
}

int deque_insert_range(Deque *dst, size_t pos, const Deque *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    return deque_append_impl(dst, pos, src, from, to);
}

int deque_append(Deque *dst, const Deque *src) {
    if (!dst || !src) return LC_EINVAL;
    return deque_append_impl(dst, dst->container.len, src, 0, src->container.len);
}

int deque_append_range(Deque *dst, const Deque *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    return deque_append_impl(dst, dst->container.len, src, from, to);
}

int deque_set(Deque *deq, size_t pos, const void *item) {
    if (!deq || !item) return LC_EINVAL;
    if (pos >= deq->container.len) return LC_EBOUNDS;

    void *slot = deque_slot_at(deq, pos);
    int rc = lc_slot_set(slot, item, deq->impl->item_size);

    return rc;
}

int deque_remove(Deque *deq, size_t pos) {
    if (!deq) return LC_EINVAL;
    if (pos >= deq->container.len) return LC_EBOUNDS;
    return deque_free_slot(deq, pos, pos + 1);
}

int deque_remove_range(Deque *deq, size_t from, size_t to) {
    if (!deq) return LC_EINVAL;
    return deque_free_slot(deq, from, to);
}

int deque_pop_front(Deque *deq) {
    return deque_remove(deq, 0);
}

int deque_pop_back(Deque *deq) {
    if (!deq) return LC_EINVAL;
    if (deq->container.len == 0) return LC_EBOUNDS;
    return deque_free_slot(deq, deq->container.len - 1, deq->container.len);
}

int deque_shrink_to_fit(Deque *deq) {
    if (!deq) return LC_EINVAL;

    const size_t len = deq->container.len;
    const size_t cap = deq->container.capacity;

    if (cap <= DEQUE_MIN_CAPACITY) return LC_OK;

    const size_t new_cap = (len > DEQUE_MIN_CAPACITY) ? len : DEQUE_MIN_CAPACITY;
    if (new_cap == cap) return LC_OK;

    int rc = deque_normalize(deq);
    if (rc != LC_OK) return rc;

    const size_t stride = deq->impl->stride;
    const size_t base_align = deq->impl->base_align;
    const size_t nat_align = lc_alloc_max_align();
    const size_t total = new_cap * stride;

    void *new_base;
    if (base_align > nat_align) {
        new_base = lc_alloc_realloc_aligned(deq->container.items, total, base_align);
    } else {
        new_base = realloc(deq->container.items, total);
    }

    if (!new_base) return LC_OK;

    deq->container.items = new_base;
    deq->container.capacity = new_cap;
    return LC_OK;
}

int deque_trim(Deque *deq) {
    return deque_shrink_to_fit(deq);
}

int deque_reserve(Deque *deq, size_t new_cap) {
    if (!deq) return LC_EINVAL;

    size_t old_cap = deq->container.capacity;
    if (new_cap <= old_cap) return LC_OK;

    int rc = deque_normalize(deq);
    if (rc != LC_OK) return rc;

    const size_t stride = deq->impl->stride;
    const size_t base_align = deq->impl->base_align;
    const size_t nat_align = lc_alloc_max_align();

    if (new_cap > SIZE_MAX / stride) return LC_EOVERFLOW;
    const size_t total = new_cap * stride;

    void *new_base;
    if (base_align > nat_align) {
        new_base = lc_alloc_realloc_aligned(deq->container.items, total, base_align);
    } else {
        new_base = realloc(deq->container.items, total);
    }

    if (!new_base) return LC_ENOMEM;

    deq->container.items = new_base;
    deq->container.capacity = new_cap;
    return LC_OK;
}

int deque_resize(Deque *deq, size_t new_len) {
    if (!deq) return LC_EINVAL;

    size_t old_len = deq->container.len;
    if (new_len == old_len) return LC_OK;

    if (new_len < old_len) {
        return deque_free_slot(deq, new_len, old_len);
    }

    /* Prevent upward resize for string vectors */
    if (deq->impl->item_size == 0) {
        return LC_ENOTSUP;
    }

    size_t count = new_len - old_len;
    void *slots = deque_insert_slot(deq, old_len, count);
    if (!slots) return LC_ENOMEM;

    memset(slots, 0, count * deq->impl->stride);
    return LC_OK;
}

int deque_clear(Deque *deq) {
    if (!deq) return LC_EINVAL;
    if (deq->container.len == 0) return LC_OK;

    if (deq->impl->item_size == 0) {
        size_t len = deq->container.len;
        size_t cap = deq->container.capacity;
        size_t head = deq->head;
        size_t stride = deq->impl->stride;
        uint8_t *base = (uint8_t *)deq->container.items;

        for (size_t i = 0; i < len; i++) {
            size_t idx = deque_phys_step(head, i, cap);
            free(*(char **)(base + (idx * stride)));
        }
    }

    deq->head = 0;
    deq->container.len = 0;

    return LC_OK;
}

int deque_reverse_inplace(Deque *deq) {
    if (!deq || deq->container.len < 2) return LC_OK;

    const size_t len = deq->container.len;
    const size_t cap = deq->container.capacity;
    const size_t head = deq->head;
    const size_t stride = deq->impl->stride;
    uint8_t *base = (uint8_t *)deq->container.items;

    uint8_t stack_temp[128];
    uint8_t *temp = (stride <= sizeof(stack_temp)) ? stack_temp : (uint8_t *)malloc(stride);
    if (!temp) return LC_ENOMEM;

    for (size_t i = 0; i < len / 2; i++) {
        size_t left_idx = deque_phys_step(head, i, cap);
        size_t right_idx = deque_phys_step(head, len - 1 - i, cap);

        uint8_t *left = base + (left_idx * stride);
        uint8_t *right = base + (right_idx * stride);

        memcpy(temp, left, stride);
        memcpy(left, right, stride);
        memcpy(right, temp, stride);
    }

    if (temp != stack_temp) free(temp);
    return LC_OK;
}

int deque_splice(Deque *dst, size_t pos, size_t remove_count, const Deque *src, size_t src_from, size_t src_to) {
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

    const size_t final_len = dst->container.len - remove_count + insert_count;
    if (final_len > dst->container.capacity) {
        int rc = deque_reserve(dst, final_len);
        if (rc != LC_OK) return rc;
    }

    uint8_t stack_buf[256];
    uint8_t *temp = NULL;
    const size_t temp_len = insert_count * stride;

    if (insert_count > 0) {
        const size_t src_head = src->head;
        const size_t src_cap = src->container.capacity;
        const uint8_t *src_base = (const uint8_t *)src->container.items;

        temp = (temp_len <= sizeof(stack_buf)) ? stack_buf : (uint8_t *)malloc(temp_len);
        if (!temp) return LC_ENOMEM;

        for (size_t i = 0; i < insert_count; i++) {
            size_t phys = deque_phys_raw(src_head, src_from + i, src_cap);
            const void *src_slot = src_base + (phys * stride);

            if (isize == 0) {
                const char *s = *(const char **)src_slot;
                char *dup = s ? strdup(s) : NULL;
                if (s && !dup) {
                    for (size_t j = 0; j < i; j++)
                        free(*(char **)(temp + (j * stride)));
                    if (temp != stack_buf) free(temp);
                    return LC_ENOMEM;
                }
                *(char **)(temp + (i * stride)) = dup;
            } else {
                memcpy(temp + (i * stride), src_slot, stride);
            }
        }
    }

    if (remove_count > 0)
        deque_free_slot(dst, pos, pos + remove_count);

    if (insert_count > 0) {
        deque_insert_slot(dst, pos, insert_count);

        const size_t dst_head = dst->head;
        const size_t dst_cap = dst->container.capacity;
        uint8_t *dst_base = (uint8_t *)dst->container.items;

        const size_t phys_start = deque_phys_raw(dst_head, pos, dst_cap);
        const size_t len_to_end = dst_cap - phys_start;

        if (insert_count <= len_to_end) {
            memcpy(dst_base + (phys_start * stride), temp, temp_len);
        } else {
            const size_t first = len_to_end;
            const size_t second = insert_count - first;
            memcpy(dst_base + (phys_start * stride), temp, first * stride);
            memcpy(dst_base, temp + (first * stride), second * stride);
        }
    }

    if (temp && temp != stack_buf) free(temp);
    return LC_OK;
}

int deque_unique(Deque *deq) {
    if (!deq || deq->container.len < 2) return LC_OK;

    int rc = deque_normalize(deq);
    if (rc != LC_OK) return rc;

    const size_t len = deq->container.len;
    const size_t stride = deq->impl->stride;
    const size_t isize = deq->impl->item_size;
    uint8_t *base = (uint8_t *)deq->container.items;
    
    if (deq->cmp) {
        const lc_Comparator cmp = deq->cmp;
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
        
        deq->container.len = write + 1;
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
        
        deq->container.len = write + 1;
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
        
        deq->container.len = write + 1;
    }

    return LC_OK;
}

int deque_sort(Deque *deq, lc_Comparator cmp) {
    if (!deq) return LC_EINVAL;
    if (deq->container.len < 2) return LC_OK;

    int rc = deque_normalize(deq);
    if (rc != LC_OK) return rc;

    lc_Comparator target_cmp = cmp ? cmp : deq->cmp;
    if (!target_cmp) return LC_EINVAL;

    qsort(deq->container.items, deq->container.len, deq->impl->stride, target_cmp);

    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Queries & utilities
 * ------------------------------------------------------------------------- */
const void *deque_at(const Deque *deq, size_t pos) {
    if (!deq) return NULL;
    void *slot = deque_slot_at((Deque *)deq, pos);
    return slot ? lc_slot_get(slot, deq->impl->item_size) : NULL;
}

const void *deque_front(const Deque *deq) {
    if (!deq) return NULL;
    void *slot = deque_slot_at((Deque *)deq, 0);
    return slot ? lc_slot_get(slot, deq->impl->item_size) : NULL;
}

const void *deque_back(const Deque *deq) {
    if (!deq || deq->container.len == 0) return NULL;
    void *slot = deque_slot_at((Deque *)deq, deq->container.len - 1);
    return slot ? lc_slot_get(slot, deq->impl->item_size) : NULL;
}

void *deque_at_mut(const Deque *deq, size_t pos) {
    if (!deq) return NULL;
    return deque_slot_at((Deque *)deq, pos);
}

void *deque_front_mut(const Deque *deq) {
    if (!deq) return NULL;
    return deque_slot_at((Deque *)deq, 0);
}

void *deque_back_mut(const Deque *deq) {
    if (!deq || deq->container.len == 0) return NULL;
    return deque_slot_at((Deque *)deq, deq->container.len - 1);
}

size_t deque_find(const Deque *deq, const void *item) {
    if (!deq || !item || deq->container.len == 0) return DEQ_NPOS;

    const DequeImpl *impl = deq->impl;
    const size_t len = deq->container.len;
    const size_t stride = impl->stride;
    const size_t head = deq->head;
    const size_t cap = deq->container.capacity;
    const size_t isize = impl->item_size;
    const lc_Comparator cmp = deq->cmp;
    const uint8_t *base = (const uint8_t *)deq->container.items;

    for (size_t i = 0; i < len; i++) {
        size_t phys_idx = deque_phys_step(head, i, cap);
        void *current_slot = (void *)(base + (phys_idx * stride));

        if (lc_slot_cmp(lc_slot_get(current_slot, isize), item, isize, cmp) == 0) {
            return i;
        }
    }

    return DEQ_NPOS;
}

size_t deque_rfind(const Deque *deq, const void *item) {
    if (!deq || !item || deq->container.len == 0) return DEQ_NPOS;

    const DequeImpl *impl = deq->impl;
    const size_t len = deq->container.len;
    const size_t stride = impl->stride;
    const size_t head = deq->head;
    const size_t cap = deq->container.capacity;
    const size_t isize = impl->item_size;
    const lc_Comparator cmp = deq->cmp;
    const uint8_t *base = (const uint8_t *)deq->container.items;

    for (size_t i = len; i > 0; i--) {
        size_t logical_idx = i - 1;
        size_t phys_idx = deque_phys_step(head, logical_idx, cap);
        void *current_slot = (void *)(base + (phys_idx * stride));

        if (lc_slot_cmp(lc_slot_get(current_slot, isize), item, isize, cmp) == 0) {
            return logical_idx;
        }
    }

    return DEQ_NPOS;
}

bool deque_contains(const Deque *deq, const void *item) {
    return deque_find(deq, item) != DEQ_NPOS;
}

bool deque_is_empty(const Deque *deq) {
    return !deq || deq->container.len == 0;
}

size_t deque_len(const Deque *deq) {
    return deq ? deq->container.len : 0;
}

size_t deque_size(const Deque *deq) {
    return deque_len(deq);
}

size_t deque_capacity(const Deque *deq) {
    return deq ? deq->container.capacity : 0;
}

size_t deque_hash(const Deque *deq) {
    if (!deq || deq->container.len == 0) return 0;

    DequeImpl *impl = (DequeImpl *)deq->impl;
    const size_t len = deq->container.len;
    const size_t cap = deq->container.capacity;
    const size_t stride = impl->stride;
    const size_t isize = impl->item_size;
    const size_t head = deq->head;
    const uint8_t *base = (const uint8_t *)deq->container.items;

    size_t h = len;

    for (size_t i = 0; i < len; i++) {
        size_t phys_idx = deque_phys_step(head, i, cap);
        void *current_slot = (void *)(base + (phys_idx * stride));
        size_t item_hash = lc_slot_hash(lc_slot_get(current_slot, isize), isize, NULL);
        h ^= item_hash + 0x9e3779b9 + (h << 6) + (h >> 2);
    }

    return lc_hash_mix(h);
}

bool deque_equals(const Deque *A, const Deque *B) {
    if (A == B) return true;
    if (!A || !B) return false;

    const DequeImpl *impl_a = A->impl;
    const DequeImpl *impl_b = B->impl;

    if (A->container.len != B->container.len ||
        impl_a->item_size != impl_b->item_size) {
        return false;
    }

    const size_t n = A->container.len;
    const size_t a_cap = A->container.capacity;
    const size_t b_cap = B->container.capacity;
    const size_t a_head = A->head;
    const size_t b_head = B->head;
    const size_t stride = impl_a->stride;
    const size_t isize = impl_a->item_size;

    const uint8_t *a_base = (const uint8_t *)A->container.items;
    const uint8_t *b_base = (const uint8_t *)B->container.items;

    for (size_t i = 0; i < n; i++) {
        size_t a_idx = deque_phys_step(a_head, i, a_cap);
        size_t b_idx = deque_phys_step(b_head, i, b_cap);

        void *a_slot = (void *)(a_base + (a_idx * stride));
        void *b_slot = (void *)(b_base + (b_idx * stride));

        if (lc_slot_cmp(lc_slot_get(a_slot, isize), lc_slot_get(b_slot, isize), isize, A->cmp) != 0) {
            return false;
        }
    }

    return true;
}

/* -------------------------------------------------------------------------
 * Copy & view operations
 * ------------------------------------------------------------------------- */
Deque *deque_reverse(const Deque *deq) {
    if (!deq) return NULL;

    Deque *rev = deque_create_from_impl(deq, deq->container.len);
    if (!rev) return NULL;

    const DequeImpl *src_impl = deq->impl;
    const size_t n = deq->container.len;
    const size_t head = deq->head;
    const size_t cap = deq->container.capacity;
    const size_t stride = src_impl->stride;
    const size_t isize = src_impl->item_size;

    uint8_t *dst_base = (uint8_t *)rev->container.items;
    const uint8_t *src_base = (const uint8_t *)deq->container.items;

    for (size_t i = 0; i < n; i++) {
        const size_t src_idx = deque_phys_step(head, (n - 1 - i), cap);
        const void *src_item = src_base + (src_idx * stride);
        void *dst_slot = dst_base + (i * stride);

        if (lc_slot_copy(dst_slot, src_item, isize) != LC_OK) {
            deque_destroy(rev);
            return NULL;
        }
        rev->container.len++;
    }

    return rev;
}

Deque *deque_clone(const Deque *deq) {
    if (!deq) return NULL;

    Deque *clone = deque_create_from_impl(deq, deq->container.capacity);
    if (!clone) return NULL;

    const DequeImpl *impl = deq->impl;
    const size_t len = deq->container.len;
    const size_t head = deq->head;
    const size_t cap = deq->container.capacity;
    const size_t isize = impl->item_size;
    const size_t stride = impl->stride;
    const uint8_t *src_base = (const uint8_t *)deq->container.items;
    uint8_t *dst_base = (uint8_t *)clone->container.items;

    if (isize == 0) {
        for (size_t i = 0; i < len; i++) {
            size_t phys_idx = deque_phys_step(head, i, cap);
            void *src_slot = (uint8_t *)src_base + (phys_idx * stride);
            void *dst_slot = dst_base + (i * stride);

            if (lc_slot_copy(dst_slot, src_slot, 0) != LC_OK) {
                deque_destroy(clone);
                return NULL;
            }
            clone->container.len++;
        }
    } else if (len > 0) {
        size_t len1 = (head + len <= cap) ? len : (cap - head);

        memcpy(dst_base, src_base + (head * stride), len1 * stride);
        if (len1 < len) {
            memcpy(dst_base + (len1 * stride), src_base, (len - len1) * stride);
        }
        clone->container.len = len;
    }

    return clone;
}

Deque *deque_slice(const Deque *deq, size_t start, size_t end) {
    if (!deq || start >= end || end > deq->container.len) return NULL;

    const size_t count = end - start;
    Deque *slice = deque_create_from_impl(deq, count);
    if (!slice) return NULL;

    const DequeImpl *impl = deq->impl;
    const size_t isize = impl->item_size;
    const size_t stride = impl->stride;
    const size_t head = deq->head;
    const size_t cap = deq->container.capacity;
    const uint8_t *src_base = (const uint8_t *)deq->container.items;
    uint8_t *dst_base = (uint8_t *)slice->container.items;

    if (isize == 0) {
        for (size_t i = 0; i < count; i++) {
            size_t phy_idx = deque_phys_step(head, start + i, cap);
            void *src_slot = (uint8_t *)src_base + (phy_idx * stride);
            void *dst_slot = dst_base + (i * stride);

            if (lc_slot_copy(dst_slot, src_slot, 0) != LC_OK) {
                deque_destroy(slice);
                return NULL;
            }
            slice->container.len++;
        }
    } else {
        size_t phys_start = deque_phys_step(head, start, cap);
        size_t len1 = (phys_start + count <= cap) ? count : (cap - phys_start);

        memcpy(dst_base, src_base + (phys_start * stride), len1 * stride);
        if (len1 < count) {
            memcpy(dst_base + (len1 * stride), src_base, (count - len1) * stride);
        }
        slice->container.len = count;
    }

    return slice;
}

Deque *deque_instance(const Deque *deq) {
    if (!deq) return NULL;
    return deque_create_from_impl(deq, DEQUE_MIN_CAPACITY);
}

/* -------------------------------------------------------------------------
 * Array conversion
 * ------------------------------------------------------------------------- */
static Array *deque_collect_fixed(const Deque *deq) {
    const size_t deq_len = deq->container.len;
    if (deq_len == 0) return NULL;

    const size_t stride = deq->impl->stride;
    const size_t base_align = deq->impl->base_align;
    const size_t total_items = deq_len * stride;

    if (deq_len > 0 && total_items / deq_len != stride) return NULL;

    size_t align = base_align > 16 ? base_align : 16;
    size_t header_size = sizeof(Array);

    if (header_size > SIZE_MAX - align) return NULL;
    size_t items_offset = (header_size + align - 1) & ~(align - 1);

    if (items_offset > SIZE_MAX - total_items) return NULL;
    size_t total = items_offset + total_items;

    if (total > SIZE_MAX) return NULL;

    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return NULL;

    uint8_t *items = buf + items_offset;

    Array *arr = (Array *)buf;
    arr->items = items;
    arr->stride = stride;
    arr->len = deq_len;

    const size_t head = deq->head;
    const size_t cap = deq->container.capacity;
    const uint8_t *src_base = (const uint8_t *)deq->container.items;

    size_t len1 = (head + deq_len <= cap) ? deq_len : (cap - head);

    memcpy(items, src_base + (head * stride), len1 * stride);
    if (len1 < deq_len) {
        memcpy(items + (len1 * stride), src_base, (deq_len - len1) * stride);
    }

    return arr;
}

static Array *deque_collect_strings(const Deque *deq) {
    const size_t deq_len = deq->container.len;
    if (deq_len == 0) return NULL;

    if (deq_len > SIZE_MAX / sizeof(char *)) return NULL;

    const size_t cap = deq->container.capacity;
    const size_t head = deq->head;
    const size_t stride = deq->impl->stride;
    const uint8_t *base = (const uint8_t *)deq->container.items;

    size_t total_chars = 0;
    for (size_t i = 0; i < deq_len; i++) {
        size_t idx = deque_phys_step(head, i, cap);
        const char *str = *(const char **)(base + (idx * stride));
        size_t len = strlen(str) + 1;
        if (total_chars > SIZE_MAX - len) return NULL;
        total_chars += len;
    }

    size_t ptrs_size = deq_len * sizeof(char *);
    if (total_chars > SIZE_MAX - sizeof(Array) - ptrs_size) return NULL;
    size_t total = sizeof(Array) + ptrs_size + total_chars;

    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return NULL;

    Array *arr = (Array *)buf;
    arr->items = buf + sizeof(Array);
    arr->stride = 0;
    arr->len = 0;

    char **table = (char **)arr->items;
    char *pool = (char *)(arr->items + ptrs_size);
    char *pool_start = pool;

    for (size_t i = 0; i < deq_len; i++) {
        size_t idx = deque_phys_step(head, i, cap);
        const char *str = *(const char **)(base + (idx * stride));

        if (!str) {
            table[i] = NULL;
            continue;
        }

        size_t len = strlen(str) + 1;

        if (pool > pool_start + total_chars) {
            free(buf);
            return NULL;
        }

        table[i] = pool;
        memcpy(pool, str, len);
        pool += len;
        arr->len++;
    }

    return arr;
}

Array *deque_to_array(const Deque *deq) {
    if (!deq) return NULL;

    if (deq->impl->item_size == 0) {
        return deque_collect_strings(deq);
    }
    return deque_collect_fixed(deq);
}

/* -------------------------------------------------------------------------
 * Iterator implementation
 * ------------------------------------------------------------------------- */

static const void *deque_next(Iterator *it) {
    Deque *deq = (Deque *)it->container;
    size_t len = deq->container.len;

    if (it->direction == ITER_FORWARD) {
        if (it->pos >= len) return NULL;
        return deque_at(deq, it->pos++);
    } else {
        if (it->pos == 0) return NULL;
        return deque_at(deq, --it->pos);
    }
}

Iterator deque_iter(const Deque *deq) {
    return Iter((const Container *)deq);
}

Iterator deque_iter_reversed(const Deque *deq) {
    return IterReverse((const Container *)deq);
}

/* -------------------------------------------------------------------------
 * Container adapter functions
 * ------------------------------------------------------------------------- */
static Container *deque_instance_adpt(const Container *deq) {
    return (Container *)deque_instance((const Deque *)deq);
}

static Container *deque_clone_adpt(const Container *deq) {
    return (Container *)deque_clone((const Deque *)deq);
}

static Array *deque_to_array_adpt(const Container *deq) {
    return deque_to_array((const Deque *)deq);
}

static size_t deque_hash_adpt(const Container *deq) {
    return deque_hash((const Deque *)deq);
}

static bool deque_insert_adpt(Container *deq, const void *item) {
    return deque_push_back((Deque *)deq, item) == LC_OK;
}

static void deque_clear_adpt(Container *deq) {
    deque_clear((Deque *)deq);
}

static void deque_destroy_adpt(Container *deq) {
    deque_destroy((Deque *)deq);
}

#endif /* DEQUE_IMPLEMENTATION_ONCE */
#endif /* DEQUE_IMPLEMENTATION */