/** @file typed.h
 * @brief All type-safe wrappers in one include
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * This header aggregates all type-safe wrapper headers (vector, deque,
 * linkedlist, hashset, hashmap) into a single include. Use this convenience
 * header when you need multiple typed containers in the same project.
 *
 * @section example Usage Example
 * @code
 *   // Declare multiple type-safe containers
 *   #define DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
 *   #define DECL_DEQUE_TYPE(double, sizeof(double), DoubleDeque)
 *   #define DECL_LINKEDLIST_TYPE(const char*, 0, StringList)
 *   #define DECL_HASHSET_TYPE(const char*, 0, StringSet)
 *   #define DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)
 *   #include <contain/typed.h>
 *
 *   int main() {
 *       IntVector *vec = IntVector_create();
 *       DoubleDeque *dq = DoubleDeque_create();
 *       StringList *list = StringList_create();
 *       StringSet *set = StringSet_create();
 *       StrIntMap *map = StrIntMap_create();
 *
 *       IntVector_push(vec, 42);
 *       DoubleDeque_push_back(dq, 3.14);
 *       StringList_push_back(list, "hello");
 *       StringSet_insert(set, "world");
 *       StrIntMap_insert(map, "answer", 42);
 *
 *       IntVector_destroy(vec);
 *       DoubleDeque_destroy(dq);
 *       StringList_destroy(list);
 *       StringSet_destroy(set);
 *       StrIntMap_destroy(map);
 *       return 0;
 *   }
 * @endcode
 *
 * @section notes Notes
 * - All `DECL_*_TYPE` macros must be defined **before** including this header
 * - Each macro generates a complete type-safe API for that container
 * - Multiple declarations are allowed in any order
 * - The generated types are independent and can be used together
 *
 * @warning Forgetting to define a required `DECL_*_TYPE` macro will result in
 *          compiler errors for the corresponding container functions.
 *
 * @see typed/vector.h for VECTOR type-safe API
 * @see typed/deque.h for DEQUE type-safe API
 * @see typed/linkedlist.h for LINKEDLIST type-safe API
 * @see typed/hashset.h for HASHSET type-safe API
 * @see typed/hashmap.h for HASHMAP type-safe API
 */

#ifndef CONTAIN_TYPED_PDR_H
#define CONTAIN_TYPED_PDR_H

/**
 * @defgroup typed_include Included Headers
 * @brief Type-safe wrapper headers for all container types
 * @{
 */

#include <contain/typed/vector.h>      /**< Type-safe Vector wrappers */
#include <contain/typed/deque.h>       /**< Type-safe Deque wrappers */
#include <contain/typed/linkedlist.h>  /**< Type-safe LinkedList wrappers */
#include <contain/typed/hashset.h>     /**< Type-safe HashSet wrappers */
#include <contain/typed/hashmap.h>     /**< Type-safe HashMap wrappers */

/** @} */

#endif /* CONTAIN_TYPED_PDR_H */