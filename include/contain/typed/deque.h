/** @file deque.h
 * @brief Type-safe wrappers for Deque container
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * This header provides macro-based type-safe wrappers around the generic
 * Deque API. Uses zero-cost overlay design where the typed struct shares
 * the same memory layout as Deque, enabling direct casting.
 *
 * @section example Usage Example
 * @code
 *   // Declare types at global scope
 *   DECL_DEQUE_TYPE(int, sizeof(int), IntDeque)
 *   DECL_DEQUE_TYPE(const char*, 0, StringDeque)
 *
 *   int main() {
 *       // Create typed deque directly (cast from generic)
 *       IntDeque *dq1 = (IntDeque*)deque_create(sizeof(int));
 *       IntDeque_push_back(dq1, 42);
 *       IntDeque_push_front(dq1, 10);
 *       int val = IntDeque_at(dq1, 0);  // 10, no cast!
 *
 *       // Or use convenience wrappers
 *       StringDeque *dq2 = StringDeque_create();
 *       StringDeque_push_back(dq2, "hello");
 *       const char *s = StringDeque_at(dq2, 0);  // "hello"
 *
 *       // Cast to generic when needed (zero-cost)
 *       Deque *raw = (Deque*)dq1;
 *       size_t len = deque_len(raw);
 *
 *       IntDeque_destroy(dq1);
 *       StringDeque_destroy(dq2);
 *       return 0;
 *   }
 * @endcode
 *
 * @warning The macro generates static inline functions. Include this header
 *          in exactly the same way as deque.h — no special implementation
 *          define is needed.
 */

#ifndef CONTAIN_TYPED_DEQUE_PDR_H
#define CONTAIN_TYPED_DEQUE_PDR_H

#include <stdlib.h>
#include <contain/deque.h>

/* Internal debug macros */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_DEQ_DEBUG_NULL(n, func) \
    if (!(n)) { \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer\n", func); \
        abort(); \
    }
#define LC_DEQ_DEBUG_BOUNDS(deq, idx, func) \
    if ((idx) >= deque_len((Deque*)deq)) { \
        fprintf(stderr, "libcontain panic: %s(%zu) - index %zu >= length %zu\n", \
                func, idx, idx, deque_len((Deque*)deq)); \
        abort(); \
    }
#define LC_DEQ_DEBUG_EMPTY(deq, func) \
    if (deque_is_empty((Deque*)deq)) { \
        fprintf(stderr, "libcontain panic: %s() - called on empty container\n", func); \
        abort(); \
    }
#else
#define LC_DEQ_DEBUG_NULL(n, func) ((void)0)
#define LC_DEQ_DEBUG_BOUNDS(deq, idx, func) ((void)0)
#define LC_DEQ_DEBUG_EMPTY(deq, func) ((void)0)
#endif

/**
 * @def DECL_DEQUE_TYPE
 * @brief Generate a type-safe deque wrapper for type T
 *
 * Creates a new type `name` that shares memory layout with Deque,
 * enabling zero-cost casting between typed and generic pointers.
 *
 * @param T    Element type (e.g., int, const char*, MyStruct)
 * @param size Size of T in bytes (0 for string mode)
 * @param name Name for the generated type (e.g., IntDeque)
 *
 * @par Design Note
 * The typed struct contains a single Deque pointer as its first member,
 * making it binary compatible with Deque*. This allows direct casting
 * without runtime overhead.
 *
 * @par Generated Functions
 *
 * **Creation & Destruction**
 *   - `name *name##_create(void)`
 *   - `name *name##_create_with_capacity(size_t cap)`
 *   - `name *name##_create_with_comparator(lc_Comparator cmp)`
 *   - `name *name##_create_aligned(size_t align)`
 *   - `void name##_destroy(name *n)`
 *
 * **Insertion**
 *   - `int name##_push_back(name *n, T val)`
 *   - `int name##_push_front(name *n, T val)`
 *   - `int name##_insert(name *n, size_t pos, T val)`
 *   - `int name##_insert_range(name *dst, size_t pos, const name *src, size_t from, size_t to)`
 *   - `int name##_append(name *dst, const name *src)`
 *
 * **Access & Modification**
 *   - `int name##_set(name *n, size_t idx, T val)`
 *   - `T name##_at(const name *n, size_t idx)`
 *   - `T name##_at_or_default(const name *n, size_t idx, T default_val)`
 *   - `T name##_front(const name *n)`
 *   - `T name##_back(const name *n)`
 *   - `T* name##_get_ptr(name *n, size_t idx)`
 *
 * **Removal**
 *   - `int name##_pop_back(name *n)`
 *   - `int name##_pop_front(name *n)`
 *   - `int name##_remove(name *n, size_t idx)`
 *   - `void name##_clear(name *n)`
 *
 * **Queries**
 *   - `size_t name##_len(const name *n)`
 *   - `size_t name##_capacity(const name *n)`
 *   - `bool name##_is_empty(const name *n)`
 *   - `size_t name##_find(const name *n, T val)`
 *   - `size_t name##_rfind(const name *n, T val)`
 *   - `bool name##_contains(const name *n, T val)`
 *
 * **Capacity**
 *   - `int name##_reserve(name *n, size_t expected_capacity)`
 *   - `int name##_shrink_to_fit(name *n)`
 *
 * **In-place Operations**
 *   - `void name##_reverse_inplace(name *n)`
 *   - `int name##_sort(name *n, lc_Comparator cmp)`
 *
 * **Copy & View**
 *   - `name *name##_reverse(const name *n)`
 *   - `name *name##_clone(const name *n)`
 *   - `name *name##_slice(const name *n, size_t from, size_t to)`
 *   - `name *name##_instance(const name *n)`
 *
 * **Configuration**
 *   - `int name##_set_comparator(name *n, lc_Comparator cmp)`
 *
 * **Container Access**
 *   - `Deque *name##_unwrap(name *n)` — Cast to generic (zero-cost)
 *   - `const Deque *name##_unwrap_const(const name *n)` — Const cast
 *   - `name *name##_wrap(Container *c)` — Cast from generic (zero-cost)
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


#define DECL_DEQUE_TYPE(T, size, name) \
    /* Compile-time size validation */ \
    LC_STATIC_ASSERT((size) == 0 || (size) == sizeof(T), \
        "libcontain: size must be 0 (string mode with T=const const char*) or sizeof(T) for fixed-size"); \
    \
    /* Zero-cost overlay: 'name' has same layout as Deque */ \
    typedef struct name { \
        Container base; \
    } name; \
    \
    /* ===== Creation & Destruction ===== */ \
    \
    /** @brief Create a new empty typed deque */ \
    static inline name* name##_create(void) { \
        return (name*)deque_create(size); \
    } \
    \
    /** @brief Create a new typed deque with specified initial capacity */ \
    static inline name* name##_create_with_capacity(size_t cap) { \
        return (name*)deque_create_with_capacity(size, cap); \
    } \
    \
    /** @brief Create a new typed deque with a custom comparator */ \
    static inline name* name##_create_with_comparator(lc_Comparator cmp) { \
        return (name*)deque_create_with_comparator(size, cmp); \
    } \
    \
    /** @brief Create a new typed deque with aligned elements */ \
    static inline name* name##_create_aligned(size_t align) { \
        return (name*)deque_create_aligned(size, align); \
    } \
    \
    /** @brief Destroy a typed deque and free all resources */ \
    static inline void name##_destroy(name *n) { \
        deque_destroy((Deque*)n); \
    } \
    \
    /* ===== Container Access ===== */ \
    \
    /** @brief Get the underlying generic Deque pointer (zero-cost cast) */ \
    static inline Deque *name##_unwrap(name *n) { \
        return (Deque*)n; \
    } \
    \
    /** @brief Get the underlying generic Deque pointer (const, zero-cost cast) */ \
    static inline const Deque *name##_unwrap_const(const name *n) { \
        return (const Deque*)n; \
    } \
    \
    /** \
     * @brief Wrap generic container (zero-cost cast, takes ownership) \
     * \
     * After calling wrap, do NOT destroy the original container. \
     * Use name##_destroy() to free both. \
     */ \
    static inline name* name##_wrap(Container *c) { \
        LC_DEQ_DEBUG_NULL(c, #name "_wrap"); \
        return (name*)c; \
    } \
    \
    /* ===== Queries ===== */ \
    \
    /** @brief Get the number of elements in the deque */ \
    static inline size_t name##_len(const name *n) { \
        return n ? deque_len((Deque*)n) : 0; \
    } \
    \
    /** @brief Get the current capacity of the deque */ \
    static inline size_t name##_capacity(const name *n) { \
        return n ? deque_capacity((Deque*)n) : 0; \
    } \
    \
    /** @brief Check if the deque is empty */ \
    static inline bool name##_is_empty(const name *n) { \
        return n ? deque_is_empty((Deque*)n) : true; \
    } \
    \
    /** @brief Find the first occurrence of an element */ \
    static inline size_t name##_find(const name *n, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_find"); \
        if (size == 0) { \
            void *ptr; \
            memcpy(&ptr, &val, sizeof(void*)); \
            return deque_find((Deque*)n, ptr); \
        } \
        return deque_find((Deque*)n, &val); \
    } \
    \
    /** @brief Find the last occurrence of an element */ \
    static inline size_t name##_rfind(const name *n, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_rfind"); \
        if (size == 0) { \
            void *ptr; \
            memcpy(&ptr, &val, sizeof(void*)); \
            return deque_rfind((Deque*)n, ptr); \
        } \
        return deque_rfind((Deque*)n, &val); \
    } \
    \
    /** @brief Check if an element exists in the deque */ \
    static inline bool name##_contains(const name *n, T val) { \
        return name##_find(n, val) != DEQ_NPOS; \
    } \
    \
    /* ===== Insertion ===== */ \
    \
    /** @brief Append an element to the back of the deque */ \
    static inline int name##_push_back(name *n, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_push_back"); \
        Deque *_d = (Deque*)n; \
        size_t _len = _d->container.len; \
        size_t _cap = _d->container.capacity; \
        if (size != 0 && _len < _cap) { \
            size_t _tail = (_d->head + _len) >= _cap ? (_d->head + _len) - _cap : (_d->head + _len); \
            ((T*)_d->container.items)[_tail] = val; \
            _d->container.len = _len + 1; \
            return LC_OK; \
        } \
        if (size == 0) { \
            void *ptr; \
            memcpy(&ptr, &val, sizeof(void*)); \
            return deque_push_back(_d, ptr); \
        } \
        return deque_push_back(_d, &val); \
    } \
    /** @brief Prepend an element to the front of the deque */ \
    static inline int name##_push_front(name *n, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_push_front"); \
        Deque *_d = (Deque*)n; \
        size_t _len = _d->container.len; \
        size_t _cap = _d->container.capacity; \
        if (size != 0 && _len < _cap) { \
            size_t _new_head = (_d->head + _cap - 1) >= _cap ? (_d->head + _cap - 1) - _cap : (_d->head + _cap - 1); \
            ((T*)_d->container.items)[_new_head] = val; \
            _d->head = _new_head; \
            _d->container.len = _len + 1; \
            return LC_OK; \
        } \
        if (size == 0) { \
            void *ptr; \
            memcpy(&ptr, &val, sizeof(void*)); \
            return deque_push_front(_d, ptr); \
        } \
        return deque_push_front(_d, &val); \
    } \
    \
    /** @brief Insert an element at the specified position */ \
    static inline int name##_insert(name *n, size_t pos, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_insert"); \
        if (size == 0) { \
            void *ptr; \
            memcpy(&ptr, &val, sizeof(void*)); \
            return deque_insert((Deque*)n, pos, ptr); \
        } \
        return deque_insert((Deque*)n, pos, &val); \
    } \
    \
    /** @brief Insert a range of elements from another deque */ \
    static inline int name##_insert_range(name *dst, size_t pos, const name *src, size_t from, size_t to) { \
        LC_DEQ_DEBUG_NULL(dst, #name "_insert_range"); \
        LC_DEQ_DEBUG_NULL(src, #name "_insert_range"); \
        return deque_insert_range((Deque*)dst, pos, (Deque*)src, from, to); \
    } \
    \
    /** @brief Append all elements from another deque */ \
    static inline int name##_append(name *dst, const name *src) { \
        LC_DEQ_DEBUG_NULL(dst, #name "_append"); \
        LC_DEQ_DEBUG_NULL(src, #name "_append"); \
        return name##_insert_range(dst, name##_len(dst), src, 0, name##_len(src)); \
    } \
    \
    /** @brief Try to push to back if capacity permits (no allocation) */ \
    static inline bool name##_try_push_back(name *n, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_try_push_back"); \
        if (deque_len((Deque*)n) >= deque_capacity((Deque*)n)) return false; \
        return name##_push_back(n, val) == LC_OK; \
    } \
    \
    /** @brief Try to push to front if capacity permits (no allocation) */ \
    static inline bool name##_try_push_front(name *n, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_try_push_front"); \
        if (deque_len((Deque*)n) >= deque_capacity((Deque*)n)) return false; \
        return name##_push_front(n, val) == LC_OK; \
    } \
    \
    /* ===== Access & Modification ===== */ \
    \
    /** @brief Set an element at the specified position */ \
    static inline int name##_set(name *n, size_t idx, T val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_set"); \
        LC_DEQ_DEBUG_BOUNDS(n, idx, #name "_set"); \
        if (size == 0) { \
            void *ptr; \
            memcpy(&ptr, &val, sizeof(void*)); \
            return deque_set((Deque*)n, idx, ptr); \
        } \
        return deque_set((Deque*)n, idx, &val); \
    } \
    \
    /** @brief Get an element at the specified position (panics if out of bounds) */ \
    static inline T name##_at(const name *n, size_t idx) { \
        LC_DEQ_DEBUG_NULL(n, #name "_at"); \
        LC_DEQ_DEBUG_BOUNDS(n, idx, #name "_at"); \
        Deque *_d = (Deque*)n; \
        size_t _phys = (_d->head + idx) >= _d->container.capacity ? (_d->head + idx) - _d->container.capacity : (_d->head + idx); \
        return ((T*)_d->container.items)[_phys]; \
    } \
    \
    /** @brief Get an element or return default if out of bounds */ \
    static inline T name##_at_or_default(const name *n, size_t idx, T default_val) { \
        LC_DEQ_DEBUG_NULL(n, #name "_at_or_default"); \
        if (idx >= deque_len((Deque*)n)) return default_val; \
        void *slot = deque_at_mut((Deque*)n, idx); \
        return slot ? *(T*)slot : default_val; \
    } \
    \
    /** @brief Get the first element (panics if empty) */ \
    static inline T name##_front(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_front"); \
        LC_DEQ_DEBUG_EMPTY(n, #name "_front"); \
        return ((T*)((Deque*)n)->container.items)[((Deque*)n)->head]; \
    } \
    \
    /** @brief Get the last element (panics if empty) */ \
    static inline T name##_back(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_back"); \
        LC_DEQ_DEBUG_EMPTY(n, #name "_back"); \
        Deque *_d = (Deque*)n; \
        size_t _tail = (_d->head + _d->container.len - 1) >= _d->container.capacity ? (_d->head + _d->container.len - 1) - _d->container.capacity : (_d->head + _d->container.len - 1); \
        return ((T*)_d->container.items)[_tail]; \
    } \
    /** @brief Get pointer to element (NULL if out of bounds) */ \
    static inline T* name##_get_ptr(name *n, size_t idx) { \
        LC_DEQ_DEBUG_NULL(n, #name "_get_ptr"); \
        if (idx >= deque_len((Deque*)n)) return NULL; \
        return (T*)deque_at_mut((Deque*)n, idx); \
    } \
    \
    /* ===== Removal ===== */ \
    \
    /** @brief Remove and return the last element */ \
    static inline int name##_pop_back(name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_pop_back"); \
        return deque_pop_back((Deque*)n); \
    } \
    \
    /** @brief Remove and return the first element */ \
    static inline int name##_pop_front(name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_pop_front"); \
        return deque_pop_front((Deque*)n); \
    } \
    \
    /** @brief Remove an element at the specified position */ \
    static inline int name##_remove(name *n, size_t idx) { \
        LC_DEQ_DEBUG_NULL(n, #name "_remove"); \
        return deque_remove((Deque*)n, idx); \
    } \
    \
    /** @brief Remove all elements from the deque */ \
    static inline void name##_clear(name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_clear"); \
        deque_clear((Deque*)n); \
    } \
    \
    /* ===== Capacity Management ===== */ \
    \
    /** @brief Set the comparator for the deque */ \
    static inline int name##_set_comparator(name *n, lc_Comparator cmp) { \
        LC_DEQ_DEBUG_NULL(n, #name "_set_comparator"); \
        return deque_set_comparator((Deque*)n, cmp); \
    } \
    \
    /** @brief Reserve capacity for expected number of elements */ \
    static inline int name##_reserve(name *n, size_t expected_capacity) { \
        LC_DEQ_DEBUG_NULL(n, #name "_reserve"); \
        return deque_reserve((Deque*)n, expected_capacity); \
    } \
    \
    /** @brief Shrink capacity to fit current length */ \
    static inline int name##_shrink_to_fit(name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_shrink_to_fit"); \
        return deque_shrink_to_fit((Deque*)n); \
    } \
    \
    /* ===== In-place Operations ===== */ \
    \
    /** @brief Reverse the deque in place */ \
    static inline void name##_reverse_inplace(name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_reverse_inplace"); \
        deque_reverse_inplace((Deque*)n); \
    } \
    \
    /** @brief Sort the deque in place */ \
    static inline int name##_sort(name *n, lc_Comparator cmp) { \
        LC_DEQ_DEBUG_NULL(n, #name "_sort"); \
        return deque_sort((Deque*)n, cmp); \
    } \
    \
    /* ===== Copy & View ===== */ \
    \
    /** @brief Create a new deque with elements in reverse order */ \
    static inline name* name##_reverse(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_reverse"); \
        return (name*)deque_reverse((Deque*)n); \
    } \
    \
    /** @brief Create a deep copy of the deque */ \
    static inline name* name##_clone(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_clone"); \
        return (name*)deque_clone((Deque*)n); \
    } \
    \
    /** @brief Extract a slice of the deque as a new deque */ \
    static inline name* name##_slice(const name *n, size_t from, size_t to) { \
        LC_DEQ_DEBUG_NULL(n, #name "_slice"); \
        return (name*)deque_slice((Deque*)n, from, to); \
    } \
    \
    /** @brief Create a new empty deque of the same type */ \
    static inline name* name##_instance(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_instance"); \
        return (name*)deque_instance((Deque*)n); \
    } \
    \
    /** @brief Swap contents of two typed deques */ \
    static inline void name##_swap(name *a, name *b) { \
        LC_DEQ_DEBUG_NULL(a, #name "_swap"); \
        LC_DEQ_DEBUG_NULL(b, #name "_swap"); \
        name tmp = *a; \
        *a = *b; \
        *b = tmp; \
    } \
    \
    /* ===== Iteration ===== */ \
    \
    /** @brief Create a forward iterator over the deque */ \
    static inline Iterator name##_iter(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_iter"); \
        return deque_iter((Deque*)n); \
    } \
    \
    /** @brief Create a reverse iterator over the deque */ \
    static inline Iterator name##_iter_reversed(const name *n) { \
        LC_DEQ_DEBUG_NULL(n, #name "_iter_reversed"); \
        return deque_iter_reversed((Deque*)n); \
    }

#endif /* CONTAIN_TYPED_DEQUE_PDR_H */