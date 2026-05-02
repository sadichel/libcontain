/** @file container.h
 * @brief Generic Container Interface and Iterator Protocol
 * libcontain - https://github.com/sadichel/libcontain 
 * 
 * This header defines the core abstraction layer that unifies all container
 * types (Vector, Deque, LinkedList, HashSet, HashMap) under a single
 * polymorphic interface. It enables generic algorithms, iterators, and
 * functional pipelines to operate on any container type without knowing
 * its concrete implementation.
 *
 * @section key_concepts Key Concepts
 *   - @ref Container — Common header embedded in all container types
 *   - @ref ContainerVTable — Virtual method table for polymorphic dispatch
 *   - @ref Iterator — Lightweight cursor for traversing containers
 *   - @ref IteratorVTable — Virtual method table for iterator dispatch
 *   - @ref Array — Flat, read-only snapshot of container elements
 *   - @ref IterDirection — Forward or reverse iteration direction
 *
 * @section design_philosophy Design Philosophy
 *   - Type-erasure via void* — containers store any POD type
 *   - Vtable dispatch — runtime polymorphism for container operations
 *   - Zero-copy iteration — iterators are cursors, not copies
 *   - Single allocation — Array packs header and data together
 *   - Debug mode — define CONTAINER_DEBUG for diagnostic output
 *
 * @section usage_example Usage Example
 * @code
 *   #include "container.h"
 *
 *   // Iterate any container generically
 *   Iterator it = Iter((Container *)vec);
 *   const void *item;
 *   while ((item = iter_next(&it))) {
 *       // Process item
 *   }
 *
 *   // Convert any container to Array
 *   Array *arr = container_as_array((Container *)my_list);
 *   // Use arr->items, arr->len, arr->stride
 *   free(arr);  // Single free for entire allocation
 * @endcode
 */

#ifndef CONTAINER_PDR_H
#define CONTAINER_PDR_H

#define LIBCONTAIN_VERSION_MAJOR 1
#define LIBCONTAIN_VERSION_MINOR 0
#define LIBCONTAIN_VERSION_PATCH 0
#define LIBCONTAIN_VERSION "1.0.0"

#include "lc_utils.h"

/**
 * @defgroup container_vtable Container Virtual Table
 * @brief Virtual method table for polymorphic container dispatch
 *
 * Every concrete container (Vector, Deque, LinkedList, HashSet, HashMap)
 * exposes exactly one static instance of this vtable and stores a pointer
 * to it in Container.ops. All generic container and iterator functions
 * dispatch through this table.
 * @{
 */

/**
 * @struct ContainerVTable
 * @brief Virtual function table for container operations
 *
 * @var ContainerVTable::next
 * Advances iter by one step and returns a pointer to the current
 * element, or NULL when exhausted. Called by iter_base_next.
 * The iterator state (pos, state) is owned and updated by the
 * container's implementation.
 *
 * @var ContainerVTable::instance
 * Creates a new empty container of the same concrete type and
 * configuration as c (same element size, alignment, comparator, etc.)
 * with default initial capacity. Does not copy any elements.
 * Used by iter_collect and chain_collect to materialise pipeline
 * results into the correct type. Returns NULL on allocation failure.
 *
 * @var ContainerVTable::clone
 * Creates a deep copy of c, including all elements. For string
 * containers every string is strdup'd. The clone is fully independent
 * — mutating one does not affect the other. Returns NULL on failure.
 *
 * @var ContainerVTable::as_array
 * Exports all elements into a flat Array (header + data in one
 * allocation). For fixed-size elements the Array is a contiguous block;
 * for string containers it is a pointer table followed by a string pool.
 * Returns NULL if c is empty or on allocation failure. The caller owns
 * the returned Array and must free it with free().
 *
 * @var ContainerVTable::hash
 * Returns a size_t hash of the container's current contents. The result
 * is cached internally and invalidated on any mutation. Returns 0 for
 * an empty container. For order-independent containers (HashSet, HashMap)
 * the hash is commutative — insertion order does not affect the value.
 * For ordered containers (Vector, Deque, LinkedList) the hash is
 * order-sensitive.
 *
 * @var ContainerVTable::insert
 * Inserts a copy of item into c according to the container's semantics:
 *   - Vector / Deque / LinkedList — appends to the back.
 *   - HashSet — inserts if not present; no-op if already there.
 *   - HashMap — item must point to a struct whose first two members are:
 *               void *key; void *value; The key and value pointers are copied into the map.
 * 
 * Returns true on success, false on allocation failure.
 * For string containers item must be a const char * and the container
 * takes a strdup'd copy.
 *
 * @var ContainerVTable::clear
 * Removes all elements from c, freeing any element-owned memory
 * (e.g. strdup'd strings). The container itself remains valid and
 * reusable — its capacity is retained.
 *
 * @var ContainerVTable::destroy
 * Frees all elements and the container itself. The pointer is invalid
 * after this call. Passing NULL is safe (no-op).
 */
typedef struct ContainerVTable ContainerVTable;

/** @} */ /* end of container_vtable group */

/**
 * @defgroup container_header Container Header
 * @brief Common header embedded in all container types
 * @{
 */

/**
 * @struct Container
 * @brief Common header embedded as the first member of every concrete container
 *
 * Because it is the first member, a Container * and a Vector *
 * point to the same address — safe to cast between them.
 *
 * @var Container::items
 * Pointer to the backing storage. For flat containers (Vector, Deque) this is
 * the element buffer. For node-based containers (LinkedList, HashSet, HashMap)
 * this is the bucket array or head/tail struct.
 *
 * @var Container::len
 * Number of elements currently stored.
 *
 * @var Container::capacity
 * Number of elements the backing storage can hold before a reallocation is needed.
 * For node-based containers this is the bucket count.
 *
 * @var Container::ops
 * Pointer to the container's static vtable. Never NULL for a valid container.
 */
typedef struct Container {
    void *items;                    /**< Pointer to backing storage */
    size_t len;                     /**< Number of elements currently stored */
    size_t capacity;                /**< Maximum elements before reallocation */
    const ContainerVTable *ops;     /**< Pointer to container's vtable */
} Container;

/** @} */ /* end of container_header group */

/**
 * @defgroup iteration Iteration Types
 * @brief Iterator and direction types for container traversal
 * @{
 */

/**
 * @enum IterDirection
 * @brief Direction for iterator traversal
 *
 * @var ITER_FORWARD
 * Iteration proceeds from the first element to the last (default for Iter())
 *
 * @var ITER_REVERSE
 * Iteration proceeds from the last element to the first (default for IterReverse())
 *
 * Each container's next() implementation is responsible for checking
 * direction and advancing pos accordingly.
 */
typedef enum {
    ITER_FORWARD,   /**< Forward iteration: first to last */
    ITER_REVERSE    /**< Reverse iteration: last to first */
} IterDirection;

/**
 * @struct IteratorVTable
 * @brief Virtual function table for iterator operations
 *
 * @var IteratorVTable::next
 * Advance the iterator and return the next element, or NULL when exhausted.
 * @see iter_next()
 *
 * @var IteratorVTable::destroy
 * Free all resources owned by the iterator. For stack-allocated base
 * iterators this is a no-op. For heap-allocated decorators this frees
 * the decorator struct, its buffer if any, and recursively destroys
 * its inner iterator(s).
 * @see iter_destroy()
 */
typedef struct IteratorVTable IteratorVTable;

/**
 * @struct Iterator
 * @brief Lightweight cursor over a Container
 *
 * Stack-allocated by Iter() and IterReverse(); heap-allocated by the
 * decorator constructors (iter_filter, iter_map, iter_skip, etc.).
 *
 * @var Iterator::container
 * The Container being iterated. For decorator iterators this is the innermost
 * source container, propagated upward at construction time so that terminals
 * (iter_any, iter_fold, etc.) always have access to the original container context.
 *
 * @var Iterator::state
 * Opaque per-iterator cursor state. For base iterators this is unused
 * (the container's own next() uses pos). Decorator iterators may use this
 * for their own bookkeeping.
 *
 * @var Iterator::pos
 * Current position index. For ordered containers this is the zero-based element
 * index. For IterReverse() it is initialised to c->len and decremented by each
 * container's next().
 *
 * @var Iterator::direction
 * ITER_FORWARD or ITER_REVERSE, set at construction.
 *
 * @var Iterator::ops
 * Vtable dispatching next() and destroy(). Stack-allocated base iterators point
 * to ITER_BASE_OPS whose destroy is a no-op. Heap-allocated decorators point to
 * their own vtable whose destroy frees the decorator and its inner iterators
 * recursively.
 */
typedef struct Iterator {
    const Container *container;     /**< Container being iterated */
    void *state;                    /**< Opaque cursor state */
    size_t pos;                     /**< Current position index */
    IterDirection direction;        /**< Forward or reverse direction */
    const IteratorVTable *ops;      /**< Vtable for iterator operations */
} Iterator;

/**
 * @struct Array
 * @brief Flat, read-only snapshot returned by container_as_array()
 *
 * The header and data are allocated in a single malloc block:
 * @code
 * [ Array header ][ element data / pointer table + string pool ]
 * @endcode
 * Free with free(arr) — never free arr->items separately.
 *
 * @var Array::items
 * Pointer to the first element (or pointer table for string containers).
 * Points into the same allocation.
 *
 * @var Array::stride
 * Size in bytes between consecutive elements. 0 for string containers where
 * items is a char* table.
 *
 * @var Array::len
 * Number of elements.
 */
typedef struct Array {
    uint8_t *items;     /**< Pointer to first element or pointer table */
    size_t stride;      /**< Bytes between consecutive elements (0 for strings) */
    size_t len;         /**< Number of elements */
} Array;

/** @} */ /* end of iteration group */

/**
 * @defgroup iterator_vtable Iterator Virtual Table
 * @brief Virtual function table for iterator operations
 * @{
 */

/**
 * @struct IteratorVTable
 * @brief Virtual function table for iterator dispatch
 */
struct IteratorVTable {
    /** @brief Advance iterator and return next element, or NULL if exhausted */
    const void *(*next)(Iterator *it);
    /** @brief Free all resources owned by the iterator */
    void (*destroy)(Iterator *it);
};

/**
 * @struct ContainerVTable
 * @brief Virtual function table for container dispatch
 */
struct ContainerVTable {
    /** @brief Advance iterator and return current element */
    const void *(*next)(Iterator *iter);
    /** @brief Create new empty container of same type */
    Container *(*instance)(const Container *c);
    /** @brief Create deep independent copy of container */
    Container *(*clone)(const Container *c);
    /** @brief Export all elements into a flat Array */
    Array *(*as_array)(const Container *c);
    /** @brief Return hash of container's current contents */
    size_t (*hash)(const Container *c);
    /** @brief Insert copy of item into container */
    bool (*insert)(Container *c, const void *item);
    /** @brief Remove all elements, retain capacity */
    void (*clear)(Container *c);
    /** @brief Free all elements and the container itself */
    void (*destroy)(Container *c);
};

/** @} */ /* end of iterator_vtable group */

/**
 * @defgroup array_ops Array Operations
 * @brief Helper functions for Array creation and destruction
 * @{
 */

/**
 * @brief Allocate an Array header wrapping an existing buffer
 *
 * Primarily used by container implementations to wrap their internal
 * storage for export. Callers who need an independent snapshot should
 * use container_as_array() instead.
 *
 * @param items   Pointer to existing buffer (NOT copied, NOT owned)
 * @param stride  Size in bytes between consecutive elements
 * @param len     Number of elements
 * @return Allocated Array header, or NULL on allocation failure
 *
 * @warning The buffer is NOT copied and NOT owned — the caller is
 *          responsible for its lifetime.
 */
static inline Array *array_create(void *items, size_t stride, size_t len) {
    Array *arr = (Array *)malloc(sizeof(Array));
    if (!arr) return NULL;
    arr->items = (uint8_t *)items;
    arr->stride = stride;
    arr->len = len;
    return arr;
}

/**
 * @brief Free the Array header only
 *
 * Does NOT free arr->items — the buffer ownership depends on how the Array
 * was created.
 *
 * @param arr Array to destroy (can be NULL)
 *
 * @note For Arrays returned by container_as_array(), the items pointer is
 *       within the same allocation as the header, so free(arr) frees both.
 *       For Arrays created with array_create(), the buffer is external and
 *       must be managed separately.
 */
static inline void array_destroy(Array *arr) {
    if (arr) free(arr);
}

/** @} */ /* end of array_ops group */

/**
 * @defgroup iterator_ops Iterator Operations
 * @brief Core iterator functions for traversal and cleanup
 * @{
 */

/**
 * @brief Advance iterator and return current element
 *
 * @param it Iterator to advance (can be NULL)
 * @return Pointer to current element, or NULL when exhausted or it is NULL
 *
 * @warning For base (non-decorator) iterators the pointer points directly
 *          into the container's backing storage — do not free it and do not
 *          store it across mutations to the container.
 *
 * @warning For map and zip iterators the pointer is into an internal buffer
 *          that is overwritten on the next call — consume the value before
 *          calling iter_next again.
 */
static inline const void *iter_next(Iterator *it) {
    LC_DEBUG_CHECK(it != NULL, "iter_next() - NULL iterator");
    return it ? it->ops->next(it) : NULL;
}

/**
 * @brief Free all resources owned by an iterator
 *
 * @param it Iterator to destroy (can be NULL)
 *
 * @note For stack-allocated base iterators (created by Iter() or IterReverse())
 *       this is a no-op — they own nothing.
 *
 * @note For heap-allocated decorator iterators this frees the decorator struct,
 *       its output buffer if any, and recursively destroys any inner iterators.
 *
 * @warning All consuming terminals (iter_any, iter_all, iter_count, iter_for_each,
 *          iter_fold, iter_collect, iter_collect_in) call iter_destroy internally.
 *          Do not call iter_destroy separately after a terminal — it is a double-free.
 */
static inline void iter_destroy(Iterator *it) {
    if (it) it->ops->destroy(it);
}

/** @} */ /* end of iterator_ops group */

/* ----------------------------------------------------------------------------
 * Base iterator internals (not part of public API)
 * ---------------------------------------------------------------------------- */

static const void *iter_base_next(Iterator *it) {
    return it->container->ops->next(it);
}

static void iter_base_destroy(Iterator *it) {
    (void)it; /* stack-allocated — nothing to free */
}

static void iter_heap_destroy(Iterator *it) {
    free(it);
}

static const IteratorVTable ITER_BASE_OPS = {
    .next    = iter_base_next,
    .destroy = iter_base_destroy
};

static const IteratorVTable ITER_HEAP_OPS = {
    .next    = iter_base_next,
    .destroy = iter_heap_destroy
};

/**
 * @defgroup iterator_constructors Iterator Constructors
 * @brief Functions for creating iterators over containers
 * @{
 */

/**
 * @brief Create a stack-allocated forward iterator over a container
 *
 * The iterator starts before the first element — the first call to
 * iter_next returns element 0.
 *
 * @param c Container to iterate over (must outlive the iterator)
 * @return Stack-allocated forward iterator (value type)
 *
 * @warning Mutating c during iteration produces undefined behaviour.
 *
 * @note The returned Iterator is a value type. Passing it to a decorator
 *       constructor (iter_filter, iter_map, etc.) requires taking its address
 *       or heap-allocating it first; see IntoIter().
 */
static inline Iterator Iter(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "Iter() - NULL container");
    return (Iterator){
        .container = c,
        .direction = ITER_FORWARD,
        .ops = &ITER_BASE_OPS
    };
}

/**
 * @brief Create a stack-allocated reverse iterator over a container
 *
 * The iterator starts past the last element — the first call to iter_next
 * returns element len-1. pos is initialised to c->len and each container's
 * next() decrements it.
 *
 * @param c Container to iterate over (must outlive the iterator)
 * @return Stack-allocated reverse iterator (value type)
 *
 * @note Only meaningful for ordered containers (Vector, Deque, LinkedList).
 *       For HashSet and HashMap the iteration order is unspecified in both
 *       directions.
 *
 * @warning Mutating c during iteration produces undefined behaviour.
 */
static inline Iterator IterReverse(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "IterReverse() - NULL container");
    return (Iterator){
        .container = c,
        .pos = c ? c->len : 0,
        .direction = ITER_REVERSE,
        .ops = &ITER_BASE_OPS
    };
}

/**
 * @brief Create a heap-allocated forward iterator over a container
 *
 * Unlike Iter() which returns a stack-allocated value type, IntoIter()
 * returns a pointer to a heap-allocated Iterator that can be passed to
 * functions that need to take ownership (such as iterator decorators).
 *
 * @param c Container to iterate over (must outlive the iterator)
 * @return Heap-allocated Iterator pointer, or NULL on allocation failure
 *
 * @note The caller must call iter_destroy() when done to free the allocated memory.
 *
 * @par Example
 * @code
 *   Iterator *it = IntoIter(vec);
 *   void *e;
 *   while ((e = iter_next(it))) { process(e); }
 *   iter_destroy(it);
 * @endcode
 */
static inline Iterator *IntoIter(Container *c) {
    LC_DEBUG_CHECK(c != NULL, "IntoIter() - NULL container");
    Iterator *it = (Iterator *)malloc(sizeof(Iterator));
    if (!it) return NULL;
    *it = (Iterator){
        .container = c,
        .direction = ITER_FORWARD,
        .ops = &ITER_HEAP_OPS
    };
    return it;
}

/**
 * @brief Create a heap-allocated reverse iterator over a container
 *
 * Similar to IntoIter() but iterates from the last element to the first.
 *
 * @param c Container to iterate over (must outlive the iterator)
 * @return Heap-allocated Iterator pointer, or NULL on allocation failure
 *
 * @note The caller must call iter_destroy() when done to free the allocated memory.
 *
 * @par Example
 * @code
 *   Iterator *it = IntoIterReverse(vec);
 *   void *e;
 *   while ((e = iter_next(it))) { process(e); }
 *   iter_destroy(it);
 * @endcode
 */
static inline Iterator *IntoIterReverse(Container *c) {
    LC_DEBUG_CHECK(c != NULL, "IntoIterReverse() - NULL container");
    Iterator *it = (Iterator *)malloc(sizeof(Iterator));
    if (!it) return NULL;
    *it = (Iterator){
        .container = c,
        .pos = c ? c->len : 0,
        .direction = ITER_REVERSE,
        .ops = &ITER_HEAP_OPS
    };
    return it;
}

/** @} */ /* end of iterator_constructors group */

/**
 * @defgroup container_ops Generic Container Operations
 * @brief Polymorphic operations that work on any container type
 * @{
 */

/**
 * @brief Create a new empty container of the same type as c
 *
 * Equivalent to calling the type-specific create function with the
 * same element size, alignment, and comparator as c, but with default
 * capacity.
 *
 * @param c Source container (can be NULL)
 * @return New empty container, or NULL if c is NULL or allocation fails
 *
 * @note Used internally by iter_collect and chain_collect to create the
 *       result container without knowing the concrete type.
 */
static inline Container *container_instance(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_instance() - NULL container");
    return c ? c->ops->instance(c) : NULL;
}

/**
 * @brief Create a deep independent copy of a container
 *
 * All elements are copied; for string containers every string is strdup'd.
 * Mutating the clone does not affect the original and vice versa.
 *
 * @param c Container to clone (can be NULL)
 * @return Deep copy of c, or NULL on allocation failure
 *
 * @note The caller owns the returned container and must destroy it with
 *       container_destroy().
 */
static inline Container *container_clone(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_clone() - NULL container");
    return c ? c->ops->clone(c) : NULL;
}

/**
 * @brief Get the number of elements in a container
 *
 * @param c Container (can be NULL)
 * @return Number of elements, or 0 if c is NULL
 */
static inline size_t container_len(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_len() - NULL container");
    return c ? c->len : 0;
}

/**
 * @brief Compute a hash of the container's current contents
 *
 * The value is cached after the first call and invalidated by any mutation.
 * Returns 0 for an empty container.
 *
 * @param c Container (can be NULL)
 * @return Hash value, or 0 if c is NULL
 *
 * @par Ordering Guarantees
 *   - For order-independent containers (HashSet, HashMap) equal containers
 *     with different insertion orders produce the same hash.
 *   - For ordered containers (Vector, Deque, LinkedList) the hash is
 *     sensitive to element order.
 *
 * @invariant If container_equals(A, B) then container_hash(A) must equal
 *            container_hash(B).
 */
static inline size_t container_hash(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_hash() - NULL container");
    return c ? c->ops->hash(c) : 0;
}

/**
 * @brief Export all elements of a container into a freshly allocated Array
 *
 * The Array header and its data are in a single allocation — free the
 * entire snapshot with free(arr).
 *
 * @param c Container to export (can be NULL)
 * @return New Array snapshot, or NULL if c is NULL, empty, or allocation fails
 *
 * @par Output Format
 *   - For fixed-size elements: items points to a contiguous block of
 *     len * stride bytes.
 *   - For string containers: items is a char* pointer table followed by
 *     a string pool; stride is 0.
 *
 * @note The element order matches iteration order. For HashSet and HashMap
 *       the order is unspecified.
 */
static inline Array *container_as_array(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_as_array() - NULL container");
    return c ? c->ops->as_array(c) : NULL;
}

/**
 * @brief Check if a container is empty
 *
 * @param c Container (can be NULL)
 * @return true if container has no elements or c is NULL, false otherwise
 */
static inline bool container_is_empty(const Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_is_empty() - NULL container");
    return c ? c->len == 0 : true;
}

/**
 * @brief Remove all elements from a container
 *
 * Frees any element-owned memory (e.g. strdup'd strings). The container
 * itself remains valid with its capacity intact — elements can be inserted
 * immediately after clearing.
 *
 * @param c Container to clear (can be NULL)
 */
static inline void container_clear(Container *c) {
    LC_DEBUG_CHECK(c != NULL, "container_clear() - NULL container");
    if (c) c->ops->clear(c);
}

/**
 * @brief Free all elements and the container itself
 *
 * The pointer is invalid after this call. Equivalent to calling the
 * type-specific destroy function (e.g. vector_destroy).
 *
 * @param c Container to destroy (can be NULL)
 */
static inline void container_destroy(Container *c) {
    if (c) c->ops->destroy(c);
}

/** @} */ /* end of container_ops group */

#endif /* CONTAINER_PDR_H */

