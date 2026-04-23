/** @file linkedlist.h
 * @brief Type-safe wrappers for LinkedList container
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * This header provides macro-based type-safe wrappers around the generic
 * LinkedList API. Uses zero-cost overlay design where the typed struct shares
 * the same memory layout as LinkedList, enabling direct casting.
 *
 * @section example Usage Example
 * @code
 *   // Declare types at global scope
 *   DECL_LINKEDLIST_TYPE(int, sizeof(int), IntList)
 *   DECL_LINKEDLIST_TYPE(const char*, 0, StringList)
 *
 *   int main() {
 *       // Create typed list directly (cast from generic)
 *       IntList *list1 = (IntList*)linkedlist_create(sizeof(int));
 *       IntList_push_back(list1, 42);
 *       IntList_push_front(list1, 10);
 *       int val = IntList_at(list1, 0);  // 10
 *
 *       // Or use convenience wrappers
 *       StringList *list2 = StringList_create();
 *       StringList_push_back(list2, "hello");
 *       const char *s = StringList_at(list2, 0);  // "hello"
 *
 *       // Cast to generic when needed (zero-cost)
 *       LinkedList *raw = (LinkedList*)list1;
 *       size_t len = linkedlist_len(raw);
 *
 *       IntList_destroy(list1);
 *       StringList_destroy(list2);
 *       return 0;
 *   }
 * @endcode
 *
 * @warning The macro generates static inline functions. Include this header
 *          in exactly the same way as linkedlist.h — no special implementation
 *          define is needed.
 */

#ifndef CONTAIN_TYPED_LINKEDLIST_PDR_H
#define CONTAIN_TYPED_LINKEDLIST_PDR_H

#include <stdlib.h>
#include <contain/linkedlist.h>

/* Internal debug macros */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_LIST_DEBUG_NULL(n, func) \
    if (!(n)) { \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer\n", func); \
        abort(); \
    }
#define LC_LIST_DEBUG_BOUNDS(list, idx, func) \
    if ((idx) >= linkedlist_len((LinkedList*)list)) { \
        fprintf(stderr, "libcontain panic: %s(%zu) - index %zu >= length %zu\n", \
                func, idx, idx, linkedlist_len((LinkedList*)list)); \
        abort(); \
    }
#define LC_LIST_DEBUG_EMPTY(list, func) \
    if (linkedlist_is_empty((LinkedList*)list)) { \
        fprintf(stderr, "libcontain panic: %s() - called on empty container\n", func); \
        abort(); \
    }
#else
#define LC_LIST_DEBUG_NULL(n, func) ((void)0)
#define LC_LIST_DEBUG_BOUNDS(list, idx, func) ((void)0)
#define LC_LIST_DEBUG_EMPTY(list, func) ((void)0)
#endif

/**
 * @def DECL_LINKEDLIST_TYPE
 * @brief Generate a type-safe linked list wrapper for type T
 *
 * Creates a new type `name` that shares memory layout with LinkedList,
 * enabling zero-cost casting between typed and generic pointers.
 *
 * @param T    Element type (e.g., int, const char*, MyStruct)
 * @param size Size of T in bytes (0 for string mode)
 * @param name Name for the generated type (e.g., IntList, StringList)
 *
 * @par Design Note
 * The typed struct contains a single LinkedList pointer as its first member,
 * making it binary compatible with LinkedList*. This allows direct casting
 * without runtime overhead.
 *
 * @par Generated Functions
 *
 * **Creation & Destruction**
 *   - `name *name##_create(void)`
 *   - `name *name##_create_with_comparator(lc_Comparator cmp)`
 *   - `name *name##_create_aligned(size_t align)`
 *   - `void name##_destroy(name *n)`
 *
 * **Container Access**
 *   - `LinkedList *name##_unwrap(name *n)` — Cast to generic (zero-cost)
 *   - `const LinkedList *name##_unwrap_const(const name *n)` — Const cast
 *   - `name *name##_wrap(Container *c)` — Cast from generic (zero-cost)
 *
 * **Instance**
 *   - `name *name##_instance(const name *n)`
 *
 * **Insertion**
 *   - `int name##_push_front(name *n, T val)`
 *   - `int name##_push_back(name *n, T val)`
 *   - `int name##_insert(name *n, size_t pos, T val)`
 *   - `int name##_insert_range(name *dst, size_t pos, const name *src, size_t from, size_t to)`
 *   - `int name##_append(name *dst, const name *src)`
 *
 * **Access & Modification**
 *   - `int name##_set(name *n, size_t pos, T val)`
 *   - `T name##_at(const name *n, size_t pos)`
 *   - `T name##_at_or_default(const name *n, size_t pos, T default_val)`
 *   - `T name##_front(const name *n)`
 *   - `T name##_back(const name *n)`
 *   - `T* name##_get_ptr(name *n, size_t pos)`
 *
 * **Removal**
 *   - `int name##_pop_front(name *n)`
 *   - `int name##_pop_back(name *n)`
 *   - `int name##_remove(name *n, size_t pos)`
 *   - `void name##_clear(name *n)`
 *
 * **Queries**
 *   - `size_t name##_len(const name *n)`
 *   - `bool name##_is_empty(const name *n)`
 *   - `size_t name##_find(const name *n, T val)`
 *   - `size_t name##_rfind(const name *n, T val)`
 *   - `bool name##_contains(const name *n, T val)`
 *
 * **Configuration**
 *   - `int name##_set_comparator(name *n, lc_Comparator cmp)`
 *
 * **In-place Operations**
 *   - `void name##_reverse_inplace(name *n)`
 *   - `int name##_sort(name *n, lc_Comparator cmp)`
 *
 * **Copy & View**
 *   - `name *name##_clone(const name *n)`
 *   - `name *name##_reverse(const name *n)`
 *   - `name *name##_sublist(const name *n, size_t from, size_t to)`
 *
 * **Iteration**
 *   - `Iterator name##_iter(const name *n)`
 *   - `Iterator name##_iter_reversed(const name *n)`
 *
 * @warning T must be copyable by memcpy. For structs with pointers,
 *          use string mode (size=0) or implement manual deep copy.
 *
 * @warning The generated functions return raw values (by copy). For large
 *          structs, consider using the generic API with pointers.
 *
 * @note Panics (abort) in debug mode when preconditions are violated.
 *       Define CONTAINER_DEBUG to enable runtime checks.
 */
#define DECL_LINKEDLIST_TYPE(T, size, name) \
    /* Compile-time size validation */ \
    LC_STATIC_ASSERT((size) == 0 || (size) == sizeof(T), \
        "libcontain: size must be 0 (string mode with T=const const char*) or sizeof(T) for fixed-size"); \
    \
    /* Zero-cost overlay: 'name' has same layout as LinkedList */ \
    typedef struct name { \
        Container base; \
    } name; \
    \
    /* ===== Creation & Destruction ===== */ \
    \
    /** @brief Create a new empty typed linked list */ \
    static inline name* name##_create(void) { \
        return (name*)linkedlist_create(size); \
    } \
    \
    /** @brief Create a new typed linked list with a custom comparator */ \
    static inline name* name##_create_with_comparator(lc_Comparator cmp) { \
        return (name*)linkedlist_create_with_comparator(size, cmp); \
    } \
    \
    /** @brief Create a new typed linked list with aligned elements */ \
    static inline name* name##_create_aligned(size_t align) { \
        return (name*)linkedlist_create_aligned(size, align); \
    } \
    \
    /** @brief Destroy a typed linked list and free all resources */ \
    static inline void name##_destroy(name *n) { \
        linkedlist_destroy((LinkedList*)n); \
    } \
    \
    /* ===== Container Access ===== */ \
    \
    /** @brief Get the underlying generic LinkedList pointer (zero-cost cast) */ \
    static inline LinkedList *name##_unwrap(name *n) { \
        return (LinkedList*)n; \
    } \
    \
    /** @brief Get the underlying generic LinkedList pointer (const, zero-cost cast) */ \
    static inline const LinkedList *name##_unwrap_const(const name *n) { \
        return (const LinkedList*)n; \
    } \
    \
    /** \
     * @brief Wrap generic container (zero-cost cast, takes ownership) \
     * \
     * After calling wrap, do NOT destroy the original container. \
     * Use name##_destroy() to free both. \
     */ \
    static inline name* name##_wrap(Container *c) { \
        LC_LIST_DEBUG_NULL(c, #name "_wrap"); \
        return (name*)c; \
    } \
    \
    /** @brief Create a new empty linked list of the same type */ \
    static inline name* name##_instance(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_instance"); \
        return (name*)linkedlist_instance((LinkedList*)n); \
    } \
    \
    /* ===== Queries ===== */ \
    \
    /** @brief Get the number of elements in the list */ \
    static inline size_t name##_len(const name *n) { \
        return n ? linkedlist_len((LinkedList*)n) : 0; \
    } \
    \
    /** @brief Check if the list is empty */ \
    static inline bool name##_is_empty(const name *n) { \
        return n ? linkedlist_is_empty((LinkedList*)n) : true; \
    } \
    \
    /** @brief Find the first occurrence of an element (O(n)) */ \
    static inline size_t name##_find(const name *n, T val) { \
        LC_LIST_DEBUG_NULL(n, #name "_find"); \
        return linkedlist_find((LinkedList*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Find the last occurrence of an element (O(n)) */ \
    static inline size_t name##_rfind(const name *n, T val) { \
        LC_LIST_DEBUG_NULL(n, #name "_rfind"); \
        return linkedlist_rfind((LinkedList*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Check if an element exists in the list (O(n)) */ \
    static inline bool name##_contains(const name *n, T val) { \
        return name##_find(n, val) != LIST_NPOS; \
    } \
    \
    /* ===== Insertion ===== */ \
    \
    /** @brief Prepend an element to the front of the list (O(1)) */ \
    static inline int name##_push_front(name *n, T val) { \
        LC_LIST_DEBUG_NULL(n, #name "_push_front"); \
        return linkedlist_push_front((LinkedList*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Append an element to the back of the list (O(1)) */ \
    static inline int name##_push_back(name *n, T val) { \
        LC_LIST_DEBUG_NULL(n, #name "_push_back"); \
        return linkedlist_push_back((LinkedList*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Insert an element at the specified position (O(n)) */ \
    static inline int name##_insert(name *n, size_t pos, T val) { \
        LC_LIST_DEBUG_NULL(n, #name "_insert"); \
        return linkedlist_insert((LinkedList*)n, pos, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Insert a range of elements from another linked list */ \
    static inline int name##_insert_range(name *dst, size_t pos, const name *src, size_t from, size_t to) { \
        LC_LIST_DEBUG_NULL(dst, #name "_insert_range"); \
        LC_LIST_DEBUG_NULL(src, #name "_insert_range"); \
        return linkedlist_insert_range((LinkedList*)dst, pos, (LinkedList*)src, from, to); \
    } \
    \
    /** @brief Append all elements from another linked list */ \
    static inline int name##_append(name *dst, const name *src) { \
        LC_LIST_DEBUG_NULL(dst, #name "_append"); \
        LC_LIST_DEBUG_NULL(src, #name "_append"); \
        return name##_insert_range(dst, name##_len(dst), src, 0, name##_len(src)); \
    } \
    \
    /* ===== Access & Modification ===== */ \
    \
    /** @brief Set an element at the specified position (O(n)) */ \
    static inline int name##_set(name *n, size_t pos, T val) { \
        LC_LIST_DEBUG_NULL(n, #name "_set"); \
        LC_LIST_DEBUG_BOUNDS(n, pos, #name "_set"); \
        return linkedlist_set((LinkedList*)n, pos, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Get an element at the specified position (O(n), panics if out of bounds) */ \
    static inline T name##_at(const name *n, size_t pos) { \
        LC_LIST_DEBUG_NULL(n, #name "_at"); \
        LC_LIST_DEBUG_BOUNDS(n, pos, #name "_at"); \
        void *slot = linkedlist_at_mut((LinkedList*)n, pos); \
        return *(T*)slot; \
    } \
    \
    /** @brief Get an element or return default if out of bounds */ \
    static inline T name##_at_or_default(const name *n, size_t pos, T default_val) { \
        LC_LIST_DEBUG_NULL(n, #name "_at_or_default"); \
        if (pos >= linkedlist_len((LinkedList*)n)) return default_val; \
        void *slot = linkedlist_at_mut((LinkedList*)n, pos); \
        return slot ? *(T*)slot : default_val; \
    } \
    \
    /** @brief Get the first element (O(1), panics if empty) */ \
    static inline T name##_front(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_front"); \
        LC_LIST_DEBUG_EMPTY(n, #name "_front"); \
        void *slot = linkedlist_front_mut((LinkedList*)n); \
        return *(T*)slot; \
    } \
    \
    /** @brief Get the last element (O(1), panics if empty) */ \
    static inline T name##_back(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_back"); \
        LC_LIST_DEBUG_EMPTY(n, #name "_back"); \
        void *slot = linkedlist_back_mut((LinkedList*)n); \
        return *(T*)slot; \
    } \
    \
    /** @brief Get pointer to element (NULL if out of bounds) */ \
    static inline T* name##_get_ptr(name *n, size_t pos) { \
        LC_LIST_DEBUG_NULL(n, #name "_get_ptr"); \
        if (pos >= linkedlist_len((LinkedList*)n)) return NULL; \
        return (T*)linkedlist_at_mut((LinkedList*)n, pos); \
    } \
    \
    /* ===== Removal ===== */ \
    \
    /** @brief Remove and return the first element (O(1)) */ \
    static inline int name##_pop_front(name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_pop_front"); \
        return linkedlist_pop_front((LinkedList*)n); \
    } \
    \
    /** @brief Remove and return the last element (O(1)) */ \
    static inline int name##_pop_back(name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_pop_back"); \
        return linkedlist_pop_back((LinkedList*)n); \
    } \
    \
    /** @brief Remove an element at the specified position (O(n)) */ \
    static inline int name##_remove(name *n, size_t pos) { \
        LC_LIST_DEBUG_NULL(n, #name "_remove"); \
        return linkedlist_remove((LinkedList*)n, pos); \
    } \
    \
    /** @brief Remove all elements from the list */ \
    static inline void name##_clear(name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_clear"); \
        linkedlist_clear((LinkedList*)n); \
    } \
    \
    /* ===== Configuration ===== */ \
    \
    /** @brief Set the comparator for the list */ \
    static inline int name##_set_comparator(name *n, lc_Comparator cmp) { \
        LC_LIST_DEBUG_NULL(n, #name "_set_comparator"); \
        return linkedlist_set_comparator((LinkedList*)n, cmp); \
    } \
    \
    /* ===== In-place Operations ===== */ \
    \
    /** @brief Reverse the list in place (O(n)) */ \
    static inline void name##_reverse_inplace(name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_reverse_inplace"); \
        linkedlist_reverse_inplace((LinkedList*)n); \
    } \
    \
    /** @brief Sort the list in place (O(n log n)) */ \
    static inline int name##_sort(name *n, lc_Comparator cmp) { \
        LC_LIST_DEBUG_NULL(n, #name "_sort"); \
        return linkedlist_sort((LinkedList*)n, cmp); \
    } \
    \
    /* ===== Copy & View ===== */ \
    \
    /** @brief Create a deep copy of the list */ \
    static inline name* name##_clone(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_clone"); \
        return (name*)linkedlist_clone((LinkedList*)n); \
    } \
    \
    /** @brief Create a new list with elements in reverse order (O(n)) */ \
    static inline name* name##_reverse(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_reverse"); \
        return (name*)linkedlist_reverse((LinkedList*)n); \
    } \
    \
    /** @brief Extract a sublist as a new list (O(n)) */ \
    static inline name* name##_sublist(const name *n, size_t from, size_t to) { \
        LC_LIST_DEBUG_NULL(n, #name "_sublist"); \
        return (name*)linkedlist_sublist((LinkedList*)n, from, to); \
    } \
    \
    /** @brief Swap contents of two typed linked lists */ \
    static inline void name##_swap(name *a, name *b) { \
        LC_LIST_DEBUG_NULL(a, #name "_swap"); \
        LC_LIST_DEBUG_NULL(b, #name "_swap"); \
        name tmp = *a; \
        *a = *b; \
        *b = tmp; \
    } \
    \
    /* ===== Iteration ===== */ \
    \
    /** @brief Create a forward iterator over the list */ \
    static inline Iterator name##_iter(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_iter"); \
        return linkedlist_iter((LinkedList*)n); \
    } \
    \
    /** @brief Create a reverse iterator over the list */ \
    static inline Iterator name##_iter_reversed(const name *n) { \
        LC_LIST_DEBUG_NULL(n, #name "_iter_reversed"); \
        return linkedlist_iter_reversed((LinkedList*)n); \
    }

#endif /* CONTAIN_TYPED_LINKEDLIST_PDR_H */