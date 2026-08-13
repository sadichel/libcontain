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
 *   DECL_DEQUE_TYPE(int, sizeof(int), IntDeque)          // Fixed-size
 *   DECL_DEQUE_TYPE(const char*, 0, StringDeque)         // Owned strings
 *   DECL_DEQUE_REF_TYPE(const char*, 0, StringRefDeque)  // Reference strings
 *
 *   int main() {
 *       // Fixed-size
 *       IntDeque *dq1 = IntDeque_create();
 *       IntDeque_push_back(dq1, 42);
 *       IntDeque_push_front(dq1, 10);
 *       int val = IntDeque_at(dq1, 0);  // 10, no cast!
 *
 *       // Cast generic to typed (zero-cost)
 *       IntDeque *dq2 = (IntDeque*)deque_create(sizeof(int));
 *       IntDeque_push(dq2, &val);
 *
 *       // Owned strings - libcontain manages string memory
 *       StringDeque *dq3 = StringDeque_create();
 *       StringDeque_push_back(dq3, "hello");
 *       const char *s1 = StringDeque_at(dq3, 0);  // "hello"
 *
 *       // Reference strings - user manages string memory
 *       StringRefDeque *dq4 = StringRefDeque_create();
 *       StringRefDeque_push_back(dq4, "hello");
 *       const char *s2 = StringRefDeque_at(dq4, 0);  // "hello"
 *
 *       // Cast to generic when needed (zero-cost)
 *       Deque *raw = (Deque*)dq1;
 *       size_t len = deque_len(raw);
 *
 *       IntDeque_destroy(dq1);
 *       IntDeque_destroy(dq2);
 *       StringDeque_destroy(dq3);
 *       StringRefDeque_destroy(dq4);
 *       return 0;
 *   }
 * @endcode
 *
 * @warning The macro generates static inline LC_UNUSED functions. Include this header
 *          in exactly the same way as deque.h — no special implementation
 *          define is needed.
 */

#ifndef CONTAIN_TYPED_DEQUE_PDR_H
#define CONTAIN_TYPED_DEQUE_PDR_H

#include <contain/deque.h>
#include <stdlib.h>

/* Internal debug macros */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_DEQ_DEBUG_NULL(n, func)                                        \
    if (!(n)) {                                                           \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer\n", func); \
        abort();                                                          \
    }
#define LC_DEQ_DEBUG_BOUNDS(deq, idx, func)                                      \
    if ((idx) >= deque_len((Deque *)deq)) {                                      \
        fprintf(stderr, "libcontain panic: %s(%zu) - index %zu >= length %zu\n", \
                func, idx, idx, deque_len((Deque *)deq));                        \
        abort();                                                                 \
    }
#define LC_DEQ_DEBUG_EMPTY(deq, func)                                                  \
    if (deque_is_empty((Deque *)deq)) {                                                \
        fprintf(stderr, "libcontain panic: %s() - called on empty container\n", func); \
        abort();                                                                       \
    }
#else
#define LC_DEQ_DEBUG_NULL(n, func) ((void)0)
#define LC_DEQ_DEBUG_BOUNDS(deq, idx, func) ((void)0)
#define LC_DEQ_DEBUG_EMPTY(deq, func) ((void)0)
#endif

/**
 * @cond INTERNAL
 * @def DEQUE_TYPE_IMPL
 * @brief Generate a type-safe deque wrapper for type T
 *
 * Creates a new type `name` that shares memory layout with Deque,
 * enabling zero-cost casting between typed and generic pointers.
 *
 * For string mode (size == 0), the `owned` parameter controls memory management:
 * - owned = 1: strdup on insert, free on destroy
 * - owned = 0: pointer only, no copy/free
 *
 * @param T    Element type (e.g., int, const char*, MyStruct)
 * @param size Size of T in bytes (0 for string mode)
 * @param name Name for the generated type (e.g., IntDeque)
 * @param owned 1 = container owns/copies strings, 0 = user owns/references
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
 *   - `T const *name##_get_ptr(const name *n, size_t idx)`
 *   - `T *name##_get_mut(name *n, size_t idx)`
 *   - `int name##_resize(name *n, size_t new_len)`
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
 *   - `T* name##_as_slice(name *n)`
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
 *   - `name##Iterator name##_iter(const name *n)`
 *   - `name##Iterator name##_iter_reversed(const name *n)`
 *   - `bool name##_next(name##Iterator *it, T *out)`
 *   - `Iterator name##_as_iterator(name##Iterator it)`
 *
 * @warning T must be copyable by memcpy. For structs with pointers,
 *          use string mode (size=0) or implement manual deep copy.
 *
 * @warning The generated functions return raw values (by copy). For large
 *          structs, consider using the generic API with pointers.
 *
 * @note Panics (abort) in debug mode when preconditions are violated.
 *       Define CONTAINER_DEBUG to enable runtime checks.
 *
 * @endcond
 */

#define DEQUE_TYPE_IMPL(T, size, name, owned)                                                                                                                                           \
    /* Compile-time size validation */                                                                                                                                                  \
    LC_STATIC_ASSERT((size) == 0 || (size) == sizeof(T),                                                                                                                                \
                     "libcontain: size must be 0 (string mode with T=const const char*) or sizeof(T) for fixed-size");                                                                  \
                                                                                                                                                                                        \
    /* Zero-cost overlay: 'name' has same layout as Deque */                                                                                                                            \
    typedef struct name {                                                                                                                                                               \
        Container base;                                                                                                                                                                 \
    } name;                                                                                                                                                                             \
                                                                                                                                                                                        \
    /* Iterator type */                                                                                                                                                                 \
    typedef struct name##Iterator {                                                                                                                                                     \
        Iterator base;                                                                                                                                                                  \
    } name##Iterator;                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Creation & Destruction ===== */                                                                                                                                            \
                                                                                                                                                                                        \
    /** @brief Create a new empty typed deque */                                                                                                                                        \
    static inline LC_UNUSED name *name##_create(void) {                                                                                                                                 \
        DequeBuilder b = deque_builder(size);                                                                                                                                           \
        if (size == 0) b = deque_builder_ref(b, owned);                                                                                                                                 \
        return (name *)deque_builder_build(b);                                                                                                                                          \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Create a new typed deque with specified initial capacity */                                                                                                              \
    static inline LC_UNUSED name *name##_create_with_capacity(size_t cap) {                                                                                                             \
        DequeBuilder b = deque_builder_capacity(deque_builder(size), cap);                                                                                                              \
        if (size == 0) b = deque_builder_ref(b, owned);                                                                                                                                 \
        return (name *)deque_builder_build(b);                                                                                                                                          \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Create a new typed deque with a custom comparator */                                                                                                                     \
    static inline LC_UNUSED name *name##_create_with_comparator(lc_Comparator cmp) {                                                                                                    \
        DequeBuilder b = deque_builder_comparator(deque_builder(size), cmp);                                                                                                            \
        if (size == 0) b = deque_builder_ref(b, owned);                                                                                                                                 \
        return (name *)deque_builder_build(b);                                                                                                                                          \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Create a new typed deque with aligned elements */                                                                                                                        \
    static inline LC_UNUSED name *name##_create_aligned(size_t align) {                                                                                                                 \
        DequeBuilder b = deque_builder_alignment(deque_builder(size), align);                                                                                                           \
        if (size == 0) b = deque_builder_ref(b, owned);                                                                                                                                 \
        return (name *)deque_builder_build(b);                                                                                                                                          \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Destroy a typed deque and free all resources */                                                                                                                          \
    static inline LC_UNUSED void name##_destroy(name *n) {                                                                                                                              \
        deque_destroy((Deque *)n);                                                                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Container Access ===== */                                                                                                                                                  \
                                                                                                                                                                                        \
    /** @brief Get the underlying generic Deque pointer (zero-cost cast) */                                                                                                             \
    static inline LC_UNUSED Deque *name##_unwrap(name *n) {                                                                                                                             \
        return (Deque *)n;                                                                                                                                                              \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get the underlying generic Deque pointer (const, zero-cost cast) */                                                                                                      \
    static inline LC_UNUSED const Deque *name##_unwrap_const(const name *n) {                                                                                                           \
        return (const Deque *)n;                                                                                                                                                        \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /**                                                                                                                                                                                 \
     * @brief Wrap generic container (zero-cost cast, takes ownership)                                                                                                                  \
     *                                                                                                                                                                                  \
     * After calling wrap, do NOT destroy the original container.                                                                                                                       \
     * Use name##_destroy() to free both.                                                                                                                                               \
     */                                                                                                                                                                                 \
    static inline LC_UNUSED name *name##_wrap(Container *c) {                                                                                                                           \
        LC_DEQ_DEBUG_NULL(c, #name "_wrap");                                                                                                                                            \
        return (name *)c;                                                                                                                                                               \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Queries ===== */                                                                                                                                                           \
                                                                                                                                                                                        \
    /** @brief Get the number of elements in the deque */                                                                                                                               \
    static inline LC_UNUSED size_t name##_len(const name *n) {                                                                                                                          \
        return n ? deque_len((Deque *)n) : 0;                                                                                                                                           \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get the current capacity of the deque */                                                                                                                                 \
    static inline LC_UNUSED size_t name##_capacity(const name *n) {                                                                                                                     \
        return n ? deque_capacity((Deque *)n) : 0;                                                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Check if the deque is empty */                                                                                                                                           \
    static inline LC_UNUSED bool name##_is_empty(const name *n) {                                                                                                                       \
        return n ? deque_is_empty((Deque *)n) : true;                                                                                                                                   \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Find the first occurrence of an element */                                                                                                                               \
    static inline LC_UNUSED size_t name##_find(const name *n, T val) {                                                                                                                  \
        LC_DEQ_DEBUG_NULL(n, #name "_find");                                                                                                                                            \
        if (size == 0) {                                                                                                                                                                \
            void *ptr;                                                                                                                                                                  \
            memcpy(&ptr, &val, sizeof(void *));                                                                                                                                         \
            return deque_find((Deque *)n, ptr);                                                                                                                                         \
        }                                                                                                                                                                               \
        return deque_find((Deque *)n, &val);                                                                                                                                            \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Find the last occurrence of an element */                                                                                                                                \
    static inline LC_UNUSED size_t name##_rfind(const name *n, T val) {                                                                                                                 \
        LC_DEQ_DEBUG_NULL(n, #name "_rfind");                                                                                                                                           \
        if (size == 0) {                                                                                                                                                                \
            void *ptr;                                                                                                                                                                  \
            memcpy(&ptr, &val, sizeof(void *));                                                                                                                                         \
            return deque_rfind((Deque *)n, ptr);                                                                                                                                        \
        }                                                                                                                                                                               \
        return deque_rfind((Deque *)n, &val);                                                                                                                                           \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Check if an element exists in the deque */                                                                                                                               \
    static inline LC_UNUSED bool name##_contains(const name *n, T val) {                                                                                                                \
        return name##_find(n, val) != DEQ_NPOS;                                                                                                                                         \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Insertion ===== */                                                                                                                                                         \
                                                                                                                                                                                        \
    /** @brief Append an element to the back of the deque */                                                                                                                            \
    static inline LC_UNUSED int name##_push_back(name *n, T val) {                                                                                                                      \
        LC_DEQ_DEBUG_NULL(n, #name "_push_back");                                                                                                                                       \
        Deque *_d = (Deque *)n;                                                                                                                                                         \
        size_t _len = _d->container.len;                                                                                                                                                \
        size_t _cap = _d->container.capacity;                                                                                                                                           \
        if (size != 0 && _len < _cap) {                                                                                                                                                 \
            size_t _tail = (_d->head + _len) >= _cap ? (_d->head + _len) - _cap : (_d->head + _len);                                                                                    \
            ((T *)_d->container.items)[_tail] = val;                                                                                                                                    \
            _d->container.len = _len + 1;                                                                                                                                               \
            return LC_OK;                                                                                                                                                               \
        }                                                                                                                                                                               \
        if (size == 0) {                                                                                                                                                                \
            void *ptr;                                                                                                                                                                  \
            memcpy(&ptr, &val, sizeof(void *));                                                                                                                                         \
            return deque_push_back(_d, ptr);                                                                                                                                            \
        }                                                                                                                                                                               \
        return deque_push_back(_d, &val);                                                                                                                                               \
    }                                                                                                                                                                                   \
    /** @brief Prepend an element to the front of the deque */                                                                                                                          \
    static inline LC_UNUSED int name##_push_front(name *n, T val) {                                                                                                                     \
        LC_DEQ_DEBUG_NULL(n, #name "_push_front");                                                                                                                                      \
        Deque *_d = (Deque *)n;                                                                                                                                                         \
        size_t _len = _d->container.len;                                                                                                                                                \
        size_t _cap = _d->container.capacity;                                                                                                                                           \
        if (size != 0 && _len < _cap) {                                                                                                                                                 \
            size_t _new_head = (_d->head + _cap - 1) >= _cap ? (_d->head + _cap - 1) - _cap : (_d->head + _cap - 1);                                                                    \
            ((T *)_d->container.items)[_new_head] = val;                                                                                                                                \
            _d->head = _new_head;                                                                                                                                                       \
            _d->container.len = _len + 1;                                                                                                                                               \
            return LC_OK;                                                                                                                                                               \
        }                                                                                                                                                                               \
        if (size == 0) {                                                                                                                                                                \
            void *ptr;                                                                                                                                                                  \
            memcpy(&ptr, &val, sizeof(void *));                                                                                                                                         \
            return deque_push_front(_d, ptr);                                                                                                                                           \
        }                                                                                                                                                                               \
        return deque_push_front(_d, &val);                                                                                                                                              \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Insert an element at the specified position */                                                                                                                           \
    static inline LC_UNUSED int name##_insert(name *n, size_t pos, T val) {                                                                                                             \
        LC_DEQ_DEBUG_NULL(n, #name "_insert");                                                                                                                                          \
        if (size == 0) {                                                                                                                                                                \
            void *ptr;                                                                                                                                                                  \
            memcpy(&ptr, &val, sizeof(void *));                                                                                                                                         \
            return deque_insert((Deque *)n, pos, ptr);                                                                                                                                  \
        }                                                                                                                                                                               \
        return deque_insert((Deque *)n, pos, &val);                                                                                                                                     \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Insert a range of elements from another deque */                                                                                                                         \
    static inline LC_UNUSED int name##_insert_range(name *dst, size_t pos, const name *src, size_t from, size_t to) {                                                                   \
        LC_DEQ_DEBUG_NULL(dst, #name "_insert_range");                                                                                                                                  \
        LC_DEQ_DEBUG_NULL(src, #name "_insert_range");                                                                                                                                  \
        return deque_insert_range((Deque *)dst, pos, (Deque *)src, from, to);                                                                                                           \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Append all elements from another deque */                                                                                                                                \
    static inline LC_UNUSED int name##_append(name *dst, const name *src) {                                                                                                             \
        LC_DEQ_DEBUG_NULL(dst, #name "_append");                                                                                                                                        \
        LC_DEQ_DEBUG_NULL(src, #name "_append");                                                                                                                                        \
        return name##_insert_range(dst, name##_len(dst), src, 0, name##_len(src));                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Try to push to back if capacity permits (no allocation) */                                                                                                               \
    static inline LC_UNUSED bool name##_try_push_back(name *n, T val) {                                                                                                                 \
        LC_DEQ_DEBUG_NULL(n, #name "_try_push_back");                                                                                                                                   \
        if (deque_len((Deque *)n) >= deque_capacity((Deque *)n)) return false;                                                                                                          \
        return name##_push_back(n, val) == LC_OK;                                                                                                                                       \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Try to push to front if capacity permits (no allocation) */                                                                                                              \
    static inline LC_UNUSED bool name##_try_push_front(name *n, T val) {                                                                                                                \
        LC_DEQ_DEBUG_NULL(n, #name "_try_push_front");                                                                                                                                  \
        if (deque_len((Deque *)n) >= deque_capacity((Deque *)n)) return false;                                                                                                          \
        return name##_push_front(n, val) == LC_OK;                                                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Access & Modification ===== */                                                                                                                                             \
                                                                                                                                                                                        \
    /** @brief Set an element at the specified position */                                                                                                                              \
    static inline LC_UNUSED int name##_set(name *n, size_t idx, T val) {                                                                                                                \
        LC_DEQ_DEBUG_NULL(n, #name "_set");                                                                                                                                             \
        LC_DEQ_DEBUG_BOUNDS(n, idx, #name "_set");                                                                                                                                      \
        if (size == 0) {                                                                                                                                                                \
            void *ptr;                                                                                                                                                                  \
            memcpy(&ptr, &val, sizeof(void *));                                                                                                                                         \
            return deque_set((Deque *)n, idx, ptr);                                                                                                                                     \
        }                                                                                                                                                                               \
        return deque_set((Deque *)n, idx, &val);                                                                                                                                        \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get an element at the specified position (panics if out of bounds) */                                                                                                    \
    static inline LC_UNUSED T name##_at(const name *n, size_t idx) {                                                                                                                    \
        LC_DEQ_DEBUG_NULL(n, #name "_at");                                                                                                                                              \
        LC_DEQ_DEBUG_BOUNDS(n, idx, #name "_at");                                                                                                                                       \
        Deque *_d = (Deque *)n;                                                                                                                                                         \
        size_t _phys = (_d->head + idx) >= _d->container.capacity ? (_d->head + idx) - _d->container.capacity : (_d->head + idx);                                                       \
        return ((T *)_d->container.items)[_phys];                                                                                                                                       \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get an element or return default if out of bounds */                                                                                                                     \
    static inline LC_UNUSED T name##_at_or_default(const name *n, size_t idx, T default_val) {                                                                                          \
        LC_DEQ_DEBUG_NULL(n, #name "_at_or_default");                                                                                                                                   \
        if (idx >= deque_len((Deque *)n)) return default_val;                                                                                                                           \
        void *slot = deque_at_mut((Deque *)n, idx);                                                                                                                                     \
        return slot ? *(T *)slot : default_val;                                                                                                                                         \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get the first element (panics if empty) */                                                                                                                               \
    static inline LC_UNUSED T name##_front(const name *n) {                                                                                                                             \
        LC_DEQ_DEBUG_NULL(n, #name "_front");                                                                                                                                           \
        LC_DEQ_DEBUG_EMPTY(n, #name "_front");                                                                                                                                          \
        return ((T *)((Deque *)n)->container.items)[((Deque *)n)->head];                                                                                                                \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get the last element (panics if empty) */                                                                                                                                \
    static inline LC_UNUSED T name##_back(const name *n) {                                                                                                                              \
        LC_DEQ_DEBUG_NULL(n, #name "_back");                                                                                                                                            \
        LC_DEQ_DEBUG_EMPTY(n, #name "_back");                                                                                                                                           \
        Deque *_d = (Deque *)n;                                                                                                                                                         \
        size_t _tail = (_d->head + _d->container.len - 1) >= _d->container.capacity ? (_d->head + _d->container.len - 1) - _d->container.capacity : (_d->head + _d->container.len - 1); \
        return ((T *)_d->container.items)[_tail];                                                                                                                                       \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get a read-only pointer to element (NULL if out of bounds) */                                                                                                            \
    static inline LC_UNUSED T const *name##_get_ptr(const name *n, size_t idx) {                                                                                                        \
        LC_DEQ_DEBUG_NULL(n, #name "_get_ptr");                                                                                                                                         \
        if (idx >= deque_len((Deque *)n)) return NULL;                                                                                                                                  \
        return (T const *)deque_at((Deque *)n, idx);                                                                                                                                    \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get a mutable pointer to element (NULL if out of bounds) */                                                                                                              \
    static inline LC_UNUSED T *name##_get_mut(name *n, size_t idx) {                                                                                                                    \
        LC_DEQ_DEBUG_NULL(n, #name "_get_mut");                                                                                                                                         \
        if (idx >= deque_len((Deque *)n)) return NULL;                                                                                                                                  \
        return (T *)deque_at_mut((Deque *)n, idx);                                                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get a pointer to the underlying array */                                                                                                                                 \
    static inline LC_UNUSED T *name##_as_slice(name *n) {                                                                                                                               \
        LC_DEQ_DEBUG_NULL(n, #name "_as_slice");                                                                                                                                        \
        return (T *)deque_as_slice((Deque *)n);                                                                                                                                         \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Removal ===== */                                                                                                                                                           \
                                                                                                                                                                                        \
    /** @brief Remove and return the last element */                                                                                                                                    \
    static inline LC_UNUSED int name##_pop_back(name *n) {                                                                                                                              \
        LC_DEQ_DEBUG_NULL(n, #name "_pop_back");                                                                                                                                        \
        return deque_pop_back((Deque *)n);                                                                                                                                              \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Remove and return the first element */                                                                                                                                   \
    static inline LC_UNUSED int name##_pop_front(name *n) {                                                                                                                             \
        LC_DEQ_DEBUG_NULL(n, #name "_pop_front");                                                                                                                                       \
        return deque_pop_front((Deque *)n);                                                                                                                                             \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Remove an element at the specified position */                                                                                                                           \
    static inline LC_UNUSED int name##_remove(name *n, size_t idx) {                                                                                                                    \
        LC_DEQ_DEBUG_NULL(n, #name "_remove");                                                                                                                                          \
        return deque_remove((Deque *)n, idx);                                                                                                                                           \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Remove all elements from the deque */                                                                                                                                    \
    static inline LC_UNUSED void name##_clear(name *n) {                                                                                                                                \
        LC_DEQ_DEBUG_NULL(n, #name "_clear");                                                                                                                                           \
        deque_clear((Deque *)n);                                                                                                                                                        \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Capacity Management ===== */                                                                                                                                               \
                                                                                                                                                                                        \
    /** @brief Set the comparator for the deque */                                                                                                                                      \
    static inline LC_UNUSED int name##_set_comparator(name *n, lc_Comparator cmp) {                                                                                                     \
        LC_DEQ_DEBUG_NULL(n, #name "_set_comparator");                                                                                                                                  \
        return deque_set_comparator((Deque *)n, cmp);                                                                                                                                   \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Reserve capacity for expected number of elements */                                                                                                                      \
    static inline LC_UNUSED int name##_reserve(name *n, size_t expected_capacity) {                                                                                                     \
        LC_DEQ_DEBUG_NULL(n, #name "_reserve");                                                                                                                                         \
        return deque_reserve((Deque *)n, expected_capacity);                                                                                                                            \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Resize the deque to a new length. */                                                                                                                                     \
    static inline LC_UNUSED int name##_resize(name *n, size_t new_len) {                                                                                                                \
        LC_DEQ_DEBUG_NULL(n, #name "_resize");                                                                                                                                          \
        return deque_resize((Deque *)n, new_len);                                                                                                                                       \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Shrink capacity to fit current length */                                                                                                                                 \
    static inline LC_UNUSED int name##_shrink_to_fit(name *n) {                                                                                                                         \
        LC_DEQ_DEBUG_NULL(n, #name "_shrink_to_fit");                                                                                                                                   \
        return deque_shrink_to_fit((Deque *)n);                                                                                                                                         \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== In-place Operations ===== */                                                                                                                                               \
                                                                                                                                                                                        \
    /** @brief Reverse the deque in place */                                                                                                                                            \
    static inline LC_UNUSED void name##_reverse_inplace(name *n) {                                                                                                                      \
        LC_DEQ_DEBUG_NULL(n, #name "_reverse_inplace");                                                                                                                                 \
        deque_reverse_inplace((Deque *)n);                                                                                                                                              \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Sort the deque in place */                                                                                                                                               \
    static inline LC_UNUSED int name##_sort(name *n, lc_Comparator cmp) {                                                                                                               \
        LC_DEQ_DEBUG_NULL(n, #name "_sort");                                                                                                                                            \
        return deque_sort((Deque *)n, cmp);                                                                                                                                             \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Copy & View ===== */                                                                                                                                                       \
                                                                                                                                                                                        \
    /** @brief Create a new deque with elements in reverse order */                                                                                                                     \
    static inline LC_UNUSED name *name##_reverse(const name *n) {                                                                                                                       \
        LC_DEQ_DEBUG_NULL(n, #name "_reverse");                                                                                                                                         \
        return (name *)deque_reverse((Deque *)n);                                                                                                                                       \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Create a deep copy of the deque */                                                                                                                                       \
    static inline LC_UNUSED name *name##_clone(const name *n) {                                                                                                                         \
        LC_DEQ_DEBUG_NULL(n, #name "_clone");                                                                                                                                           \
        return (name *)deque_clone((Deque *)n);                                                                                                                                         \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Extract a slice of the deque as a new deque */                                                                                                                           \
    static inline LC_UNUSED name *name##_slice(const name *n, size_t from, size_t to) {                                                                                                 \
        LC_DEQ_DEBUG_NULL(n, #name "_slice");                                                                                                                                           \
        return (name *)deque_slice((Deque *)n, from, to);                                                                                                                               \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Create a new empty deque of the same type */                                                                                                                             \
    static inline LC_UNUSED name *name##_instance(const name *n) {                                                                                                                      \
        LC_DEQ_DEBUG_NULL(n, #name "_instance");                                                                                                                                        \
        return (name *)deque_instance((Deque *)n);                                                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Swap contents of two typed deques */                                                                                                                                     \
    static inline LC_UNUSED void name##_swap(name *a, name *b) {                                                                                                                        \
        LC_DEQ_DEBUG_NULL(a, #name "_swap");                                                                                                                                            \
        LC_DEQ_DEBUG_NULL(b, #name "_swap");                                                                                                                                            \
        name tmp = *a;                                                                                                                                                                  \
        *a = *b;                                                                                                                                                                        \
        *b = tmp;                                                                                                                                                                       \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /* ===== Iteration ===== */                                                                                                                                                         \
                                                                                                                                                                                        \
    /** @brief Create a forward iterator over the deque */                                                                                                                              \
    static inline LC_UNUSED name##Iterator name##_iter(const name *n) {                                                                                                                 \
        LC_DEQ_DEBUG_NULL(n, #name "_iter");                                                                                                                                            \
        return (name##Iterator){ .base = deque_iter((Deque *)n) };                                                                                                                      \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Create a reverse iterator over the deque */                                                                                                                              \
    static inline LC_UNUSED name##Iterator name##_iter_reversed(const name *n) {                                                                                                        \
        LC_DEQ_DEBUG_NULL(n, #name "_iter_reversed");                                                                                                                                   \
        return (name##Iterator){ .base = deque_iter_reversed((Deque *)n) };                                                                                                             \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get the underlying iterator */                                                                                                                                           \
    static inline LC_UNUSED Iterator name##_as_iterator(name##Iterator it) {                                                                                                            \
        return it.base;                                                                                                                                                                 \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    /** @brief Get the next element */                                                                                                                                                  \
    static inline LC_UNUSED bool name##_next(name##Iterator *it, T *out) {                                                                                                              \
        LC_DEQ_DEBUG_NULL(it, #name "_next");                                                                                                                                           \
        LC_DEQ_DEBUG_NULL(out, #name "_next");                                                                                                                                          \
        const void *ptr = iter_next(&it->base);                                                                                                                                         \
        if (!ptr) return false;                                                                                                                                                         \
        if (size == 0) {                                                                                                                                                                \
            *(const void **)out = ptr;                                                                                                                                                  \
        } else {                                                                                                                                                                        \
            memcpy(out, ptr, sizeof(T));                                                                                                                                                \
        }                                                                                                                                                                               \
        return true;                                                                                                                                                                    \
    }

/**
 * @brief Declare a type-safe deque
 *
 * For string mode (size == 0), libcontain manages memory:
 * - strdup on insert
 * - free on destroy
 *
 * For fixed-size types (size > 0), ownership is ignored.
 *
 * @param T    Element type
 * @param size Size of T in bytes (0 for string mode)
 * @param name Name for the generated type
 */
#define DECL_DEQUE_TYPE(T, size, name) \
    DEQUE_TYPE_IMPL(T, size, name, 1)

/**
 * @brief Declare a type-safe deque with explicit ownership control
 *
 * @param T      Element type
 * @param size   Size of T in bytes (0 for string mode)
 * @param name   Name for the generated type
 * @param owned  1 = libcontain owns strings (strdup/free),
 *               0 = user owns strings (reference only)
 */
#define DECL_DEQUE_REF_TYPE(T, size, name, owned) \
    DEQUE_TYPE_IMPL(T, size, name, owned)

#endif /* CONTAIN_TYPED_DEQUE_PDR_H */
