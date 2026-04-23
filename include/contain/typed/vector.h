/** @file vector.h
 * @brief Type-safe wrappers for Vector container
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * This header provides macro-based type-safe wrappers around the generic
 * Vector API. Uses zero-cost overlay design where the typed struct shares
 * the same memory layout as Container, enabling direct casting.
 * 
 * @section example Usage Example
 * @code
 *   // Declare types at global scope
 *   DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
 *   DECL_VECTOR_TYPE(const char*, 0, StringVector)
 *
 *   int main() {
 *       // Create typed vector directly (cast from generic)
 *       IntVector *vec1 = (IntVector*)vector_create(sizeof(int));
 *       IntVector_push(vec1, 42);
 *       IntVector_set(vec1, 0, 100);
 *       int val = IntVector_at(vec1, 0);  // 100, no cast!
 *
 *       // Or use convenience wrappers
 *       StringVector *vec2 = StringVector_create();
 *       StringVector_push(vec2, "hello");
 *       const char *s = StringVector_at(vec2, 0);  // "hello"
 *
 *       IntVector_reverse_inplace(vec1);
 *       IntVector_reserve(vec1, 1000);
 *
 *       // Cast to generic when needed (zero-cost)
 *       Vector *raw = (Vector*)vec1;
 *       size_t len = vector_len(raw);
 *
 *       IntVector_destroy(vec1);
 *       StringVector_destroy(vec2);
 *       return 0;
 *   }
 * @endcode
 *
 * @warning The macro generates static inline functions. Include this header
 *          in exactly the same way as vector.h — no special implementation
 *          define is needed.
 */

#ifndef CONTAIN_TYPED_VECTOR_PDR_H
#define CONTAIN_TYPED_VECTOR_PDR_H

#include <stdlib.h>
#include <contain/vector.h>

/* Internal debug macros */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_VEC_DEBUG_NULL(n, func) \
    if (!(n)) { \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer\n", func); \
        abort(); \
    }
#define LC_VEC_DEBUG_BOUNDS(vec, idx, func) \
    if ((idx) >= vector_len((Vector*)vec)) { \
        fprintf(stderr, "libcontain panic: %s(%zu) - index %zu >= length %zu\n", \
                func, idx, idx, vector_len((Vector*)vec)); \
        abort(); \
    }
#define LC_VEC_DEBUG_EMPTY(vec, func) \
    if (vector_is_empty((Vector*)vec)) { \
        fprintf(stderr, "libcontain panic: %s() - called on empty container\n", func); \
        abort(); \
    }
#else
#define LC_VEC_DEBUG_NULL(n, func) ((void)0)
#define LC_VEC_DEBUG_BOUNDS(vec, idx, func) ((void)0)
#define LC_VEC_DEBUG_EMPTY(vec, func) ((void)0)
#endif

/**
 * @def DECL_VECTOR_TYPE
 * @brief Generate a type-safe vector wrapper for type T
 *
 * Creates a new type `name` that shares memory layout with Vector,
 * enabling zero-cost casting between typed and generic pointers.
 *
 * @param T    Element type (e.g., int, const char*, MyStruct)
 * @param size Size of T in bytes (0 for string mode)
 * @param name Name for the generated type (e.g., IntVector)
 *
 * @par Design Note
 * The typed struct contains a single Vector pointer as its first member,
 * making it binary compatible with Vector*. This allows direct casting
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
 *   - `int name##_push(name *n, T val)`
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
 *   - `int name##_pop(name *n)`
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
 *   - `T* name##_as_slice(const name *n)`
 *
 * **Configuration**
 *   - `int name##_set_comparator(name *n, lc_Comparator cmp)`
 *
 * **Container Access**
 *   - `Vector *name##_unwrap(name *n)` — Cast to generic (zero-cost)
 *   - `const Vector *name##_unwrap_const(const name *n)` — Const cast
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
#define DECL_VECTOR_TYPE(T, size, name) \
    /* Compile-time size validation */ \
    LC_STATIC_ASSERT((size) == 0 || (size) == sizeof(T), \
        "libcontain: size must be 0 (string mode with T=const const char*) or sizeof(T) for fixed-size"); \
    \
    /* Zero-cost overlay: 'name' has same layout as Vector */ \
    typedef struct name { \
        Container base; \
    } name; \
    \
    /* ===== Creation & Destruction ===== */ \
    \
    /** @brief Create a new empty typed vector */ \
    static inline name* name##_create(void) { \
        return (name*)vector_create(size); \
    } \
    \
    /** @brief Create a new typed vector with specified initial capacity */ \
    static inline name* name##_create_with_capacity(size_t cap) { \
        return (name*)vector_create_with_capacity(size, cap); \
    } \
    \
    /** @brief Create a new typed vector with a custom comparator */ \
    static inline name* name##_create_with_comparator(lc_Comparator cmp) { \
        return (name*)vector_create_with_comparator(size, cmp); \
    } \
    \
    /** @brief Create a new typed vector with aligned elements */ \
    static inline name* name##_create_aligned(size_t align) { \
        return (name*)vector_create_aligned(size, align); \
    } \
    \
    /** @brief Destroy a typed vector and free all resources */ \
    static inline void name##_destroy(name *n) { \
        vector_destroy((Vector*)n); \
    } \
    \
    /* ===== Container Access ===== */ \
    \
    /** @brief Get the underlying generic Vector pointer (zero-cost cast) */ \
    static inline Vector *name##_unwrap(name *n) { \
        return (Vector*)n; \
    } \
    \
    /** @brief Get the underlying generic Vector pointer (const, zero-cost cast) */ \
    static inline const Vector *name##_unwrap_const(const name *n) { \
        return (const Vector*)n; \
    } \
    \
    /** \
     * @brief Wrap generic container (zero-cost cast, takes ownership) \
     * \
     * After calling wrap, do NOT destroy the original container. \
     * Use name##_destroy() to free both. \
     */ \
    static inline name* name##_wrap(Container *c) { \
        LC_VEC_DEBUG_NULL(c, #name "_wrap"); \
        return (name*)c; \
    } \
    \
    /* ===== Queries ===== */ \
    \
    /** @brief Get the number of elements in the vector */ \
    static inline size_t name##_len(const name *n) { \
        return n ? vector_len((Vector*)n) : 0; \
    } \
    \
    /** @brief Get the current capacity of the vector */ \
    static inline size_t name##_capacity(const name *n) { \
        return n ? vector_capacity((Vector*)n) : 0; \
    } \
    \
    /** @brief Check if the vector is empty */ \
    static inline bool name##_is_empty(const name *n) { \
        return n ? vector_is_empty((Vector*)n) : true; \
    } \
    \
    /** @brief Find the first occurrence of an element */ \
    static inline size_t name##_find(const name *n, T val) { \
        LC_VEC_DEBUG_NULL(n, #name "_find"); \
        return vector_find((Vector*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Find the last occurrence of an element */ \
    static inline size_t name##_rfind(const name *n, T val) { \
        LC_VEC_DEBUG_NULL(n, #name "_rfind"); \
        return vector_rfind((Vector*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Check if an element exists in the vector */ \
    static inline bool name##_contains(const name *n, T val) { \
        return name##_find(n, val) != VEC_NPOS; \
    } \
    \
    /* ===== Insertion ===== */ \
    \
    /** @brief Append an element to the end of the vector */ \
    static inline int name##_push(name *n, T val) { \
        LC_VEC_DEBUG_NULL(n, #name "_push"); \
        return vector_push((Vector*)n, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    \
    /** @brief Insert an element at the specified position */ \
    static inline int name##_insert(name *n, size_t pos, T val) { \
        LC_VEC_DEBUG_NULL(n, #name "_insert"); \
        return vector_insert((Vector*)n, pos, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Insert a range of elements from another vector */ \
    static inline int name##_insert_range(name *dst, size_t pos, const name *src, size_t from, size_t to) { \
        LC_VEC_DEBUG_NULL(dst, #name "_insert_range"); \
        LC_VEC_DEBUG_NULL(src, #name "_insert_range"); \
        return vector_insert_range((Vector*)dst, pos, (Vector*)src, from, to); \
    } \
    \
    /** @brief Append all elements from another vector */ \
    static inline int name##_append(name *dst, const name *src) { \
        LC_VEC_DEBUG_NULL(dst, #name "_append"); \
        LC_VEC_DEBUG_NULL(src, #name "_append"); \
        return name##_insert_range(dst, name##_len(dst), src, 0, name##_len(src)); \
    } \
    \
    /** @brief Try to push if capacity permits (no allocation) */ \
    static inline bool name##_try_push(name *n, T val) { \
        LC_VEC_DEBUG_NULL(n, #name "_try_push"); \
        if (vector_len((Vector*)n) >= vector_capacity((Vector*)n)) return false; \
        return name##_push(n, val) == LC_OK; \
    } \
    \
    /* ===== Access & Modification ===== */ \
    \
    /** @brief Set an element at the specified position */ \
    static inline int name##_set(name *n, size_t idx, T val) { \
        LC_VEC_DEBUG_NULL(n, #name "_set"); \
        LC_VEC_DEBUG_BOUNDS(n, idx, #name "_set"); \
        return vector_set((Vector*)n, idx, (size == 0) ? (const void*)(*(void**)&val) : &val); \
    } \
    \
    /** @brief Get an element at the specified position (panics if out of bounds) */ \
    static inline T name##_at(const name *n, size_t idx) { \
        LC_VEC_DEBUG_NULL(n, #name "_at"); \
        LC_VEC_DEBUG_BOUNDS(n, idx, #name "_at"); \
        void *slot = vector_at_mut((Vector*)n, idx); \
        return *(T*)slot; \
    } \
    \
    /** @brief Get an element or return default if out of bounds */ \
    static inline T name##_at_or_default(const name *n, size_t idx, T default_val) { \
        LC_VEC_DEBUG_NULL(n, #name "_at_or_default"); \
        if (idx >= vector_len((Vector*)n)) return default_val; \
        void *slot = vector_at_mut((Vector*)n, idx); \
        return slot ? *(T*)slot : default_val; \
    } \
    \
    /** @brief Get the first element (panics if empty) */ \
    static inline T name##_front(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_front"); \
        LC_VEC_DEBUG_EMPTY(n, #name "_front"); \
        void *slot = vector_front_mut((Vector*)n); \
        return *(T*)slot; \
    } \
    \
    /** @brief Get the last element (panics if empty) */ \
    static inline T name##_back(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_back"); \
        LC_VEC_DEBUG_EMPTY(n, #name "_back"); \
        void *slot = vector_back_mut((Vector*)n); \
        return *(T*)slot; \
    } \
    \
    /** @brief Get pointer to element (NULL if out of bounds) */ \
    static inline T* name##_get_ptr(name *n, size_t idx) { \
        LC_VEC_DEBUG_NULL(n, #name "_get_ptr"); \
        if (idx >= vector_len((Vector*)n)) return NULL; \
        return (T*)vector_at_mut((Vector*)n, idx); \
    } \
    \
    /** @brief Get pointer to underlying array (read-only) */ \
    static inline T* name##_as_slice(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_as_slice"); \
        return (T*)vector_front_mut((Vector*)n); \
    } \
    \
    /* ===== Removal ===== */ \
    \
    /** @brief Remove and return the last element */ \
    static inline int name##_pop(name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_pop"); \
        return vector_pop((Vector*)n); \
    } \
    \
    /** @brief Remove an element at the specified position */ \
    static inline int name##_remove(name *n, size_t idx) { \
        LC_VEC_DEBUG_NULL(n, #name "_remove"); \
        return vector_remove((Vector*)n, idx); \
    } \
    \
    /** @brief Remove all elements from the vector */ \
    static inline void name##_clear(name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_clear"); \
        vector_clear((Vector*)n); \
    } \
    \
    /* ===== Capacity Management ===== */ \
    \
    /** @brief Set the comparator for the vector */ \
    static inline int name##_set_comparator(name *n, lc_Comparator cmp) { \
        LC_VEC_DEBUG_NULL(n, #name "_set_comparator"); \
        return vector_set_comparator((Vector*)n, cmp); \
    } \
    \
    /** @brief Reserve capacity for expected number of elements */ \
    static inline int name##_reserve(name *n, size_t expected_capacity) { \
        LC_VEC_DEBUG_NULL(n, #name "_reserve"); \
        return vector_reserve((Vector*)n, expected_capacity); \
    } \
    \
    /** @brief Shrink capacity to fit current length */ \
    static inline int name##_shrink_to_fit(name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_shrink_to_fit"); \
        return vector_shrink_to_fit((Vector*)n); \
    } \
    \
    /* ===== In-place Operations ===== */ \
    \
    /** @brief Reverse the vector in place */ \
    static inline void name##_reverse_inplace(name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_reverse_inplace"); \
        vector_reverse_inplace((Vector*)n); \
    } \
    \
    /** @brief Sort the vector in place */ \
    static inline int name##_sort(name *n, lc_Comparator cmp) { \
        LC_VEC_DEBUG_NULL(n, #name "_sort"); \
        return vector_sort((Vector*)n, cmp); \
    } \
    \
    /* ===== Copy & View ===== */ \
    \
    /** @brief Create a new vector with elements in reverse order */ \
    static inline name* name##_reverse(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_reverse"); \
        return (name*)vector_reverse((Vector*)n); \
    } \
    \
    /** @brief Create a deep copy of the vector */ \
    static inline name* name##_clone(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_clone"); \
        return (name*)vector_clone((Vector*)n); \
    } \
    \
    /** @brief Extract a slice of the vector as a new vector */ \
    static inline name* name##_slice(const name *n, size_t from, size_t to) { \
        LC_VEC_DEBUG_NULL(n, #name "_slice"); \
        return (name*)vector_slice((Vector*)n, from, to); \
    } \
    \
    /** @brief Create a new empty vector of the same type */ \
    static inline name* name##_instance(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_instance"); \
        return (name*)vector_instance((Vector*)n); \
    } \
    \
    /** @brief Swap contents of two typed vectors */ \
    static inline void name##_swap(name *a, name *b) { \
        LC_VEC_DEBUG_NULL(a, #name "_swap"); \
        LC_VEC_DEBUG_NULL(b, #name "_swap"); \
        name tmp = *a; \
        *a = *b; \
        *b = tmp; \
    } \
    \
    /* ===== Iteration ===== */ \
    \
    /** @brief Create a forward iterator over the vector */ \
    static inline Iterator name##_iter(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_iter"); \
        return vector_iter((Vector*)n); \
    } \
    \
    /** @brief Create a reverse iterator over the vector */ \
    static inline Iterator name##_iter_reversed(const name *n) { \
        LC_VEC_DEBUG_NULL(n, #name "_iter_reversed"); \
        return vector_iter_reversed((Vector*)n); \
    }

#endif /* CONTAIN_TYPED_VECTOR_PDR_H */

