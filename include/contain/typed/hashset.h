/** @file hashset.h
 * @brief Type-safe wrappers for HashSet container
 * libcontain - https://github.com/sadichel/libcontain
 *
 * This header provides macro-based type-safe wrappers around the generic
 * HashSet API. Uses zero-cost overlay design where the typed struct shares
 * the same memory layout as Container, enabling direct casting.
 *
 * @section example Usage Example
 * @code
 *   // Declare types at global scope
 *   DECL_HASHSET_TYPE(int, sizeof(int), IntSet)             // Fixed-size
 *   DECL_HASHSET_TYPE(char*, 0, StringSet)                  // Owned strings
 *   DECL_HASHSET_REF_TYPE(const char*, 0, StringRefSet, 0)  // Reference strings
 *
 *   int main() {
 *       // Fixed-size
 *       IntSet *set1 = IntSet_create();
 *       IntSet_insert(set1, 42);
 *       IntSet_insert(set1, 100);
 *       bool has = IntSet_contains(set1, 42);  // true
 *
 *       // Cast generic to typed (zero-cost)
 *       IntSet *set2 = (IntSet*)hashset_create(sizeof(int));
 *       IntSet_insert(set2, 42);
 *
 *       // Owned strings - libcontain manages string memory
 *       StringSet *set3 = StringSet_create();
 *       StringSet_insert(set3, "hello");
 *       const char *s1 = StringSet_at(set3, 0);  // "hello"
 *
 *       // Reference strings - user manages string memory
 *       StringRefSet *set4 = StringRefSet_create();
 *       StringRefSet_insert(set4, "hello");
 *       const char *s2 = StringRefSet_at(set4, 0);  // "hello"
 *
 *       // Cast to generic when needed (zero-cost)
 *       HashSet *raw = (HashSet*)set1;
 *       size_t len = hashset_len(raw);
 *
 *       IntSet_destroy(set1);
 *       IntSet_destroy(set2);
 *       StringSet_destroy(set3);
 *       StringRefSet_destroy(set4);
 *       return 0;
 *   }
 * @endcode
 *
 * @warning The macro generates static inline LC_UNUSED functions. Include this header
 *          in exactly the same way as hashset.h — no special implementation
 *          define is needed.
 */

#ifndef CONTAIN_TYPED_HASHSET_PDR_H
#define CONTAIN_TYPED_HASHSET_PDR_H

#include <contain/hashset.h>
#include <stdlib.h>

/* Internal debug macros */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_SET_DEBUG_NULL(n, func)                                        \
    if (!(n)) {                                                           \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer\n", func); \
        abort();                                                          \
    }
#define LC_SET_DEBUG_NULL_PAIR(a, b, func)                                   \
    if (!(a) || !(b)) {                                                      \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer(s)\n", func); \
        abort();                                                             \
    }
#else
#define LC_SET_DEBUG_NULL(n, func) ((void)0)
#define LC_SET_DEBUG_NULL_PAIR(a, b, func) ((void)0)
#endif

/**
 * @cond INTERNAL
 * @def HASHSET_TYPE_IMPL
 * @brief Generate a type-safe hash set wrapper for type T
 *
 * Creates a new type `name` that shares memory layout with HashSet,
 * enabling zero-cost casting between typed and generic pointers.
 *
 *  * For string mode (size == 0), the `owned` parameter controls memory management:
 * - owned = 1: strdup on insert, free on destroy
 * - owned = 0: pointer only, no copy/free
 *
 * @param T    Element type (e.g., int, const char*, MyStruct)
 * @param size Size of T in bytes (0 for string mode)
 * @param name Name for the generated type (e.g., IntSet)
 * @param owned 1 = container owns/copies strings, 0 = user owns/references
 *
 * @par Design Note
 * The typed struct contains a single HashSet pointer as its first member,
 * making it binary compatible with HashSet*. This allows direct casting
 * without runtime overhead.
 *
 * @par Generated Functions
 *
 * **Creation & Destruction**
 *   - `name *name##_create(void)`
 *   - `name *name##_create_with_capacity(size_t cap)`
 *   - `name *name##_create_with_hasher(lc_Hasher hasher)`
 *   - `name *name##_create_with_comparator(lc_Comparator cmp)`
 *   - `name *name##_create_aligned(size_t align)`
 *   - `void name##_destroy(name *n)`
 *
 * **Container Access**
 *   - `HashSet *name##_unwrap(name *n)` — Cast to generic (zero-cost)
 *   - `const HashSet *name##_unwrap_const(const name *n)` — Const cast
 *   - `name *name##_wrap(Container *c)` — Cast from generic (zero-cost)
 *
 * **Instance**
 *   - `name *name##_instance(const name *n)`
 *
 * **Core Operations**
 *   - `int name##_insert(name *n, T val)`
 *   - `int name##_remove(name *n, T val)`
 *   - `bool name##_contains(const name *n, T val)`
 *   - `int name##_merge(name *dst, const name *src)`
 *
 * **Queries**
 *   - `size_t name##_len(const name *n)`
 *   - `size_t name##_capacity(const name *n)`
 *   - `bool name##_is_empty(const name *n)`
 *   - `size_t name##_hash(const name *n)`
 *
 * **Modification**
 *   - `void name##_clear(name *n)`
 *   - `int name##_reserve(name *n, size_t expected)`
 *   - `int name##_shrink_to_fit(name *n)`
 *
 * **Configuration**
 *   - `int name##_set_hasher(name *n, lc_Hasher hasher)`
 *   - `int name##_set_comparator(name *n, lc_Comparator cmp)`
 *
 * **Set Operations (returns new set)**
 *   - `name *name##_clone(const name *n)`
 *   - `name *name##_union(const name *a, const name *b)`
 *   - `name *name##_intersection(const name *a, const name *b)`
 *   - `name *name##_difference(const name *a, const name *b)`
 *
 * **Set Comparisons**
 *   - `bool name##_subset(const name *a, const name *b)`
 *   - `bool name##_equals(const name *a, const name *b)`
 *
 * **Export & Iteration**
 *   - `Array *name##_to_array(const name *n)`
 *   - `name##Iterator name##_iter(const name *n)`
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
#define HASHSET_TYPE_IMPL(T, size, name, owned)                                                                  \
    /* Compile-time size validation */                                                                           \
    LC_STATIC_ASSERT((size) == 0 || (size) == sizeof(T),                                                         \
                     "libcontain: size must be 0 (string mode with T=const char*) or sizeof(T) for fixed-size"); \
                                                                                                                 \
    /* Zero-cost overlay: 'name' has same layout as HashSet */                                                   \
    typedef struct name {                                                                                        \
        Container base;                                                                                          \
    } name;                                                                                                      \
                                                                                                                 \
    /* Iterator type */                                                                                          \
    typedef struct name##Iterator {                                                                              \
        Iterator base;                                                                                           \
    } name##Iterator;                                                                                            \
                                                                                                                 \
    /* ===== Creation & Destruction ===== */                                                                     \
                                                                                                                 \
    /** @brief Create a new empty typed hash set */                                                              \
    static inline LC_UNUSED name *name##_create(void) {                                                          \
        HashSetBuilder b = hashset_builder(size);                                                                \
        if (size == 0) b = hashset_builder_ref(b, owned);                                                        \
        return (name *)hashset_builder_build(b);                                                                 \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new typed hash set with specified initial bucket count */                                \
    static inline LC_UNUSED name *name##_create_with_capacity(size_t cap) {                                      \
        HashSetBuilder b = hashset_builder_capacity(hashset_builder(size), cap);                                 \
        if (size == 0) b = hashset_builder_ref(b, owned);                                                        \
        return (name *)hashset_builder_build(b);                                                                 \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new typed hash set with a custom hash function */                                        \
    static inline LC_UNUSED name *name##_create_with_hasher(lc_Hasher hasher) {                                  \
        HashSetBuilder b = hashset_builder_hasher(hashset_builder(size), hasher);                                \
        if (size == 0) b = hashset_builder_ref(b, owned);                                                        \
        return (name *)hashset_builder_build(b);                                                                 \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new typed hash set with a custom comparator */                                           \
    static inline LC_UNUSED name *name##_create_with_comparator(lc_Comparator cmp) {                             \
        HashSetBuilder b = hashset_builder_comparator(hashset_builder(size), cmp);                               \
        if (size == 0) b = hashset_builder_ref(b, owned);                                                        \
        return (name *)hashset_builder_build(b);                                                                 \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new typed hash set with aligned elements */                                              \
    static inline LC_UNUSED name *name##_create_aligned(size_t align) {                                          \
        HashSetBuilder b = hashset_builder_alignment(hashset_builder(size), align);                              \
        if (size == 0) b = hashset_builder_ref(b, owned);                                                        \
        return (name *)hashset_builder_build(b);                                                                 \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Destroy a typed hash set and free all resources */                                                \
    static inline LC_UNUSED void name##_destroy(name *n) {                                                       \
        hashset_destroy((HashSet *)n);                                                                           \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Container Access ===== */                                                                           \
                                                                                                                 \
    /** @brief Get the underlying generic HashSet pointer (zero-cost cast) */                                    \
    static inline LC_UNUSED HashSet *name##_unwrap(name *n) {                                                    \
        return (HashSet *)n;                                                                                     \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Get the underlying generic HashSet pointer (const, zero-cost cast) */                             \
    static inline LC_UNUSED const HashSet *name##_unwrap_const(const name *n) {                                  \
        return (const HashSet *)n;                                                                               \
    }                                                                                                            \
                                                                                                                 \
    /**                                                                                                          \
     * @brief Wrap generic container (zero-cost cast, takes ownership)                                           \
     *                                                                                                           \
     * After calling wrap, do NOT destroy the original container.                                                \
     * Use name##_destroy() to free both.                                                                        \
     */                                                                                                          \
    static inline LC_UNUSED name *name##_wrap(Container *c) {                                                    \
        LC_SET_DEBUG_NULL(c, #name "_wrap");                                                                     \
        return (name *)c;                                                                                        \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new empty hash set of the same type */                                                   \
    static inline LC_UNUSED name *name##_instance(const name *n) {                                               \
        LC_SET_DEBUG_NULL(n, #name "_instance");                                                                 \
        return (name *)hashset_instance((HashSet *)n);                                                           \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Core Operations ===== */                                                                            \
                                                                                                                 \
    /** @brief Insert an element into the hash set */                                                            \
    static inline LC_UNUSED int name##_insert(name *n, T val) {                                                  \
        LC_SET_DEBUG_NULL(n, #name "_insert");                                                                   \
        if (size == 0) {                                                                                         \
            void *ptr;                                                                                           \
            memcpy(&ptr, &val, sizeof(void *));                                                                  \
            return hashset_insert((HashSet *)n, ptr);                                                            \
        }                                                                                                        \
        return hashset_insert((HashSet *)n, &val);                                                               \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Remove an element from the hash set */                                                            \
    static inline LC_UNUSED int name##_remove(name *n, T val) {                                                  \
        LC_SET_DEBUG_NULL(n, #name "_remove");                                                                   \
        if (size == 0) {                                                                                         \
            void *ptr;                                                                                           \
            memcpy(&ptr, &val, sizeof(void *));                                                                  \
            return hashset_remove((HashSet *)n, ptr);                                                            \
        }                                                                                                        \
        return hashset_remove((HashSet *)n, &val);                                                               \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Check if an element exists in the hash set */                                                     \
    static inline LC_UNUSED bool name##_contains(const name *n, T val) {                                         \
        LC_SET_DEBUG_NULL(n, #name "_contains");                                                                 \
        if (size == 0) {                                                                                         \
            void *ptr;                                                                                           \
            memcpy(&ptr, &val, sizeof(void *));                                                                  \
            return hashset_contains((HashSet *)n, ptr);                                                          \
        }                                                                                                        \
        return hashset_contains((HashSet *)n, &val);                                                             \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Merge another hash set into this one (in-place union) */                                          \
    static inline LC_UNUSED int name##_merge(name *dst, const name *src) {                                       \
        LC_SET_DEBUG_NULL_PAIR(dst, src, #name "_merge");                                                        \
        return hashset_merge((HashSet *)dst, (HashSet *)src);                                                    \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Queries ===== */                                                                                    \
                                                                                                                 \
    /** @brief Get the number of elements in the hash set */                                                     \
    static inline LC_UNUSED size_t name##_len(const name *n) {                                                   \
        return n ? hashset_len((HashSet *)n) : 0;                                                                \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Get the current bucket count of the hash set */                                                   \
    static inline LC_UNUSED size_t name##_capacity(const name *n) {                                              \
        return n ? hashset_capacity((HashSet *)n) : 0;                                                           \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Check if the hash set is empty */                                                                 \
    static inline LC_UNUSED bool name##_is_empty(const name *n) {                                                \
        return n ? hashset_is_empty((HashSet *)n) : true;                                                        \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Compute a hash of the hash set's contents */                                                      \
    static inline LC_UNUSED size_t name##_hash(const name *n) {                                                  \
        return n ? hashset_hash((HashSet *)n) : 0;                                                               \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Modification ===== */                                                                               \
                                                                                                                 \
    /** @brief Remove all elements from the hash set */                                                          \
    static inline LC_UNUSED void name##_clear(name *n) {                                                         \
        LC_SET_DEBUG_NULL(n, #name "_clear");                                                                    \
        hashset_clear((HashSet *)n);                                                                             \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Reserve capacity for expected number of elements */                                               \
    static inline LC_UNUSED int name##_reserve(name *n, size_t expected) {                                       \
        LC_SET_DEBUG_NULL(n, #name "_reserve");                                                                  \
        return hashset_reserve((HashSet *)n, expected);                                                          \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Shrink bucket count to fit current load factor */                                                 \
    static inline LC_UNUSED int name##_shrink_to_fit(name *n) {                                                  \
        LC_SET_DEBUG_NULL(n, #name "_shrink_to_fit");                                                            \
        return hashset_shrink_to_fit((HashSet *)n);                                                              \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Configuration ===== */                                                                              \
                                                                                                                 \
    /** @brief Set the hash function for the hash set (empty set only) */                                        \
    static inline LC_UNUSED int name##_set_hasher(name *n, lc_Hasher hasher) {                                   \
        LC_SET_DEBUG_NULL(n, #name "_set_hasher");                                                               \
        return hashset_set_hasher((HashSet *)n, hasher);                                                         \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Set the comparator for the hash set (empty set only) */                                           \
    static inline LC_UNUSED int name##_set_comparator(name *n, lc_Comparator cmp) {                              \
        LC_SET_DEBUG_NULL(n, #name "_set_comparator");                                                           \
        return hashset_set_comparator((HashSet *)n, cmp);                                                        \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Set Operations ===== */                                                                             \
                                                                                                                 \
    /** @brief Create a deep copy of the hash set */                                                             \
    static inline LC_UNUSED name *name##_clone(const name *n) {                                                  \
        LC_SET_DEBUG_NULL(n, #name "_clone");                                                                    \
        return (name *)hashset_clone((HashSet *)n);                                                              \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new hash set containing the union of two sets */                                         \
    static inline LC_UNUSED name *name##_union(const name *a, const name *b) {                                   \
        LC_SET_DEBUG_NULL_PAIR(a, b, #name "_union");                                                            \
        return (name *)hashset_union((HashSet *)a, (HashSet *)b);                                                \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new hash set containing the intersection of two sets */                                  \
    static inline LC_UNUSED name *name##_intersection(const name *a, const name *b) {                            \
        LC_SET_DEBUG_NULL_PAIR(a, b, #name "_intersection");                                                     \
        return (name *)hashset_intersection((HashSet *)a, (HashSet *)b);                                         \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a new hash set containing the difference of two sets (A \ B) */                            \
    static inline LC_UNUSED name *name##_difference(const name *a, const name *b) {                              \
        LC_SET_DEBUG_NULL_PAIR(a, b, #name "_difference");                                                       \
        return (name *)hashset_difference((HashSet *)a, (HashSet *)b);                                           \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Set Comparisons ===== */                                                                            \
                                                                                                                 \
    /** @brief Check if A is a subset of B */                                                                    \
    static inline LC_UNUSED bool name##_subset(const name *a, const name *b) {                                   \
        LC_SET_DEBUG_NULL_PAIR(a, b, #name "_subset");                                                           \
        return hashset_subset((HashSet *)a, (HashSet *)b);                                                       \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Check if two hash sets are equal */                                                               \
    static inline LC_UNUSED bool name##_equals(const name *a, const name *b) {                                   \
        LC_SET_DEBUG_NULL_PAIR(a, b, #name "_equals");                                                           \
        return hashset_equals((HashSet *)a, (HashSet *)b);                                                       \
    }                                                                                                            \
                                                                                                                 \
    /* ===== Export & Iteration ===== */                                                                         \
                                                                                                                 \
    /** @brief Export the hash set to a flat Array snapshot */                                                   \
    static inline LC_UNUSED Array *name##_to_array(const name *n) {                                              \
        LC_SET_DEBUG_NULL(n, #name "_to_array");                                                                 \
        return hashset_to_array((HashSet *)n);                                                                   \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Create a forward iterator over the hash set */                                                    \
    static inline LC_UNUSED name##Iterator name##_iter(const name *n) {                                          \
        LC_SET_DEBUG_NULL(n, #name "_iter");                                                                     \
        return (name##Iterator){ .base = hashset_iter((HashSet *)n) };                                           \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Get the underlying iterator */                                                                    \
    static inline LC_UNUSED Iterator name##_as_iterator(name##Iterator it) {                                     \
        return it.base;                                                                                          \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Get the next element */                                                                           \
    static inline LC_UNUSED bool name##_next(name##Iterator *it, T *out) {                                       \
        LC_SET_DEBUG_NULL(it, #name "_next");                                                                    \
        LC_SET_DEBUG_NULL(out, #name "_next");                                                                   \
        const void *ptr = iter_next(&it->base);                                                                  \
        if (!ptr) return false;                                                                                  \
        if (size == 0) {                                                                                         \
            *(const void **)out = ptr;                                                                           \
        } else {                                                                                                 \
            memcpy(out, ptr, sizeof(T));                                                                         \
        }                                                                                                        \
        return true;                                                                                             \
    }                                                                                                            \
                                                                                                                 \
    /** @brief Swap contents of two typed hash sets */                                                           \
    static inline LC_UNUSED void name##_swap(name *a, name *b) {                                                 \
        LC_SET_DEBUG_NULL_PAIR(a, b, #name "_swap");                                                             \
        name tmp = *a;                                                                                           \
        *a = *b;                                                                                                 \
        *b = tmp;                                                                                                \
    }

/**
 * @brief Declare a type-safe hashset
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
#define DECL_HASHSET_TYPE(T, size, name) \
    HASHSET_TYPE_IMPL(T, size, name, 1)

/**
 * @brief Declare a type-safe hashset with explicit ownership control
 *
 * @param T      Element type
 * @param size   Size of T in bytes (0 for string mode)
 * @param name   Name for the generated type
 * @param owned  1 = libcontain owns strings (strdup/free),
 *               0 = user owns strings (reference only)
 */
#define DECL_HASHSET_REF_TYPE(T, size, name, owned) \
    HASHSET_TYPE_IMPL(T, size, name, owned)

#endif /* CONTAIN_TYPED_HASHSET_PDR_H */
