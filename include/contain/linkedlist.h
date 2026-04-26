/** @file linkedlist.h
 * @brief Doubly-Linked List with Node Pool Allocation
 * libcontain - https://github.com/sadichel/libcontain 
 * 
 * A type-safe, generic doubly-linked list implementation supporting arbitrary
 * item types. Provides O(1) insertion and removal at any position (given an
 * iterator), O(n) random access, and efficient node pool allocation.
 *
 * @section key_features Key Features
 *   - @ref type_agnostic — stores any POD type or C strings
 *   - @ref doubly_linked — efficient bidirectional traversal
 *   - @ref node_pool_allocation — reduces malloc overhead
 *   - @ref custom_comparators — user-defined comparison
 *   - @ref string_support — first-class strings (linkedlist_str)
 *   - @ref alignment_control — per-type item alignment
 *   - @ref container_interface — implements the Container vtable for generic use
 *
 * @section complexity Complexity Guarantees
 *   - **Insert/Remove at known position**: O(1) with iterator
 *   - **Insert/Remove at arbitrary position**: O(n) (requires traversal)
 *   - **Push front/pop front**: O(1)
 *   - **Push back/pop back**: O(1)
 *   - **Random access (at)**: O(n)
 *   - **Find**: O(n)
 *
 * @section memory_management Memory Management
 *   - Node pool allocator reduces malloc overhead
 *   - Splice between lists with same allocator is O(1) (pointer relinking)
 *   - Splice between lists with different allocators copies elements
 *
 * @section usage Usage Example
 * @code
 *   #include "linkedlist.h"
 *
 *   LinkedList *list = linkedlist_create(sizeof(int));
 *   linkedlist_push_back(list, &(int){1});
 *   linkedlist_push_front(list, &(int){0});
 *   int *front = linkedlist_front(list);  // 0
 *   linkedlist_destroy(list);
 * @endcode
 * 
 * @note Unlike Deque (ring buffer), LinkedList uses node-based storage.
 *       This provides O(1) insert/remove at any position (with an iterator),
 *       but random access is O(n) and memory overhead is higher.
 *       For fast random access, consider Deque or Vector instead.
 */

#ifndef LINKEDLIST_PDR_H
#define LINKEDLIST_PDR_H

#include "allocator.h"
#include "container.h"

/**
 * @defgroup linkedlist_constants LinkedList Constants
 * @{
 */

/** @brief Default allocation capacity for node pool allocator */
#define LINKEDLIST_ALLOC_CAPACITY 16

/** @brief Default alignment for node pool allocator */
#define LINKEDLIST_ALLOC_ALIGN    8

/** @brief Return value for "not found" in find operations */
#define LIST_NPOS                 SIZE_MAX

/** @} */

/**
 * @defgroup linkedlist_types LinkedList Types
 * @brief Core types for the linked list container
 * @{
 */

/**
 * @struct LinkedListImpl
 * @brief Opaque implementation structure for linked list internals
 */
typedef struct LinkedListImpl LinkedListImpl;

/**
 * @struct LinkedList
 * @brief Doubly-linked list container
 *
 * @var LinkedList::container
 * Common container header (items, len, capacity, ops)
 *
 * @var LinkedList::alloc
 * Node pool allocator (reduces malloc overhead)
 *
 * @var LinkedList::cmp
 * Comparator function for elements (NULL = memcmp for fixed-size, strcmp for strings)
 *
 * @var LinkedList::impl
 * Opaque pointer to implementation details (item_size, stride, alignment, cached_hash)
 */
typedef struct LinkedList {
    Container  container;       /**< Common container header */
    Allocator *alloc;           /**< Node pool allocator */
    lc_Comparator cmp;          /**< Element comparator (NULL = default) */
    const LinkedListImpl *impl; /**< Opaque implementation data */
} LinkedList;

/**
 * @struct LinkedListBuilder
 * @brief Fluent configuration builder for linked list creation
 *
 * @var LinkedListBuilder::item_size
 * Size of each element in bytes (0 = string mode, stores char*)
 *
 * @var LinkedListBuilder::item_align
 * Alignment for items (0 = default, must be power of two)
 *
 * @var LinkedListBuilder::alloc
 * Custom allocator (NULL = internal pool allocator)
 *
 * @var LinkedListBuilder::cmp
 * Item comparator (NULL = memcmp for fixed-size, strcmp for strings)
 */
typedef struct {
    size_t item_size;      /**< 0 = string mode (stores char*) */
    size_t item_align;     /**< Item alignment (0 = default) */
    Allocator *alloc;      /**< Custom allocator (NULL = internal pool) */
    lc_Comparator cmp;     /**< Item comparator (NULL = memcmp/strcmp) */
} LinkedListBuilder;

/** @} */

/**
 * @defgroup linkedlist_creation Creation
 * @brief Functions for creating and destroying linked lists
 * @{
 */

/**
 * @brief Create a new linked list with default configuration
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @return Newly allocated linked list, or NULL on allocation failure
 *
 * @warning The caller owns the returned list and must destroy it with linkedlist_destroy()
 */
LinkedList *linkedlist_create(size_t item_size);

/**
 * @brief Create a new linked list with a custom allocator
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param alloc     Custom allocator for nodes
 * @return Newly allocated linked list, or NULL on allocation failure
 */
LinkedList *linkedlist_create_with_allocator(size_t item_size, Allocator *alloc);

/**
 * @brief Create a new linked list with a custom comparator
 *
 * @param item_size Size of each element in bytes (0 for string mode)
 * @param cmp       Element comparator function
 * @return Newly allocated linked list, or NULL on allocation failure
 */
LinkedList *linkedlist_create_with_comparator(size_t item_size, lc_Comparator cmp);

/**
 * @brief Create a new linked list with aligned items
 *
 * Useful for SIMD, atomic operations, or cache-line optimisation.
 *
 * @param item_size  Size of each element in bytes (0 for string mode)
 * @param item_align Alignment for items (must be power of two)
 * @return Newly allocated linked list, or NULL on allocation failure
 */
LinkedList *linkedlist_create_aligned(size_t item_size, size_t item_align);

/**
 * @brief Create a new string linked list
 *
 * Stores C strings as char* with automatic strdup on insertion.
 *
 * @return Newly allocated string linked list, or NULL on allocation failure
 */
LinkedList *linkedlist_str(void);

/**
 * @brief Create a new string linked list with a custom comparator
 *
 * @param cmp String comparator function (receives char**)
 * @return Newly allocated string linked list, or NULL on allocation failure
 */
LinkedList *linkedlist_str_with_comparator(lc_Comparator cmp);

/**
 * @brief Create a new string linked list with a custom allocator
 *
 * @param alloc Custom allocator for nodes
 * @return Newly allocated string linked list, or NULL on allocation failure
 */
LinkedList *linkedlist_str_with_allocator(Allocator *alloc);

/**
 * @brief Build a linked list using a fluent builder
 *
 * @param b Initialised LinkedListBuilder
 * @return Newly allocated linked list, or NULL on configuration or allocation failure
 *
 * @par Example
 * @code
 *   LinkedList *list = linkedlist_builder_build(
 *       linkedlist_builder_comparator(
 *           linkedlist_builder(sizeof(int)),
 *           my_compare_function));
 * @endcode
 */
LinkedList *linkedlist_builder_build(LinkedListBuilder b);

/**
 * @brief Destroy a linked list and free all resources
 *
 * Frees all nodes, elements (including strings), and the list itself.
 *
 * @param list Linked list to destroy (can be NULL)
 */
void linkedlist_destroy(LinkedList *list);

/**
 * @brief Set the comparator for a linked list
 *
 * @param list Linked list to modify
 * @param cmp  New comparator function
 * @return LC_OK on success, LC_EINVAL if list or cmp is NULL,
 *         LC_EBUSY if list is not empty
 *
 * @note Only allowed on empty lists.
 */
int linkedlist_set_comparator(LinkedList *list, lc_Comparator cmp);

/**
 * @brief Set the allocator for a linked list
 *
 * @param list  Linked list to modify
 * @param alloc New allocator
 * @return LC_OK on success, LC_EINVAL if list or alloc is NULL,
 *         LC_ETYPE if allocator stride is insufficient,
 *         LC_EBUSY if list is not empty
 *
 * @note Only allowed on empty lists.
 */
int linkedlist_set_allocator(LinkedList *list, Allocator *alloc);

/** @} */

/**
 * @defgroup linkedlist_core Core Operations
 * @brief Insert, remove, and modify elements
 * @{
 */

/**
 * @brief Append an element to the end of the list
 *
 * O(1) operation.
 *
 * @param list Linked list to modify
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL if list or item is NULL,
 *         LC_ENOMEM on allocation failure
 *
 * @par Example
 * @code
 *   int value = 42;
 *   linkedlist_push_back(list, &value);
 * @endcode
 */
int linkedlist_push_back(LinkedList *list, const void *item);

/**
 * @brief Prepend an element to the front of the list
 *
 * O(1) operation.
 *
 * @param list Linked list to modify
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL if list or item is NULL,
 *         LC_ENOMEM on allocation failure
 */
int linkedlist_push_front(LinkedList *list, const void *item);

/**
 * @brief Insert an element at the specified position
 *
 * O(n) in the worst case (traverses to position).
 *
 * @param list Linked list to modify
 * @param pos  Position to insert at (0 = front, len = back)
 * @param item Pointer to element to copy
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_EBOUNDS if pos > len, LC_ENOMEM on allocation failure
 */
int linkedlist_insert(LinkedList *list, size_t pos, const void *item);

/**
 * @brief Insert a range of elements from another list
 *
 * @param dst  Destination list
 * @param pos  Position to insert at
 * @param src  Source list
 * @param from Start index in src (inclusive)
 * @param to   End index in src (exclusive)
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_EBOUNDS on invalid indices, LC_ENOMEM on allocation failure
 */
int linkedlist_insert_range(LinkedList *dst, size_t pos, const LinkedList *src, size_t from, size_t to);

/**
 * @brief Append all elements from another list
 *
 * @param dst Destination list
 * @param src Source list
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_ENOMEM on allocation failure
 */
int linkedlist_append(LinkedList *dst, const LinkedList *src);

/**
 * @brief Append a range of elements from another list
 *
 * @param dst  Destination list
 * @param src  Source list
 * @param from Start index in src (inclusive)
 * @param to   End index in src (exclusive)
 * @return LC_OK on success, LC_ETYPE if item sizes differ,
 *         LC_EBOUNDS on invalid indices, LC_ENOMEM on allocation failure
 */
int linkedlist_append_range(LinkedList *dst, const LinkedList *src, size_t from, size_t to);

/**
 * @brief Set an element at the specified position
 *
 * @param list Linked list to modify
 * @param pos  Position to set
 * @param item Pointer to new element value
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_EBOUNDS if pos >= len
 */
int linkedlist_set(LinkedList *list, size_t pos, const void *item);

/**
 * @brief Remove and return the last element
 *
 * O(1) operation.
 *
 * @param list Linked list to modify
 * @return LC_OK on success, LC_EINVAL if list is NULL,
 *         LC_EBOUNDS if list is empty
 */
int linkedlist_pop_back(LinkedList *list);

/**
 * @brief Remove and return the first element
 *
 * O(1) operation.
 *
 * @param list Linked list to modify
 * @return LC_OK on success, LC_EINVAL if list is NULL,
 *         LC_EBOUNDS if list is empty
 */
int linkedlist_pop_front(LinkedList *list);

/**
 * @brief Remove an element at the specified position
 *
 * O(n) in the worst case (traverses to position).
 *
 * @param list Linked list to modify
 * @param pos  Position to remove
 * @return LC_OK on success, LC_EINVAL if list is NULL,
 *         LC_EBOUNDS if pos >= len
 */
int linkedlist_remove(LinkedList *list, size_t pos);

/**
 * @brief Remove a range of elements
 *
 * @param list Linked list to modify
 * @param from Start index (inclusive)
 * @param to   End index (exclusive)
 * @return LC_OK on success, LC_EINVAL if list is NULL,
 *         LC_EBOUNDS if from >= to or to > len
 */
int linkedlist_remove_range(LinkedList *list, size_t from, size_t to);

/**
 * @brief Splice operation: move a range from source to destination
 *
 * If dst and src share the same allocator, this is O(1) (pointer relinking).
 * Otherwise, elements are copied.
 *
 * @param dst   Destination list
 * @param pos   Position in dst to splice into
 * @param src   Source list (can be same as dst? No, undefined behaviour)
 * @param from  Start index in src (inclusive)
 * @param to    End index in src (exclusive)
 * @return LC_OK on success, LC_EINVAL on invalid input,
 *         LC_ETYPE if item sizes differ, LC_EBOUNDS on invalid indices,
 *         LC_ENOMEM on allocation failure (when copying)
 *
 * @warning dst and src must be different lists.
 */
int linkedlist_splice(LinkedList *dst, size_t pos, LinkedList *src, size_t from, size_t to);

/**
 * @brief Remove all elements from the list
 *
 * All nodes and elements (including strings) are freed.
 *
 * @param list Linked list to clear
 * @return LC_OK on success, LC_EINVAL if list is NULL
 */
int linkedlist_clear(LinkedList *list);

/** @} */

/**
 * @defgroup linkedlist_inplace In-place Operations
 * @brief Transform the list in place
 * @{
 */

/**
 * @brief Reverse the order of elements in place
 *
 * O(n) operation.
 *
 * @param list Linked list to reverse
 * @return LC_OK on success, LC_EINVAL if list is NULL
 */
int linkedlist_reverse_inplace(LinkedList *list);

/**
 * @brief Sort the list in place using merge sort
 *
 * O(n log n) operation. Stable sort.
 *
 * @param list Linked list to sort
 * @param cmp  Comparator (if NULL, uses list's stored comparator)
 * @return LC_OK on success, LC_EINVAL if list is NULL or no comparator available
 */
int linkedlist_sort(LinkedList *list, lc_Comparator cmp);

/**
 * @brief Remove duplicate adjacent elements (requires sorted list)
 *
 * @param list List that must be sorted (behaviour undefined if unsorted)
 * @return LC_OK on success, LC_EINVAL if list is NULL
 */
int linkedlist_unique(LinkedList *list);

/** @} */

/**
 * @defgroup linkedlist_queries Queries
 * @brief Read element data and query properties
 * @{
 */

/**
 * @brief Get a pointer to an element (read-only)
 *
 * O(n) operation — traverses from the nearest end.
 *
 * @param list Linked list to query
 * @param pos  Position to access
 * @return Pointer to the element, or NULL if list is NULL or pos out of bounds
 *
 * @warning The returned pointer points directly into the list's storage.
 *          It may be invalidated by any operation that modifies the list.
 */
const void *linkedlist_at(const LinkedList *list, size_t pos);

/**
 * @brief Get a pointer to the first element (read-only)
 *
 * O(1) operation.
 *
 * @param list Linked list to query
 * @return Pointer to the first element, or NULL if list is empty or NULL
 */
const void *linkedlist_front(const LinkedList *list);

/**
 * @brief Get a pointer to the last element (read-only)
 *
 * O(1) operation.
 *
 * @param list Linked list to query
 * @return Pointer to the last element, or NULL if list is empty or NULL
 */
const void *linkedlist_back(const LinkedList *list);

/**
 * @brief Get a mutable pointer to an element
 *
 * O(n) operation — traverses from the nearest end.
 *
 * @param list Linked list to query
 * @param pos  Position to access
 * @return Mutable pointer to the element, or NULL if list is NULL or pos out of bounds
 *
 * @warning Modifying the element via this pointer does not invalidate the
 *          cached hash. Call linkedlist_hash() again if needed.
 */
void *linkedlist_at_mut(const LinkedList *list, size_t pos);

/**
 * @brief Get a mutable pointer to the first element
 *
 * O(1) operation.
 *
 * @param list Linked list to query
 * @return Mutable pointer to the first element, or NULL if list is empty or NULL
 */
void *linkedlist_front_mut(const LinkedList *list);

/**
 * @brief Get a mutable pointer to the last element
 *
 * O(1) operation.
 *
 * @param list Linked list to query
 * @return Mutable pointer to the last element, or NULL if list is empty or NULL
 */
void *linkedlist_back_mut(const LinkedList *list);

/**
 * @brief Find the first occurrence of an element
 *
 * O(n) operation.
 *
 * @param list Linked list to search
 * @param item Element to find
 * @return Index of the element, or LIST_NPOS if not found
 */
size_t linkedlist_find(const LinkedList *list, const void *item);

/**
 * @brief Find the last occurrence of an element (reverse find)
 *
 * O(n) operation.
 *
 * @param list Linked list to search
 * @param item Element to find
 * @return Index of the element, or LIST_NPOS if not found
 */
size_t linkedlist_rfind(const LinkedList *list, const void *item);

/**
 * @brief Check if an element exists in the list
 *
 * @param list Linked list to search
 * @param item Element to find
 * @return true if element exists, false otherwise
 */
bool linkedlist_contains(const LinkedList *list, const void *item);

/**
 * @brief Check if the list is empty
 *
 * @param list Linked list to query
 * @return true if list is NULL or contains no elements, false otherwise
 */
bool linkedlist_is_empty(const LinkedList *list);

/**
 * @brief Check if two linked lists are equal
 *
 * Compares length, element sizes, and each element in order using the comparator.
 *
 * @param A First list
 * @param B Second list
 * @return true if lists are equal, false otherwise
 */
bool linkedlist_equals(const LinkedList *A, const LinkedList *B);

/**
 * @brief Get the number of elements in the list
 *
 * @param list Linked list to query
 * @return Number of elements, or 0 if list is NULL
 */
size_t linkedlist_len(const LinkedList *list);

/**
 * @brief Compute a hash of the list's contents
 *
 * The hash is order-sensitive. The hash is cached and invalidated on any mutation.
 *
 * @param list Linked list to hash
 * @return Hash value, or 0 if list is NULL or empty
 */
size_t linkedlist_hash(const LinkedList *list);

/** @} */

/**
 * @defgroup linkedlist_copy Copy & View Operations
 * @brief Create new linked lists from existing ones
 * @{
 */

/**
 * @brief Create a new list with elements in reverse order
 *
 * @param list Source list
 * @return New list with reversed elements, or NULL on allocation failure
 */
LinkedList *linkedlist_reverse(const LinkedList *list);

/**
 * @brief Get the allocator used by the list
 *
 * @param list Linked list to query
 * @return Allocator used by the list, or NULL if list is NULL
 */
Allocator *linkedlist_allocator(const LinkedList *list);

/**
 * @brief Create a new empty list of the same type
 *
 * @param list Source list (determines item size, alignment, comparator, allocator)
 * @return New empty list, or NULL on allocation failure
 */
LinkedList *linkedlist_instance(const LinkedList *list);

/**
 * @brief Create a deep copy of a linked list
 *
 * For string lists, all strings are strdup'd.
 *
 * @param list Source list
 * @return Deep copy of the list, or NULL on allocation failure
 */
LinkedList *linkedlist_clone(const LinkedList *list);

/**
 * @brief Extract a sublist as a new list
 *
 * @param list Source list
 * @param from Start index (inclusive)
 * @param to   End index (exclusive)
 * @return New list containing the sublist, or NULL on invalid range or allocation failure
 */
LinkedList *linkedlist_sublist(const LinkedList *list, size_t from, size_t to);

/** @} */

/**
 * @defgroup linkedlist_conversion Conversion & Iteration
 * @brief Export to Array and create iterators
 * @{
 */

/**
 * @brief Export the list to a flat Array snapshot
 *
 * The Array header and data are in a single allocation — free with free(arr).
 *
 * @param list Linked list to export
 * @return Array snapshot, or NULL if list is NULL or empty or allocation fails
 */
Array *linkedlist_to_array(const LinkedList *list);

/**
 * @brief Create a forward iterator over the list
 *
 * @param list Linked list to iterate
 * @return Forward iterator (stack-allocated)
 */
Iterator linkedlist_iter(const LinkedList *list);

/**
 * @brief Create a reverse iterator over the list
 *
 * @param list Linked list to iterate
 * @return Reverse iterator (stack-allocated)
 */
Iterator linkedlist_iter_reversed(const LinkedList *list);

/** @} */

#endif /* LINKEDLIST_PDR_H */

/**
 * @defgroup linkedlist_implementation Implementation
 * @brief STB-style implementation section
 *
 * To include the implementation, define LINKEDLIST_IMPLEMENTATION in ONE .c file
 * before including this header:
 *
 * @code
 *   #define LINKEDLIST_IMPLEMENTATION
 *   #include "linkedlist.h"
 * @endcode
 *
 * @note The implementation is intentionally not documented in detail as it is
 *       meant to be included only once per project.
 */

#ifdef LINKEDLIST_IMPLEMENTATION
    #define ALLOCATOR_IMPLEMENTATION
    #include "allocator.h"
#ifndef LINKEDLIST_IMPLEMENTATION_ONCE
#define LINKEDLIST_IMPLEMENTATION_ONCE

/* ============================================================================
 * LinkedList — Implementation
 * ============================================================================
 *
 * Important Notes:
 *   • Doubly-linked with head and tail pointers for O(1) ends
 *   • Node pool allocator reduces malloc overhead
 *   • String mode (item_size == 0) stores char* pointers, each strdup'd
 *   • O(1) splice when allocators match (pointer relinking)
 *   • O(n) random access (walk from nearest end)
 *   • Cached hash invalidated on any mutation
 * ============================================================================ */

/* -------------------------------------------------------------------------
 * Container vtable
 * ------------------------------------------------------------------------- */
static const void *linkedlist_next(Iterator *iter);
static Container *linkedlist_instance_adpt(const Container *list);
static Container *linkedlist_clone_adpt(const Container *list);
static Array *linkedlist_to_array_adpt(const Container *list);
static size_t linkedlist_hash_adpt(const Container *list);
static bool linkedlist_insert_adpt(Container *list, const void *item);
static void linkedlist_clear_adpt(Container *list);
static void linkedlist_destroy_adpt(Container *list);

static const ContainerVTable LINKEDLIST_CONTAINER_OPS = {
    .next     = linkedlist_next,
    .instance = linkedlist_instance_adpt,
    .clone    = linkedlist_clone_adpt,
    .as_array = linkedlist_to_array_adpt,
    .hash     = linkedlist_hash_adpt,
    .insert   = linkedlist_insert_adpt,
    .clear    = linkedlist_clear_adpt,
    .destroy  = linkedlist_destroy_adpt
};

/* -------------------------------------------------------------------------
 * Internal structures
 * ------------------------------------------------------------------------- */
struct LinkedListImpl {
    uint16_t item_offset;
    uint16_t item_size;
    uint32_t stride;
};

/* Node structure */
typedef struct LinkedListEntry {
    struct LinkedListEntry *next;
    struct LinkedListEntry *prev;
    uint8_t data[];
} LinkedListEntry;

/* Head/tail pair stored in container.items */
typedef struct {
    LinkedListEntry *head;
    LinkedListEntry *tail;
} LinkedListEnds;

/* Layout calculation result */
typedef struct {
    uint16_t item_offset;
    uint32_t stride;
} LinkedListEntryLayout;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Create new node with copied item */
static LinkedListEntry *linkedlist_entry_create(LinkedList *list, const void *item) {
    LinkedListEntry *entry = (LinkedListEntry *)allocator_alloc(list->alloc);
    if (!entry) return NULL;

    LinkedListImpl *impl = (LinkedListImpl *)list->impl;
    
    void *item_slot = lc_slot_at(entry->data, impl->item_offset);
    
    if (lc_slot_init(item_slot, item, impl->item_size) != LC_OK) {
        allocator_free(list->alloc, entry);
        return NULL;
    }

    list->container.len++;

    return entry;
}

/* Free node and its item */
static void linkedlist_entry_free(LinkedList *list, LinkedListEntry *entry) {
    LinkedListImpl *impl = (LinkedListImpl *)list->impl;
    if (impl->item_size == 0) {
        void *item_slot = lc_slot_at(entry->data, impl->item_offset);
        void *ptr = *(void **)item_slot;
        if (ptr) free(ptr);
    }
    allocator_free(list->alloc, entry);
    list->container.len--;
}

/* Compute node layout */
static bool linkedlist_compute_layout(size_t item_sz, size_t item_align, LinkedListEntryLayout *out) {
    if (item_align == 0 || (item_align & (item_align - 1)) != 0) return false;

    const size_t ptr_size = sizeof(void *);

    if (item_sz == 0) {
        item_sz = ptr_size;
        item_align = ptr_size;
    }

    /* Fast path: small, byte-aligned payload */
    if (item_align == 1 && item_sz <= ptr_size) {
        out->item_offset = 0;
        out->stride = (uint32_t)(3 * ptr_size);
        return true;
    }

    if (item_align < ptr_size) item_align = ptr_size;

    /* Data starts after next and prev pointers */
    size_t data_start = 2 * ptr_size;
    size_t item_start = (data_start + item_align - 1) & ~(item_align - 1);
    size_t item_offset = item_start - data_start;
    if (item_offset > UINT16_MAX) return false;

    size_t end = item_start + item_sz;
    size_t stride = (end + item_align - 1) & ~(item_align - 1);
    if (stride > UINT32_MAX) return false;

    out->item_offset = (uint16_t)item_offset;
    out->stride = (uint32_t)stride;
    return true;
}

/* -------------------------------------------------------------------------
 * Creation helpers
 * ------------------------------------------------------------------------- */
typedef struct {
    LinkedList *list;
    void *ends;
    Allocator *alloc;
} LinkedListCreateCtx;

static void linkedlist_create_ctx_cleanup(LinkedListCreateCtx *ctx) {
    if (ctx->alloc) allocator_destroy(ctx->alloc);
    if (ctx->ends) free(ctx->ends);
    if (ctx->list) free(ctx->list);
}

static LinkedList *linkedlist_create_impl(const LinkedListBuilder *cfg, const LinkedListEntryLayout *layout) {
    LinkedListCreateCtx ctx = {0};

    ctx.list = (LinkedList *)malloc(sizeof(LinkedList) + sizeof(LinkedListImpl));
    if (!ctx.list) return NULL;

    ctx.ends = calloc(1, sizeof(LinkedListEnds));
    if (!ctx.ends) {
        linkedlist_create_ctx_cleanup(&ctx);
        return NULL;
    }

    if (cfg->alloc) {
        ctx.alloc = allocator_ref(cfg->alloc);
    } else {
        ctx.alloc = pool_allocator_create(layout->stride, LINKEDLIST_ALLOC_CAPACITY, cfg->item_align, GROW_GOLDEN);
    }

    if (!ctx.alloc) {
        linkedlist_create_ctx_cleanup(&ctx);
        return NULL;
    }
    
    LinkedListImpl *impl = (LinkedListImpl *)(ctx.list + 1);
    impl->item_size = (uint16_t)cfg->item_size;
    impl->item_offset = layout->item_offset;
    impl->stride = layout->stride;

    ctx.list->container.items = ctx.ends;
    ctx.list->container.len = 0;
    ctx.list->container.capacity = 2;  /* head + tail */
    ctx.list->container.ops = &LINKEDLIST_CONTAINER_OPS;
    ctx.list->alloc = ctx.alloc;
    ctx.list->cmp = cfg->cmp;
    ctx.list->impl = impl;

    return ctx.list;
}

/* Create new list from existing list's type */
static LinkedList *linkedlist_create_from_impl(const LinkedList *src, bool share_alloc) {
    if (!src) return NULL;

    LinkedListCreateCtx ctx = {0};

    ctx.list = (LinkedList *)malloc(sizeof(LinkedList) + sizeof(LinkedListImpl));
    if (!ctx.list) return NULL;

    ctx.ends = calloc(1, sizeof(LinkedListEnds));
    if (!ctx.ends) {
        linkedlist_create_ctx_cleanup(&ctx);
        return NULL;
    }

    const LinkedListImpl *src_impl = src->impl;
    
    if (share_alloc) {
        ctx.alloc = allocator_ref(src->alloc);
    } else {
        ctx.alloc = pool_allocator_create(src_impl->stride, LINKEDLIST_ALLOC_CAPACITY,
                                          allocator_alignment(src->alloc), GROW_GOLDEN);
    }

    if (!ctx.alloc) {
        linkedlist_create_ctx_cleanup(&ctx);
        return NULL;
    }

    LinkedListImpl *impl = (LinkedListImpl *)(ctx.list + 1);
    impl->item_size = src_impl->item_size;
    impl->item_offset = src_impl->item_offset;
    impl->stride = src_impl->stride;

    ctx.list->container.items = ctx.ends;
    ctx.list->container.len = 0;
    ctx.list->container.capacity = 2;
    ctx.list->container.ops = &LINKEDLIST_CONTAINER_OPS;
    ctx.list->alloc = ctx.alloc;
    ctx.list->cmp = src->cmp;
    ctx.list->impl = impl;

    return ctx.list;
}

/* -------------------------------------------------------------------------
 * Builder API
 * ------------------------------------------------------------------------- */
LinkedListBuilder linkedlist_builder(size_t item_size) {
    return (LinkedListBuilder){
        .item_size = item_size,
        .item_align = 1,
        .alloc = NULL,
        .cmp = NULL,
    };
}

LinkedListBuilder linkedlist_builder_str(void) {
    return (LinkedListBuilder){
        .item_size = 0,
        .item_align = 1,
        .alloc = NULL,
        .cmp = NULL,
    };
}

LinkedListBuilder linkedlist_builder_comparator(LinkedListBuilder b, lc_Comparator cmp) {
    b.cmp = cmp;
    return b;
}

LinkedListBuilder linkedlist_builder_allocator(LinkedListBuilder b, Allocator *alloc) {
    b.alloc = alloc;
    return b;
}

LinkedListBuilder linkedlist_builder_alignment(LinkedListBuilder b, size_t item_align) {
    b.item_align = item_align;
    return b;
}

LinkedList *linkedlist_builder_build(LinkedListBuilder b) {
    LinkedListEntryLayout layout;
    if (!linkedlist_compute_layout(b.item_size, b.item_align, &layout)) return NULL;
    
    return linkedlist_create_impl(&b, &layout);
}

/* -------------------------------------------------------------------------
 * Public creation functions
 * ------------------------------------------------------------------------- */
LinkedList *linkedlist_create(size_t item_size) {
    return linkedlist_builder_build(linkedlist_builder(item_size));
}

LinkedList *linkedlist_create_with_comparator(size_t item_size, lc_Comparator cmp) {
    return linkedlist_builder_build(linkedlist_builder_comparator(linkedlist_builder(item_size), cmp));
}

LinkedList *linkedlist_create_with_allocator(size_t item_size, Allocator *alloc) {
    return linkedlist_builder_build(linkedlist_builder_allocator(linkedlist_builder(item_size), alloc));
}

LinkedList *linkedlist_create_aligned(size_t item_size, size_t item_align) {
    return linkedlist_builder_build(linkedlist_builder_alignment(linkedlist_builder(item_size), item_align));
}

LinkedList *linkedlist_str(void) {
    return linkedlist_builder_build(linkedlist_builder_str());
}

LinkedList *linkedlist_str_with_comparator(lc_Comparator cmp) {
    return linkedlist_builder_build(linkedlist_builder_comparator(linkedlist_builder_str(), cmp));
}

LinkedList *linkedlist_str_with_allocator(Allocator *alloc) {
    return linkedlist_builder_build(linkedlist_builder_allocator(linkedlist_builder_str(), alloc));
}

/* -------------------------------------------------------------------------
 * Destruction & configuration
 * ------------------------------------------------------------------------- */
void linkedlist_destroy(LinkedList *list) {
    if (!list) return;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListEntry *entry = ends->head;
    while (entry) {
        LinkedListEntry *next = entry->next;
        linkedlist_entry_free(list, entry);
        entry = next;
    }

    free(list->container.items);
    allocator_destroy(list->alloc);
    free(list);
}

int linkedlist_set_comparator(LinkedList *list, lc_Comparator cmp) {
    if (!list || !cmp) return LC_EINVAL;
    if (list->container.len > 0) return LC_EBUSY;
    list->cmp = cmp;
    return LC_OK;
}

int linkedlist_set_allocator(LinkedList *list, Allocator *alloc) {
    if (!list || !alloc) return LC_EINVAL;
    if (list->container.len > 0) return LC_EBUSY;
    
    if (allocator_stride(alloc) < list->impl->stride) {
        return LC_ETYPE;
    }

    Allocator *old = list->alloc;
    list->alloc = allocator_ref(alloc);
    allocator_destroy(old);
    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Core operations
 * ------------------------------------------------------------------------- */

/* Insert at position */
static int linkedlist_insert_impl(LinkedList *list, size_t pos, const void *item) {
    if (!list) return LC_EINVAL;

    size_t old_len = list->container.len;
    if (pos > old_len) return LC_EBOUNDS;

    LinkedListEntry *entry = linkedlist_entry_create(list, item);
    if (!entry) return LC_ENOMEM;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;

    if (old_len == 0) {
        entry->next = NULL;
        entry->prev = NULL;
        ends->head = entry;
        ends->tail = entry;
        return LC_OK;
    }

    if (pos == 0) {
        entry->prev = NULL;
        entry->next = ends->head;
        ends->head->prev = entry;
        ends->head = entry;
        return LC_OK;
    }

    if (pos == old_len) {
        entry->next = NULL;
        entry->prev = ends->tail;
        ends->tail->next = entry;
        ends->tail = entry;
        return LC_OK;
    }

    /* Insert in middle - walk from nearest end */
    LinkedListEntry *runner;
    if (pos <= old_len / 2) {
        runner = ends->head;
        for (size_t i = 0; i < pos; i++) runner = runner->next;
    } else {
        runner = ends->tail;
        for (size_t i = old_len - 1; i > pos; i--) runner = runner->prev;
    }

    entry->prev = runner->prev;
    entry->next = runner;
    runner->prev->next = entry;
    runner->prev = entry;
    return LC_OK;
}

/* Append range from src to dst at position */
static int linkedlist_append_impl(LinkedList *dst, size_t pos, const LinkedList *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    if (dst->impl->item_size != src->impl->item_size) return LC_ETYPE;
    if (pos > dst->container.len) return LC_EBOUNDS;
    if (from > to || to > src->container.len) return LC_EBOUNDS;
    if (from == to) return LC_OK;

    const LinkedListImpl *src_impl = src->impl;
    const size_t item_sz = src_impl->item_size;
    const size_t item_off = src_impl->item_offset;

    LinkedListEnds *src_ends = (LinkedListEnds *)src->container.items;

    /* Walk to first src node */
    LinkedListEntry *src_runner = src_ends->head;
    for (size_t i = 0; i < from; i++) src_runner = src_runner->next;

    /* Snapshot stop node */
    LinkedListEntry *src_stop = src_runner;
    for (size_t i = from; i < to; i++) src_stop = src_stop->next;

    /* Create new nodes */
    LinkedListEntry *first_new = NULL, *last_new = NULL;
    for (LinkedListEntry *r = src_runner; r != src_stop; r = r->next) {
        void *item = lc_slot_get(lc_slot_at(r->data, item_off), item_sz);
        LinkedListEntry *new_node = linkedlist_entry_create(dst, item);
        if (!new_node) {
            while (first_new) {
                LinkedListEntry *next = first_new->next;
                linkedlist_entry_free(dst, first_new);
                first_new = next;
            }
            return LC_ENOMEM;
        }

        new_node->prev = last_new;
        new_node->next = NULL;
        if (last_new) last_new->next = new_node;
        else first_new = new_node;
        last_new = new_node;
    }

    /* Find insertion point */
    LinkedListEnds *dst_ends = (LinkedListEnds *)dst->container.items;
    LinkedListEntry *dst_runner = dst_ends->head;
    for (size_t i = 0; i < pos && i < dst->container.len; i++) dst_runner = dst_runner->next;
    if (pos >= dst->container.len) dst_runner = NULL;

    /* Splice chain into dst */
    LinkedListEntry *prev_node = dst_runner ? dst_runner->prev : dst_ends->tail;

    first_new->prev = prev_node;
    if (prev_node) prev_node->next = first_new;
    else dst_ends->head = first_new;

    last_new->next = dst_runner;
    if (dst_runner) dst_runner->prev = last_new;
    else dst_ends->tail = last_new;

    return LC_OK;
}

int linkedlist_push_back(LinkedList *list, const void *item) {
    if (!list || !item) return LC_EINVAL;
    return linkedlist_insert_impl(list, list->container.len, item);
}

int linkedlist_push_front(LinkedList *list, const void *item) {
    if (!list || !item) return LC_EINVAL;
    return linkedlist_insert_impl(list, 0, item);
}

int linkedlist_insert(LinkedList *list, size_t pos, const void *item) {
    if (!list || !item) return LC_EINVAL;
    return linkedlist_insert_impl(list, pos, item);
}

int linkedlist_insert_range(LinkedList *dst, size_t pos, const LinkedList *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    return linkedlist_append_impl(dst, pos, src, from, to);
}

int linkedlist_append(LinkedList *dst, const LinkedList *src) {
    if (!dst || !src) return LC_EINVAL;
    return linkedlist_append_impl(dst, dst->container.len, src, 0, src->container.len);
}

int linkedlist_append_range(LinkedList *dst, const LinkedList *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    return linkedlist_append_impl(dst, dst->container.len, src, from, to);
}

int linkedlist_set(LinkedList *list, size_t pos, const void *item) {
    if (!list || !item) return LC_EINVAL;
    if (pos >= list->container.len) return LC_EBOUNDS;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    const size_t len = list->container.len;
    LinkedListEntry *runner;

    /* Walk from nearest end */
    if (pos <= len / 2) {
        runner = ends->head;
        for (size_t i = 0; i < pos; i++) runner = runner->next;
    } else {
        runner = ends->tail;
        for (size_t i = len - 1; i > pos; i--) runner = runner->prev;
    }

    void *item_slot = lc_slot_at(runner->data, list->impl->item_offset);
    int rc = lc_slot_set(item_slot, item, list->impl->item_size);
    
    return rc;
}

/* Remove node from list without freeing */
static void linkedlist_unlink_node(LinkedList *list, LinkedListEntry *node) {
    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;

    if (node->prev) node->prev->next = node->next;
    else ends->head = node->next;

    if (node->next) node->next->prev = node->prev;
    else ends->tail = node->prev;
}

/* Remove range [from, to) */
static int linkedlist_remove_impl(LinkedList *list, size_t from, size_t to) {
    if (!list) return LC_EINVAL;
    if (from >= to || to > list->container.len) return LC_EBOUNDS;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListEntry *runner = ends->head;
    for (size_t i = 0; i < from; i++) runner = runner->next;

    for (size_t i = from; i < to; i++) {
        LinkedListEntry *next = runner->next;
        linkedlist_unlink_node(list, runner);
        linkedlist_entry_free(list, runner);
        runner = next;
    }

    return LC_OK;
}

int linkedlist_pop_back(LinkedList *list) {
    if (!list) return LC_EINVAL;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    if (!ends->tail) return LC_EBOUNDS;

    LinkedListEntry *node = ends->tail;
    linkedlist_unlink_node(list, node);
    linkedlist_entry_free(list, node);

    return LC_OK;
}

int linkedlist_pop_front(LinkedList *list) {
    if (!list) return LC_EINVAL;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    if (!ends->head) return LC_EBOUNDS;

    LinkedListEntry *node = ends->head;
    linkedlist_unlink_node(list, node);
    linkedlist_entry_free(list, node);

    return LC_OK;
}

int linkedlist_remove(LinkedList *list, size_t pos) {
    if (!list) return LC_EINVAL;
    return linkedlist_remove_impl(list, pos, pos + 1);
}

int linkedlist_remove_range(LinkedList *list, size_t from, size_t to) {
    if (!list) return LC_EINVAL;
    return linkedlist_remove_impl(list, from, to);
}

int linkedlist_clear(LinkedList *list) {
    if (!list) return LC_EINVAL;
    if (list->container.len == 0) return LC_OK;
    return linkedlist_remove_impl(list, 0, list->container.len);
}

int linkedlist_reverse_inplace(LinkedList *list) {
    if (!list || list->container.len < 2) return LC_OK;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListEntry *curr = ends->head;

    while (curr) {
        LinkedListEntry *next = curr->next;
        curr->next = curr->prev;
        curr->prev = next;
        curr = next;
    }

    LinkedListEntry *tmp_head = ends->head;
    ends->head = ends->tail;
    ends->tail = tmp_head;

    return LC_OK;
}

/* Extract chain [from, to) from src */
static void linkedlist_extract_chain(LinkedList *src, size_t from, size_t to,
                                      LinkedListEntry **out_first, LinkedListEntry **out_last) {
    LinkedListEnds *ends = (LinkedListEnds *)src->container.items;

    LinkedListEntry *first = ends->head;
    for (size_t i = 0; i < from; i++) first = first->next;

    LinkedListEntry *last = first;
    for (size_t i = from + 1; i < to; i++) last = last->next;

    LinkedListEntry *before = first->prev;
    LinkedListEntry *after = last->next;

    if (before) before->next = after;
    else ends->head = after;

    if (after) after->prev = before;
    else ends->tail = before;

    first->prev = NULL;
    last->next = NULL;

    src->container.len -= (to - from);

    *out_first = first;
    *out_last = last;
}

/* Splice chain into dst at position */
static void linkedlist_splice_chain(LinkedList *dst, size_t pos,
                                     LinkedListEntry *first, LinkedListEntry *last, size_t count) {
    LinkedListEnds *ends = (LinkedListEnds *)dst->container.items;

    LinkedListEntry *dst_runner = ends->head;
    for (size_t i = 0; i < pos; i++) dst_runner = dst_runner->next;

    LinkedListEntry *prev_node = dst_runner ? dst_runner->prev : ends->tail;

    first->prev = prev_node;
    if (prev_node) prev_node->next = first;
    else ends->head = first;

    last->next = dst_runner;
    if (dst_runner) dst_runner->prev = last;
    else ends->tail = last;

    dst->container.len += count;
}

int linkedlist_splice(LinkedList *dst, size_t pos, LinkedList *src, size_t from, size_t to) {
    if (!dst || !src) return LC_EINVAL;
    if (dst == src) return LC_EINVAL;
    if (dst->impl->item_size != src->impl->item_size) return LC_ETYPE;
    if (pos > dst->container.len) return LC_EBOUNDS;
    if (from > to || to > src->container.len) return LC_EBOUNDS;
    if (from == to) return LC_OK;

    /* Same allocator: O(1) relink */
    if (dst->alloc == src->alloc) {
        LinkedListEntry *first, *last;
        linkedlist_extract_chain(src, from, to, &first, &last);
        linkedlist_splice_chain(dst, pos, first, last, to - from);
        return LC_OK;
    }

    /* Different allocators: copy */
    return linkedlist_append_impl(dst, pos, src, from, to);
}

int linkedlist_unique(LinkedList *list) {
    if (!list || list->container.len < 2) return LC_OK;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListImpl *impl = (LinkedListImpl *)list->impl;
    
    const size_t item_sz = impl->item_size;
    const size_t item_off = impl->item_offset;
    const lc_Comparator cmp = list->cmp;

    LinkedListEntry *curr = ends->head;
    
    while (curr && curr->next) {
        void *val_curr = lc_slot_get(lc_slot_at(curr->data, item_off), item_sz);
        void *val_next = lc_slot_get(lc_slot_at(curr->next->data, item_off), item_sz);

        if (lc_slot_cmp(val_curr, val_next, item_sz, cmp) == 0) {
            LinkedListEntry *duplicate = curr->next;
            linkedlist_unlink_node(list, duplicate);
            linkedlist_entry_free(list, duplicate);
        } else {
            curr = curr->next;
        }
    }

    return LC_OK;
}

/* Merge two sorted chains */
static void linkedlist_merge_chains(LinkedListEntry *a, LinkedListEntry *b,
                                    size_t item_off, lc_Comparator cmp,
                                    LinkedListEntry **out_head, LinkedListEntry **out_tail) {
    LinkedListEntry dummy = {0};
    LinkedListEntry *curr = &dummy;

    while (a && b) {
        void *val_a = lc_slot_at(a->data, item_off);
        void *val_b = lc_slot_at(b->data, item_off);

        if (cmp(val_a, val_b) <= 0) {
            curr->next = a;
            a->prev = curr;
            a = a->next;
        } else {
            curr->next = b;
            b->prev = curr;
            b = b->next;
        }
        curr = curr->next;
    }

    curr->next = a ? a : b;
    if (curr->next) curr->next->prev = curr;

    while (curr->next) curr = curr->next;

    *out_head = dummy.next;
    if (*out_head) (*out_head)->prev = NULL;
    *out_tail = curr;
}

int linkedlist_sort(LinkedList *list, lc_Comparator cmp) {
    if (!list) return LC_EINVAL;
    if (list->container.len < 2) return LC_OK;

    if (!cmp) return LC_EINVAL;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListImpl *impl = (LinkedListImpl *)list->impl;
    
    size_t item_off = impl->item_offset;
    size_t len = list->container.len;

    LinkedListEntry *head = ends->head;
    LinkedListEntry *curr, *left, *right, *next_pair, *tail_prev;

    /* Bottom-up merge sort */
    for (size_t step = 1; step < len; step <<= 1) {
        curr = head;
        tail_prev = NULL;
        head = NULL;

        while (curr) {
            left = curr;
            
            /* Find end of left side */
            LinkedListEntry *temp = left;
            for (size_t i = 1; i < step && temp->next; i++) temp = temp->next;
            
            right = temp->next;
            if (right) right->prev = NULL;
            temp->next = NULL;

            /* Find end of right side */
            temp = right;
            for (size_t i = 1; i < step && temp && temp->next; i++) temp = temp->next;
            
            next_pair = temp ? temp->next : NULL;
            if (temp) temp->next = NULL;
            if (next_pair) next_pair->prev = NULL;

            /* Merge */
            LinkedListEntry *merged_h, *merged_t;
            linkedlist_merge_chains(left, right, item_off, cmp, &merged_h, &merged_t);

            /* Connect merged segment */
            if (!head) head = merged_h;
            if (tail_prev) {
                tail_prev->next = merged_h;
                merged_h->prev = tail_prev;
            }
            
            tail_prev = merged_t;
            curr = next_pair;
        }
    }

    ends->head = head;
    ends->tail = tail_prev;
    
    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Queries & utilities
 * ------------------------------------------------------------------------- */

static void *linkedlist_at_impl(const LinkedList *list, size_t pos) {
    if (pos >= list->container.len) return NULL;
    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    const size_t len = list->container.len;
    LinkedListEntry *entry = NULL;

    if (pos <= len / 2) {
        entry = ends->head;
        for (size_t i = 0; i < pos; i++) entry = entry->next;
    } else {
        entry = ends->tail;
        for (size_t i = len - 1; i > pos; i--) entry = entry->prev;
    }
    
    return lc_slot_at(entry->data, list->impl->item_offset);
}

static void *linkedlist_front_impl(const LinkedList *list) {
    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    if (!ends->head) return NULL;
    return lc_slot_at(ends->head->data, list->impl->item_offset);
}

static void *linkedlist_back_impl(const LinkedList *list) {
    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    if (!ends->tail) return NULL;
    return lc_slot_at(ends->tail->data, list->impl->item_offset);
}

const void *linkedlist_at(const LinkedList *list, size_t pos) {
    if (!list) return NULL;
    void *slot = linkedlist_at_impl(list, pos);
    return slot ? lc_slot_get(slot, list->impl->item_size) : NULL;
}

const void *linkedlist_front(const LinkedList *list) {
    if (!list) return NULL;
    void *slot = linkedlist_front_impl(list);
    return slot ? lc_slot_get(slot, list->impl->item_size) : NULL;
}

const void *linkedlist_back(const LinkedList *list) {
    if (!list) return NULL;
    void *slot = linkedlist_back_impl(list);
    return slot ? lc_slot_get(slot, list->impl->item_size) : NULL;
}

void *linkedlist_at_mut(const LinkedList *list, size_t pos) {
    if (!list) return NULL;
    return linkedlist_at_impl(list, pos);
}

void *linkedlist_front_mut(const LinkedList *list) {
    if (!list) return NULL;
    return linkedlist_front_impl(list);
}

void *linkedlist_back_mut(const LinkedList *list) {
    if (!list) return NULL;
    return linkedlist_back_impl(list);
}

size_t linkedlist_find(const LinkedList *list, const void *item) {
    if (!list || !item) return LIST_NPOS;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    if (!ends->head) return LIST_NPOS;
    
    const LinkedListImpl *impl = list->impl;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Comparator cmp = list->cmp;
    
    LinkedListEntry *runner = ends->head;
    size_t pos = 0;
    while (runner) {
        void *node_item = lc_slot_get(lc_slot_at(runner->data, item_off), item_sz);
        if (lc_slot_cmp(node_item, item, item_sz, cmp) == 0) {
            return pos;
        }
        runner = runner->next;
        pos++;
    }

    return LIST_NPOS;
}

size_t linkedlist_rfind(const LinkedList *list, const void *item) {
    if (!list || !item) return LIST_NPOS;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    if (!ends->tail) return LIST_NPOS;
    
    const LinkedListImpl *impl = list->impl;
    const size_t item_off = impl->item_offset;
    const size_t item_sz = impl->item_size;
    const lc_Comparator cmp = list->cmp;
    
    LinkedListEntry *runner = ends->tail;
    size_t pos = list->container.len - 1;
    while (runner) {
        void *node_item = lc_slot_get(lc_slot_at(runner->data, item_off), item_sz);
        if (lc_slot_cmp(node_item, item, item_sz, cmp) == 0) {
            return pos;
        }
        runner = runner->prev;
        pos--;
    }

    return LIST_NPOS;
}

bool linkedlist_contains(const LinkedList *list, const void *item) {
    return linkedlist_find(list, item) != LIST_NPOS;
}

bool linkedlist_is_empty(const LinkedList *list) {
    return !list || list->container.len == 0;
}

size_t linkedlist_len(const LinkedList *list) {
    return list ? list->container.len : 0;
}

size_t linkedlist_size(const LinkedList *list) {
    return linkedlist_len(list);
}

size_t linkedlist_hash(const LinkedList *list) {
    if (!list || list->container.len == 0) return 0;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    const size_t item_off = list->impl->item_offset;
    const size_t item_sz = list->impl->item_size;

    size_t h = list->container.len;
    for (LinkedListEntry *entry = ends->head; entry; entry = entry->next) {
        void *item = lc_slot_get(lc_slot_at(entry->data, item_off), item_sz);
        size_t x = lc_slot_hash(item, item_sz, NULL);
        h ^= x + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    
    return lc_hash_mix(h);
}

bool linkedlist_equals(const LinkedList *A, const LinkedList *B) {
    if (!A || !B) return false;
    if (A == B) return true;
    if (A->container.len != B->container.len) return false;

    if (A->impl->item_size != B->impl->item_size) return false;

    const size_t item_off = A->impl->item_offset;
    const size_t item_sz = A->impl->item_size;

    LinkedListEnds *ends_a = (LinkedListEnds *)A->container.items;
    LinkedListEnds *ends_b = (LinkedListEnds *)B->container.items;
    LinkedListEntry *ra = ends_a->head, *rb = ends_b->head;
    
    while (ra && rb) {
        void *a_item = lc_slot_get(lc_slot_at(ra->data, item_off), item_sz);
        void *b_item = lc_slot_get(lc_slot_at(rb->data, item_off), item_sz);
        if (lc_slot_cmp(a_item, b_item, item_sz, A->cmp) != 0) return false;
        ra = ra->next;
        rb = rb->next;
    }

    return true;
}

Allocator *linkedlist_allocator(const LinkedList *list) {
    return list ? list->alloc : NULL;
}

/* -------------------------------------------------------------------------
 * Copy & view operations
 * ------------------------------------------------------------------------- */
LinkedList *linkedlist_clone(const LinkedList *list) {
    if (!list) return NULL;

    LinkedList *clone = linkedlist_create_from_impl(list, false);
    if (!clone) return NULL;
    
    const size_t item_sz = list->impl->item_size;
    const size_t item_off = list->impl->item_offset;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;

    for (LinkedListEntry *runner = ends->head; runner; runner = runner->next) {
        void *item = lc_slot_get(lc_slot_at(runner->data, item_off), item_sz);
        int rc = linkedlist_insert_impl(clone, clone->container.len, item);
        if (rc != LC_OK) {
            linkedlist_destroy(clone);
            return NULL;
        }
    }

    return clone;
}

LinkedList *linkedlist_instance(const LinkedList *list) {
    if (!list) return NULL;
    return linkedlist_create_from_impl(list, false);
}

LinkedList *linkedlist_reverse(const LinkedList *list) {
    if (!list) return NULL;

    LinkedList *rev = linkedlist_create_from_impl(list, false);
    if (!rev) return NULL;

    const size_t item_sz = list->impl->item_size;
    const size_t item_off = list->impl->item_offset;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;

    for (LinkedListEntry *runner = ends->tail; runner; runner = runner->prev) {
        void *item = lc_slot_get(lc_slot_at(runner->data, item_off), item_sz);
        int rc = linkedlist_insert_impl(rev, rev->container.len, item);
        if (rc != LC_OK) {
            linkedlist_destroy(rev);
            return NULL;
        }
    }

    return rev;
}

LinkedList *linkedlist_sublist(const LinkedList *list, size_t from, size_t to) {
    if (!list) return NULL;
    if (from > to || to > list->container.len) return NULL;
    
    LinkedList *slice = linkedlist_create_from_impl(list, false);
    if (!slice) return NULL;

    const size_t item_sz = list->impl->item_size;
    const size_t item_off = list->impl->item_offset;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListEntry *runner = ends->head;
    for (size_t i = 0; i < from; i++) runner = runner->next;

    for (size_t i = from; i < to; i++) {
        void *item = lc_slot_get(lc_slot_at(runner->data, item_off), item_sz);
        int rc = linkedlist_insert_impl(slice, slice->container.len, item);
        if (rc != LC_OK) {
            linkedlist_destroy(slice);
            return NULL;
        }
        runner = runner->next;
    }

    return slice;
}

/* -------------------------------------------------------------------------
 * Array conversion
 * ------------------------------------------------------------------------- */
static Array *linkedlist_collect_fixed(const LinkedList *list) {
    size_t list_len = list->container.len;
    if (list_len == 0) return NULL;

    const size_t item_size = list->impl->item_size;
    const size_t offset = list->impl->item_offset;
    const size_t base_align = 1; /* No alignment needed for array items? */

    size_t align = base_align > 16 ? base_align : 16;
    size_t header_size = sizeof(Array);

    if (header_size > SIZE_MAX - align) return NULL;
    size_t items_offset = (header_size + align - 1) & ~(align - 1);
    
    size_t total_items = list_len * item_size;
    if (items_offset > SIZE_MAX - total_items) return NULL;
    size_t total = items_offset + total_items;

    Array *arr = (Array *)malloc(total);
    if (!arr) return NULL;

    arr->items = (uint8_t *)arr + items_offset;
    arr->stride = item_size;
    arr->len = list_len;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    LinkedListEntry *runner = ends->head;
    size_t pos = 0;
    while (runner) {
        void *item = lc_slot_at(runner->data, offset);
        memcpy((uint8_t *)arr->items + (pos++ * item_size), item, item_size);
        runner = runner->next;
    }

    return arr;
}

static Array *linkedlist_collect_strings(const LinkedList *list) {
    size_t list_len = list->container.len;
    if (list_len == 0) return NULL;

    if (list_len > SIZE_MAX / sizeof(char *)) return NULL;

    LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
    const size_t item_off = list->impl->item_offset;
    
    size_t total_chars = 0;
    for (LinkedListEntry *entry = ends->head; entry; entry = entry->next) {
        void *item = lc_slot_at(entry->data, item_off);
        const char *str = *(const char **)item;
        total_chars += strlen(str) + 1;
    }
   
    size_t ptrs_bytes = list_len * sizeof(char *);
    if (total_chars > SIZE_MAX - sizeof(Array) - ptrs_bytes) return NULL;
    
    Array *arr = (Array *)malloc(sizeof(Array) + ptrs_bytes + total_chars);
    if (!arr) return NULL;

    arr->items = (uint8_t *)(arr + 1);
    arr->stride = 0;
    arr->len = list_len;

    char **table = (char **)arr->items;
    char *pool = (char *)(arr->items + ptrs_bytes);
    
    size_t pos = 0;
    for (LinkedListEntry *entry = ends->head; entry; entry = entry->next) {
        void *item = lc_slot_at(entry->data, item_off);
        const char *str = *(const char **)item;
        size_t len = strlen(str) + 1;
        table[pos++] = pool;
        memcpy(pool, str, len);
        pool += len;
    }

    return arr;
}

Array *linkedlist_to_array(const LinkedList *list) {
    if (!list) return NULL;

    if (list->impl->item_size == 0) {
        return linkedlist_collect_strings(list);
    }
    return linkedlist_collect_fixed(list);
}

/* -------------------------------------------------------------------------
 * Iterator
 * ------------------------------------------------------------------------- */
static const void *linkedlist_next(Iterator *iter) {
    if (!iter || !iter->container) return NULL;
    LinkedList *list = (LinkedList *)iter->container;
    LinkedListEntry *entry = (LinkedListEntry *)iter->state;

    if (iter->direction == ITER_FORWARD) {
        if (iter->pos >= list->container.len) return NULL;
        if (!iter->state) {
            LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
            iter->state = ends->head;
        } else {
            iter->state = entry->next;
        }
        iter->pos++;
    } else {
        if (iter->pos == 0) return NULL;
        if (!iter->state) {
            LinkedListEnds *ends = (LinkedListEnds *)list->container.items;
            iter->state = ends->tail;
        } else {
            iter->state = ((LinkedListEntry *)iter->state)->prev;
        }
        iter->pos--;
    }
    entry = (LinkedListEntry *)iter->state;
    return entry ? lc_slot_get(lc_slot_at(entry->data, list->impl->item_offset), list->impl->item_size) : NULL;
}

Iterator linkedlist_iter(const LinkedList *list) {
    return Iter((const Container *)list);
}

Iterator linkedlist_iter_reversed(const LinkedList *list) {
    return IterReverse((const Container *)list);
}

/* -------------------------------------------------------------------------
 * Container adapter functions
 * ------------------------------------------------------------------------- */
static Container *linkedlist_instance_adpt(const Container *list) {
    return (Container *)linkedlist_instance((const LinkedList *)list);
}

static Container *linkedlist_clone_adpt(const Container *list) {
    return (Container *)linkedlist_clone((const LinkedList *)list);
}

static Array *linkedlist_to_array_adpt(const Container *list) {
    return linkedlist_to_array((const LinkedList *)list);
}

static size_t linkedlist_hash_adpt(const Container *list) {
    return linkedlist_hash((const LinkedList *)list);
}

static bool linkedlist_insert_adpt(Container *list, const void *item) {
    return linkedlist_push_back((LinkedList *)list, item) == LC_OK;
}

static void linkedlist_clear_adpt(Container *list) {
    linkedlist_clear((LinkedList *)list);
}

static void linkedlist_destroy_adpt(Container *list) {
    linkedlist_destroy((LinkedList *)list);
}

#endif /* LINKEDLIST_IMPLEMENTATION_ONCE */
#endif /* LINKEDLIST_IMPLEMENTATION */