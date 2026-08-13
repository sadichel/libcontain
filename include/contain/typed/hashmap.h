/** @file hashmap.h
 * @brief Type-safe wrappers for HashMap container
 * libcontain - https://github.com/sadichel/libcontain
 *
 * This header provides macro-based type-safe wrappers around the generic
 * HashMap API. Uses zero-cost overlay design where the typed struct shares
 * the same memory layout as Container, enabling direct casting.
 *
 * @section example Usage Example
 * @code
 *   // Declare types at global scope
 *   DECL_HASHMAP_TYPE(int, int, sizeof(int), sizeof(int), IntIntMap)
 *   DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)
 *   DECL_HASHMAP_TYPE(const char*, const char*, 0, 0, StrStrMap)
 *   DECL_HASHMAP_REF_TYPE(const char*, const char*, 0, 0, StrStrMapRef, 0, 0)
 *
 *   int main() {
 *       // Fixed-size - no ownership
 *       IntIntMap *map1 = IntIntMap_create();
 *       IntIntMap_insert(map1, 1, 100);
 *       IntIntMap_insert(map1, 2, 200);
 *       int val = IntIntMap_get(map1, 1);  // 100, no cast!
 *
 *       // Cast generic to typed (zero-cost)
 *       IntIntMap *map2 = (IntIntMap*)hashmap_create(sizeof(int), sizeof(int));
 *       IntIntMap_insert(map2, 1, 100);
 *       IntIntMap_insert(map2, 2, 200);
 *
 *       // Owned String Fixed
 *       StrIntMap *map3 = StrIntMap_create();
 *       StrIntMap_insert(map3, "hello", 100);
 *       int val2 = StrIntMap_get(map3, "hello");  // 100, no cast!
 *
 *       // Owned strings - libcontain manages string memory
 *       StrStrMap *map4 = StrStrMap_create();
 *       StrStrMap_insert(map4, "hello", "world");
 *       const char *s = StrStrMap_get(map4, "hello");  // "world"
 *
 *       // Reference strings - user manages string memory
 *       StrStrMapRef *map5 = StrStrMapRef_create();
 *       StrStrMapRef_insert(map5, "hello", "world");
 *       const char *s2 = StrStrMapRef_get(map5, "hello");  // "world"
 *
 *       // Cast to generic when needed (zero-cost)
 *       HashMap *raw = (HashMap*)map1;
 *       size_t len = hashmap_len(raw);
 *
 *       IntIntMap_destroy(map2);
 *       StrStrMapRef_destroy(map4);
 *       StrIntMap_destroy(map5);
 *       StrStrMapRef_destroy(map3);
 *       return 0;
 *   }
 * @endcode
 *
 * @warning The macro generates static inline LC_UNUSED functions. Include this header
 *          in exactly the same way as hashmap.h — no special implementation
 *          define is needed.
 */

#ifndef CONTAIN_TYPED_HASHMAP_PDR_H
#define CONTAIN_TYPED_HASHMAP_PDR_H

#include <contain/hashmap.h>
#include <stdlib.h>

/* Internal debug macros */
#ifdef CONTAINER_DEBUG
#include <stdio.h>
#define LC_MAP_DEBUG_NULL(n, func)                                        \
    if (!(n)) {                                                           \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer\n", func); \
        abort();                                                          \
    }
#define LC_MAP_DEBUG_NULL_PAIR(a, b, func)                                   \
    if (!(a) || !(b)) {                                                      \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer(s)\n", func); \
        abort();                                                             \
    }
#define LC_MAP_DEBUG_NULL_ENTRY(n, entry, func)                              \
    if (!(n) || !(entry)) {                                                  \
        fprintf(stderr, "libcontain panic: %s() - NULL pointer(s)\n", func); \
        abort();                                                             \
    }
#else
#define LC_MAP_DEBUG_NULL(n, func) ((void)0)
#define LC_MAP_DEBUG_NULL_PAIR(a, b, func) ((void)0)
#define LC_MAP_DEBUG_NULL_ENTRY(n, entry, func) ((void)0)
#endif

/**
 * @cond INTERNAL
 * @def HASHMAP_TYPE_IMPL
 * @brief Generate a type-safe hash map wrapper for key type K and value type V
 *
 * Creates a new type `name` that shares memory layout with HashMap,
 * enabling zero-cost casting between typed and generic pointers.
 *
 * For string mode (size == 0), the `owned` parameter controls memory management:
 * - owned = 1: strdup on insert, free on destroy
 * - owned = 0: pointer only, no copy/free
 *
 * @param K      Key type (e.g., int, const char*, MyStruct)
 * @param V      Value type (e.g., int, const char*, MyStruct)
 * @param ksize  Size of K in bytes (0 for string mode)
 * @param vsize  Size of V in bytes (0 for string mode)
 * @param name   Name for the generated type (e.g., IntIntMap, StrStrMap)
 * @param kowned Whether the key is owned by the map (1 for owned, 0 for borrowed)
 * @param vowned Whether the value is owned by the map (1 for owned, 0 for borrowed)
 *
 * @par Design Note
 * The typed struct contains a single HashMap pointer as its first member,
 * making it binary compatible with HashMap*. This allows direct casting
 * without runtime overhead.
 *
 * @par Generated Functions
 *
 * **Creation & Destruction**
 *   - `name *name##_create(void)`
 *   - `name *name##_create_with_capacity(size_t cap)`
 *   - `name *name##_create_with_hasher(lc_Hasher hasher)`
 *   - `name *name##_create_with_comparator(lc_Comparator kcmp, lc_Comparator vcmp)`
 *   - `name *name##_create_aligned(size_t key_align, size_t val_align)`
 *   - `void name##_destroy(name *n)`
 *
 * **Container Access**
 *   - `HashMap *name##_unwrap(name *n)` — Cast to generic (zero-cost)
 *   - `const HashMap *name##_unwrap_const(const name *n)` — Const cast
 *   - `name *name##_wrap(Container *c)` — Cast from generic (zero-cost)
 *
 * **Instance**
 *   - `name *name##_instance(const name *n)`
 *
 * **Core Operations**
 *   - `int name##_insert(name *n, K key, V val)`
 *   - `int name##_remove(name *n, K key)`
 *   - `int name##_remove_entry(name *n, K key, V val)`
 *   - `V name##_get(const name *n, K key)`
 *   - `V name##_get_or_default(const name *n, K key, V dval)`
 *   - `V const *name##_get_ptr(const name *n, K key)`  
 *   - `V* name##_get_mut(name *n, K key)`
 *   - `bool name##_contains(const name *n, K key)`
 *   - `bool name##_contains_entry(const name *n, K key, V val)`
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
 *   - `int name##_set_comparator(name *n, lc_Comparator kcmp, lc_Comparator vcmp)`
 *
 * **Copy & Comparison**
 *   - `name *name##_clone(const name *n)`
 *   - `bool name##_equals(const name *a, const name *b)`
 *   - `void name##_swap(name *a, name *b)`
 *
 * **Export & Iteration**
 *   - `Array *name##_keys(const name *n)`
 *   - `Array *name##_values(const name *n)`
 *   - `name##Iterator name##_iter(const name *n)`
 *   - `bool name##_next(name##Iterator *it, K *key_out, V *val_out)`
 *   - `Iterator name##_as_iterator(name##Iterator it)`
 *
 * @warning T must be copyable by memcpy. For structs with pointers,
 *          use string mode (size=0) or implement manual deep copy.
 *
 * @note Panics (abort) in debug mode when preconditions are violated.
 *       Define CONTAINER_DEBUG to enable runtime checks.
 *
 * @endcond
 */
#define HASHMAP_TYPE_IMPL(K, V, ksize, vsize, name, kowned, vowned)                                                     \
    /* Compile-time size validation */                                                                                  \
    LC_STATIC_ASSERT((ksize) == 0 || (ksize) == sizeof(K),                                                              \
                     "libcontain: ksize must be 0 (string mode with K=const const char*) or sizeof(K) for fixed-size"); \
    LC_STATIC_ASSERT((vsize) == 0 || (vsize) == sizeof(V),                                                              \
                     "libcontain: vsize must be 0 (string mode with V=const const char*) or sizeof(V) for fixed-size"); \
                                                                                                                        \
    /* Zero-cost overlay: 'name' has same layout as HashMap */                                                          \
    typedef struct name {                                                                                               \
        Container base;                                                                                                 \
    } name;                                                                                                             \
                                                                                                                        \
    /* Iterator type */                                                                                                 \
    typedef struct name##Iterator {                                                                                     \
        Iterator base;                                                                                                  \
    } name##Iterator;                                                                                                   \
                                                                                                                        \
    /* ===== Creation & Destruction ===== */                                                                            \
                                                                                                                        \
    /** @brief Create a new empty typed hash map */                                                                     \
    static inline LC_UNUSED name *name##_create(void) {                                                                 \
        HashMapBuilder b = hashmap_builder(ksize, vsize);                                                               \
        if (ksize == 0 || vsize == 0) b = hashmap_builder_ref(b, kowned, vowned);                                       \
        return (name *)hashmap_builder_build(b);                                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Create a new typed hash map with specified initial bucket count */                                       \
    static inline LC_UNUSED name *name##_create_with_capacity(size_t cap) {                                             \
        HashMapBuilder b = hashmap_builder_capacity(hashmap_builder(ksize, vsize), cap);                                \
        if (ksize == 0 || vsize == 0) b = hashmap_builder_ref(b, kowned, vowned);                                       \
        return (name *)hashmap_builder_build(b);                                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Create a new typed hash map with a custom key hash function */                                           \
    static inline LC_UNUSED name *name##_create_with_hasher(lc_Hasher hasher) {                                         \
        HashMapBuilder b = hashmap_builder_hasher(hashmap_builder(ksize, vsize), hasher);                               \
        if (ksize == 0 || vsize == 0) b = hashmap_builder_ref(b, kowned, vowned);                                       \
        return (name *)hashmap_builder_build(b);                                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Create a new typed hash map with custom key and value comparators */                                     \
    static inline LC_UNUSED name *name##_create_with_comparator(lc_Comparator kcmp, lc_Comparator vcmp) {               \
        HashMapBuilder b = hashmap_builder_comparators(hashmap_builder(ksize, vsize), kcmp, vcmp);                      \
        if (ksize == 0 || vsize == 0) b = hashmap_builder_ref(b, kowned, vowned);                                       \
        return (name *)hashmap_builder_build(b);                                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Create a new typed hash map with aligned keys and values */                                              \
    static inline LC_UNUSED name *name##_create_aligned(size_t key_align, size_t val_align) {                           \
        HashMapBuilder b = hashmap_builder_alignment(hashmap_builder(ksize, vsize), key_align, val_align);              \
        if (ksize == 0 || vsize == 0) b = hashmap_builder_ref(b, kowned, vowned);                                       \
        return (name *)hashmap_builder_build(b);                                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Destroy a typed hash map and free all resources */                                                       \
    static inline LC_UNUSED void name##_destroy(name *n) {                                                              \
        hashmap_destroy((HashMap *)n);                                                                                  \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Container Access ===== */                                                                                  \
                                                                                                                        \
    /** @brief Get the underlying generic HashMap pointer (zero-cost cast) */                                           \
    static inline LC_UNUSED HashMap *name##_unwrap(name *n) {                                                           \
        return (HashMap *)n;                                                                                            \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get the underlying generic HashMap pointer (const, zero-cost cast) */                                    \
    static inline LC_UNUSED const HashMap *name##_unwrap_const(const name *n) {                                         \
        return (const HashMap *)n;                                                                                      \
    }                                                                                                                   \
                                                                                                                        \
    /**                                                                                                                 \
     * @brief Wrap generic container (zero-cost cast, takes ownership)                                                  \
     *                                                                                                                  \
     * After calling wrap, do NOT destroy the original container.                                                       \
     * Use name##_destroy() to free both.                                                                               \
     */                                                                                                                 \
    static inline LC_UNUSED name *name##_wrap(Container *c) {                                                           \
        LC_MAP_DEBUG_NULL(c, #name "_wrap");                                                                            \
        return (name *)c;                                                                                               \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Create a new empty hash map of the same type */                                                          \
    static inline LC_UNUSED name *name##_instance(const name *n) {                                                      \
        LC_MAP_DEBUG_NULL(n, #name "_instance");                                                                        \
        return (name *)hashmap_instance((HashMap *)n);                                                                  \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Core Operations ===== */                                                                                   \
                                                                                                                        \
    /** @brief Insert or update a key-value pair */                                                                     \
    static inline LC_UNUSED int name##_insert(name *n, K key, V val) {                                                  \
        LC_MAP_DEBUG_NULL(n, #name "_insert");                                                                          \
        if (ksize != 0 && vsize != 0) {                                                                                 \
            return hashmap_insert((HashMap *)n, &key, &val);                                                            \
        }                                                                                                               \
        const void *kptr, *vptr;                                                                                        \
        if (ksize == 0) {                                                                                               \
            memcpy((void *)&kptr, &key, sizeof(void *));                                                                \
        } else {                                                                                                        \
            kptr = &key;                                                                                                \
        }                                                                                                               \
        if (vsize == 0) {                                                                                               \
            memcpy((void *)&vptr, &val, sizeof(void *));                                                                \
        } else {                                                                                                        \
            vptr = &val;                                                                                                \
        }                                                                                                               \
        return hashmap_insert((HashMap *)n, kptr, vptr);                                                                \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Remove a key-value pair by key */                                                                        \
    static inline LC_UNUSED int name##_remove(name *n, K key) {                                                         \
        LC_MAP_DEBUG_NULL(n, #name "_remove");                                                                          \
        if (ksize != 0) {                                                                                               \
            return hashmap_remove((HashMap *)n, &key);                                                                  \
        }                                                                                                               \
        void *kptr;                                                                                                     \
        memcpy(&kptr, &key, sizeof(void *));                                                                            \
        return hashmap_remove((HashMap *)n, kptr);                                                                      \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Remove a specific key-value pair (both must match) */                                                    \
    static inline LC_UNUSED int name##_remove_entry(name *n, K key, V val) {                                            \
        LC_MAP_DEBUG_NULL(n, #name "_remove_entry");                                                                    \
        if (ksize != 0 && vsize != 0) {                                                                                 \
            return hashmap_remove_entry((HashMap *)n, &key, &val);                                                      \
        }                                                                                                               \
        const void *kptr, *vptr;                                                                                        \
        if (ksize == 0) {                                                                                               \
            memcpy((void *)&kptr, &key, sizeof(void *));                                                                \
        } else {                                                                                                        \
            kptr = &key;                                                                                                \
        }                                                                                                               \
        if (vsize == 0) {                                                                                               \
            memcpy((void *)&vptr, &val, sizeof(void *));                                                                \
        } else {                                                                                                        \
            vptr = &val;                                                                                                \
        }                                                                                                               \
        return hashmap_remove_entry((HashMap *)n, kptr, vptr);                                                          \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get a value by key (panics if key not exist) */                                                          \
    static inline LC_UNUSED V name##_get(const name *n, K key) {                                                        \
        LC_MAP_DEBUG_NULL(n, #name "_get");                                                                             \
        if (ksize != 0) {                                                                                               \
            return *(V *)hashmap_get_mut((HashMap *)n, &key);                                                           \
        }                                                                                                               \
        void *kptr;                                                                                                     \
        memcpy(&kptr, &key, sizeof(void *));                                                                            \
        return *(V *)hashmap_get_mut((HashMap *)n, kptr);                                                               \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get a value by key with a default fallback */                                                            \
    static inline LC_UNUSED V name##_get_or_default(const name *n, K key, V default_val) {                              \
        LC_MAP_DEBUG_NULL(n, #name "_get_or_default");                                                                  \
        if (ksize != 0) {                                                                                               \
            return *(V *)hashmap_get_mut_or_default((HashMap *)n, &key, &default_val);                                  \
        }                                                                                                               \
        void *kptr;                                                                                                     \
        memcpy(&kptr, &key, sizeof(void *));                                                                            \
        const void *val = hashmap_get_mut_or_default((HashMap *)n, kptr, &default_val);                                 \
        return *(V *)val;                                                                                               \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get a read-only pointer to a value by key (returns NULL if not found) */                                 \
    static inline LC_UNUSED V const *name##_get_ptr(const name *n, K key) {                                             \
        LC_MAP_DEBUG_NULL(n, #name "_get_ptr");                                                                         \
        if (ksize != 0) {                                                                                               \
            return (V const *)hashmap_get((HashMap *)n, &key);                                                          \
        }                                                                                                               \
        void *kptr;                                                                                                     \
        memcpy(&kptr, &key, sizeof(void *));                                                                            \
        return (V const *)hashmap_get((HashMap *)n, kptr);                                                              \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get a mutable pointer to a value by key (returns NULL if not found) */                                   \
    static inline LC_UNUSED V *name##_get_mut(name *n, K key) {                                                         \
        LC_MAP_DEBUG_NULL(n, #name "_get_mut");                                                                         \
        if (ksize != 0) {                                                                                               \
            return (V *)hashmap_get_mut((HashMap *)n, &key);                                                            \
        }                                                                                                               \
        void *kptr;                                                                                                     \
        memcpy(&kptr, &key, sizeof(void *));                                                                            \
        return (V *)hashmap_get_mut((HashMap *)n, kptr);                                                                \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Check if a key exists in the hash map */                                                                 \
    static inline LC_UNUSED bool name##_contains(const name *n, K key) {                                                \
        LC_MAP_DEBUG_NULL(n, #name "_contains");                                                                        \
        if (ksize != 0) {                                                                                               \
            return hashmap_contains((HashMap *)n, &key);                                                                \
        }                                                                                                               \
        void *kptr;                                                                                                     \
        memcpy(&kptr, &key, sizeof(void *));                                                                            \
        return hashmap_contains((HashMap *)n, kptr);                                                                    \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Check if a specific key-value pair exists */                                                             \
    static inline LC_UNUSED bool name##_contains_entry(const name *n, K key, V val) {                                   \
        LC_MAP_DEBUG_NULL(n, #name "_contains_entry");                                                                  \
        if (ksize != 0 && vsize != 0) {                                                                                 \
            return hashmap_contains_entry((HashMap *)n, &key, &val);                                                    \
        }                                                                                                               \
        const void *kptr, *vptr;                                                                                        \
        if (ksize == 0) {                                                                                               \
            memcpy((void *)&kptr, &key, sizeof(void *));                                                                \
        } else {                                                                                                        \
            kptr = &key;                                                                                                \
        }                                                                                                               \
        if (vsize == 0) {                                                                                               \
            memcpy((void *)&vptr, &val, sizeof(void *));                                                                \
        } else {                                                                                                        \
            vptr = &val;                                                                                                \
        }                                                                                                               \
        return hashmap_contains_entry((HashMap *)n, kptr, vptr);                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Merge another hash map into this one (in-place) */                                                       \
    static inline LC_UNUSED int name##_merge(name *dst, const name *src) {                                              \
        LC_MAP_DEBUG_NULL_PAIR(dst, src, #name "_merge");                                                               \
        return hashmap_merge((HashMap *)dst, (HashMap *)src);                                                           \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Queries ===== */                                                                                           \
                                                                                                                        \
    /** @brief Get the number of key-value pairs in the hash map */                                                     \
    static inline LC_UNUSED size_t name##_len(const name *n) {                                                          \
        return n ? hashmap_len((HashMap *)n) : 0;                                                                       \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get the current bucket count of the hash map */                                                          \
    static inline LC_UNUSED size_t name##_capacity(const name *n) {                                                     \
        return n ? hashmap_capacity((HashMap *)n) : 0;                                                                  \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Check if the hash map is empty */                                                                        \
    static inline LC_UNUSED bool name##_is_empty(const name *n) {                                                       \
        return n ? hashmap_is_empty((HashMap *)n) : true;                                                               \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Compute a hash of the hash map's contents */                                                             \
    static inline LC_UNUSED size_t name##_hash(const name *n) {                                                         \
        return n ? hashmap_hash((HashMap *)n) : 0;                                                                      \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Modification ===== */                                                                                      \
                                                                                                                        \
    /** @brief Remove all key-value pairs from the hash map */                                                          \
    static inline LC_UNUSED void name##_clear(name *n) {                                                                \
        LC_MAP_DEBUG_NULL(n, #name "_clear");                                                                           \
        hashmap_clear((HashMap *)n);                                                                                    \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Reserve capacity for expected number of elements */                                                      \
    static inline LC_UNUSED int name##_reserve(name *n, size_t expected) {                                              \
        LC_MAP_DEBUG_NULL(n, #name "_reserve");                                                                         \
        return hashmap_reserve((HashMap *)n, expected);                                                                 \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Shrink bucket count to fit current load factor */                                                        \
    static inline LC_UNUSED int name##_shrink_to_fit(name *n) {                                                         \
        LC_MAP_DEBUG_NULL(n, #name "_shrink_to_fit");                                                                   \
        return hashmap_shrink_to_fit((HashMap *)n);                                                                     \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Configuration ===== */                                                                                     \
                                                                                                                        \
    /** @brief Set the key hash function (empty map only) */                                                            \
    static inline LC_UNUSED int name##_set_hasher(name *n, lc_Hasher hasher) {                                          \
        LC_MAP_DEBUG_NULL(n, #name "_set_hasher");                                                                      \
        return hashmap_set_hasher((HashMap *)n, hasher);                                                                \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Set key and value comparators (empty map only) */                                                        \
    static inline LC_UNUSED int name##_set_comparator(name *n, lc_Comparator kcmp, lc_Comparator vcmp) {                \
        LC_MAP_DEBUG_NULL(n, #name "_set_comparator");                                                                  \
        return hashmap_set_comparator((HashMap *)n, kcmp, vcmp);                                                        \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Copy & Comparison ===== */                                                                                 \
                                                                                                                        \
    /** @brief Create a deep copy of the hash map */                                                                    \
    static inline LC_UNUSED name *name##_clone(const name *n) {                                                         \
        LC_MAP_DEBUG_NULL(n, #name "_clone");                                                                           \
        return (name *)hashmap_clone((HashMap *)n);                                                                     \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Check if two hash maps are equal */                                                                      \
    static inline LC_UNUSED bool name##_equals(const name *a, const name *b) {                                          \
        LC_MAP_DEBUG_NULL_PAIR(a, b, #name "_equals");                                                                  \
        return hashmap_equals((HashMap *)a, (HashMap *)b);                                                              \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Swap contents of two typed hash maps */                                                                  \
    static inline LC_UNUSED void name##_swap(name *a, name *b) {                                                        \
        LC_MAP_DEBUG_NULL_PAIR(a, b, #name "_swap");                                                                    \
        name tmp = *a;                                                                                                  \
        *a = *b;                                                                                                        \
        *b = tmp;                                                                                                       \
    }                                                                                                                   \
                                                                                                                        \
    /* ===== Export & Iteration ===== */                                                                                \
                                                                                                                        \
    /** @brief Export all keys to a flat Array snapshot */                                                              \
    static inline LC_UNUSED Array *name##_keys(const name *n) {                                                         \
        LC_MAP_DEBUG_NULL(n, #name "_keys");                                                                            \
        return hashmap_keys((HashMap *)n);                                                                              \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Export all values to a flat Array snapshot */                                                            \
    static inline LC_UNUSED Array *name##_values(const name *n) {                                                       \
        LC_MAP_DEBUG_NULL(n, #name "_values");                                                                          \
        return hashmap_values((HashMap *)n);                                                                            \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Create a forward iterator over the hash map */                                                           \
    static inline LC_UNUSED name##Iterator name##_iter(const name *n) {                                                 \
        LC_MAP_DEBUG_NULL(n, #name "_iter");                                                                            \
        return (name##Iterator){ .base = hashmap_iter((HashMap *)n) };                                                  \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get the underlying iterator */                                                                           \
    static inline LC_UNUSED Iterator name##_as_iterator(name##Iterator it) {                                            \
        return it.base;                                                                                                 \
    }                                                                                                                   \
                                                                                                                        \
    /** @brief Get the next key-value pair */                                                                           \
    static inline LC_UNUSED bool name##_next(name##Iterator *it, K *key_out, V *val_out) {                              \
        LC_MAP_DEBUG_NULL(it, #name "_next");                                                                           \
        LC_MAP_DEBUG_NULL(key_out, #name "_next");                                                                      \
        LC_MAP_DEBUG_NULL(val_out, #name "_next");                                                                      \
        const void *entry = iter_next(&it->base);                                                                       \
        if (!entry) return false;                                                                                       \
        HashMap *map = (HashMap *)it->base.container;                                                                   \
        const void *key_ptr = hashmap_entry_key(map, entry);                                                            \
        const void *val_ptr = hashmap_entry_value(map, entry);                                                          \
        if (ksize == 0) {                                                                                               \
            *(const void **)key_out = key_ptr;                                                                          \
        } else {                                                                                                        \
            memcpy(key_out, key_ptr, sizeof(K));                                                                        \
        }                                                                                                               \
        if (vsize == 0) {                                                                                               \
            *(const void **)val_out = val_ptr;                                                                          \
        } else {                                                                                                        \
            memcpy(val_out, val_ptr, sizeof(V));                                                                        \
        }                                                                                                               \
        return true;                                                                                                    \
    }

/** @brief Declare a type-safe hashmap
 *
 * For string mode (size == 0), libcontain manages memory:
 * - strdup on insert
 * - free on destroy
 *
 * For fixed-size types (size > 0), ownership is ignored.
 *
 * @param K      Key type
 * @param V      Value type
 * @param ksize  Size of K in bytes (0 for string mode)
 * @param vsize  Size of V in bytes (0 for string mode)
 * @param name   Name for the generated type
 */
#define DECL_HASHMAP_TYPE(K, V, ksize, vsize, name) \
    HASHMAP_TYPE_IMPL(K, V, ksize, vsize, name, 1, 1)

/** @brief Declare a type-safe hashmap with explicit ownership control
 *
 * @param K      Key type
 * @param V      Value type
 * @param ksize  Size of K in bytes (0 for string mode)
 * @param vsize  Size of V in bytes (0 for string mode)
 * @param name   Name for the generated type
 * @param kowned Whether the key is owned by the map (1 for owned, 0 for borrowed)
 * @param vowned Whether the value is owned by the map (1 for owned, 0 for borrowed)
 */
#define DECL_HASHMAP_REF_TYPE(K, V, ksize, vsize, name, kowned, vowned) \
    HASHMAP_TYPE_IMPL(K, V, ksize, vsize, name, kowned, vowned)

#endif /* CONTAIN_TYPED_HASHMAP_PDR_H */
