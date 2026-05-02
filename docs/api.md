# libcontain API Reference

A high-performance, type-generic container library for C with a unified interface for vectors, deques, linked lists, hash sets, and hash maps.

## Table of Contents

- [Core Types](#core-types)
  - [Container](#container)
  - [Iterator](#iterator)
  - [Array](#array)
  - [Error Codes](#error-codes)
- [Vector](#vector)
- [Deque](#deque)
- [LinkedList](#linkedlist)
- [HashSet](#hashset)
- [HashMap](#hashmap)
- [Iterator Pipelines](#iterator-pipelines)
- [Chainer](#chainer)
- [Chainer2](#chainer2)
- [Type-Safe Wrappers](#type-safe-wrappers)
- [Utility Functions](#utility-functions)
- [Build Configuration](#build-configuration)
- [Thread Safety](#thread-safety)
- [Memory Management](#memory-management)

---

## Core Types

### Container

Common header embedded in all container types.

```c
typedef struct Container {
    void *items;                    /* Pointer to backing storage */
    size_t len;                     /* Number of elements stored */
    size_t capacity;                /* Maximum elements before reallocation */
    const ContainerVTable *ops;     /* Virtual function table */
} Container;
```

### Iterator

Cursor for traversing containers.

```c
typedef struct Iterator {
    const Container *container;     /* Container being iterated */
    void *state;                    /* Opaque cursor state */
    size_t pos;                     /* Current position index */
    IterDirection direction;        /* ITER_FORWARD or ITER_REVERSE */
    const IteratorVTable *ops;      /* Virtual function table */
} Iterator;
```

### Array

Flat snapshot of container elements.

```c
typedef struct Array {
    uint8_t *items;     /* Pointer to element data */
    size_t stride;      /* Bytes between elements (0 for strings) */
    size_t len;         /* Number of elements */
} Array;
```

### Error Codes

Functions that perform operations (insert, remove, modify) return an `int` status code where `LC_OK` (0) indicates success. Negative values indicate errors.

| Code | Value | Description |
|------|------:|-------------|
| `LC_OK` | 0 | Success |
| `LC_ERROR` | -1 | Generic error |
| `LC_EINVAL` | -2 | Invalid argument |
| `LC_ENOMEM` | -3 | Out of memory |
| `LC_EBOUNDS` | -4 | Index out of bounds |
| `LC_EBUSY` | -5 | Container not empty |
| `LC_ENOTFOUND` | -6 | Element not found |
| `LC_ETYPE` | -7 | Type mismatch |
| `LC_EOVERFLOW` | -8 | Overflow occurred |
| `LC_EFULL` | -9 | Container full |
| `LC_ENOTSUP` | -10 | Operation not supported |

Use `lc_error_str(err)` to get a human-readable description.

> **Note:** Not all functions return error codes. Functions that return pointers (e.g., `vector_at`, `vector_front`) return `NULL` on error. Functions that return `bool` or `size_t` return appropriate sentinel values (e.g., `false` or `VEC_NPOS`/`SIZE_MAX` for "not found").

---

## Vector

Dynamic array with automatic resizing.

### Creation

| Function | Description |
|----------|-------------|
| `Vector *vector_create(size_t item_size)` | Create new vector |
| `Vector *vector_create_with_capacity(size_t item_size, size_t cap)` | Create with initial capacity |
| `Vector *vector_create_with_comparator(size_t item_size, lc_Comparator cmp)` | Create with custom comparator |
| `Vector *vector_create_aligned(size_t item_size, size_t align)` | Create with aligned elements |
| `Vector *vector_str(void)` | Create string vector |
| `void vector_destroy(Vector *vec)` | Destroy vector |

### Insertion

| Function | Description |
|----------|-------------|
| `int vector_push(Vector *vec, const void *item)` | Append element |
| `int vector_insert(Vector *vec, size_t pos, const void *item)` | Insert at position |
| `int vector_insert_range(Vector *dst, size_t pos, const Vector *src, size_t from, size_t to)` | Insert range |
| `int vector_append(Vector *dst, const Vector *src)` | Append all elements |

### Access

| Function | Description |
|----------|-------------|
| `const void *vector_at(const Vector *vec, size_t pos)` | Get element (read-only) |
| `void *vector_at_mut(const Vector *vec, size_t pos)` | Get element (mutable) |
| `const void *vector_front(const Vector *vec)` | Get first element |
| `const void *vector_back(const Vector *vec)` | Get last element |
| `int vector_set(Vector *vec, size_t pos, const void *item)` | Set element |

### Removal

| Function | Description |
|----------|-------------|
| `int vector_pop(Vector *vec)` | Remove last element |
| `int vector_remove(Vector *vec, size_t pos)` | Remove at position |
| `int vector_remove_range(Vector *vec, size_t from, size_t to)` | Remove range |
| `int vector_clear(Vector *vec)` | Remove all elements |

### Queries

| Function | Description |
|----------|-------------|
| `size_t vector_len(const Vector *vec)` | Number of elements |
| `size_t vector_capacity(const Vector *vec)` | Current capacity |
| `bool vector_is_empty(const Vector *vec)` | Check if empty |
| `size_t vector_find(const Vector *vec, const void *item)` | Find first occurrence |
| `size_t vector_rfind(const Vector *vec, const void *item)` | Find last occurrence |
| `bool vector_contains(const Vector *vec, const void *item)` | Check existence |
| `bool vector_equals(const Vector *a, const Vector *b)` | Compare vectors |
| `size_t vector_hash(const Vector *vec)` | Compute hash |

### Capacity

| Function | Description |
|----------|-------------|
| `int vector_reserve(Vector *vec, size_t cap)` | Reserve capacity |
| `int vector_shrink_to_fit(Vector *vec)` | Reduce capacity to fit length |
| `int vector_resize(Vector *vec, size_t new_len)` | Change length |

### Operations

| Function | Description |
|----------|-------------|
| `int vector_sort(Vector *vec, lc_Comparator cmp)` | Sort in place |
| `int vector_reverse_inplace(Vector *vec)` | Reverse in place |
| `int vector_unique(Vector *vec)` | Remove adjacent duplicates |

### Copy

| Function | Description |
|----------|-------------|
| `Vector *vector_clone(const Vector *vec)` | Deep copy |
| `Vector *vector_slice(const Vector *vec, size_t from, size_t to)` | Extract slice |
| `Vector *vector_reverse(const Vector *vec)` | New reversed vector |
| `Vector *vector_instance(const Vector *vec)` | New empty of same type |

### Iteration

| Function | Description |
|----------|-------------|
| `Iterator vector_iter(const Vector *vec)` | Forward iterator |
| `Iterator vector_iter_reversed(const Vector *vec)` | Reverse iterator |

### Conversion

| Function | Description |
|----------|-------------|
| `Array *vector_to_array(const Vector *vec)` | Export to flat array |

---

## Deque

Double-ended queue with O(1) push/pop at both ends.

### Creation

| Function | Description |
|----------|-------------|
| `Deque *deque_create(size_t item_size)` | Create new deque |
| `Deque *deque_create_with_capacity(size_t item_size, size_t cap)` | Create with initial capacity |
| `Deque *deque_create_with_comparator(size_t item_size, lc_Comparator cmp)` | Create with custom comparator |
| `Deque *deque_create_aligned(size_t item_size, size_t align)` | Create with aligned elements |
| `Deque *deque_str(void)` | Create string deque |
| `void deque_destroy(Deque *deq)` | Destroy deque |

### Insertion

| Function | Description |
|----------|-------------|
| `int deque_push_back(Deque *deq, const void *item)` | Append to back |
| `int deque_push_front(Deque *deq, const void *item)` | Prepend to front |
| `int deque_insert(Deque *deq, size_t pos, const void *item)` | Insert at position |
| `int deque_insert_range(Deque *dst, size_t pos, const Deque *src, size_t from, size_t to)` | Insert range |
| `int deque_append(Deque *dst, const Deque *src)` | Append all elements |

### Access

| Function | Description |
|----------|-------------|
| `const void *deque_at(const Deque *deq, size_t pos)` | Get element (read-only) |
| `void *deque_at_mut(const Deque *deq, size_t pos)` | Get element (mutable) |
| `const void *deque_front(const Deque *deq)` | Get first element |
| `const void *deque_back(const Deque *deq)` | Get last element |
| `int deque_set(Deque *deq, size_t pos, const void *item)` | Set element |

### Removal

| Function | Description |
|----------|-------------|
| `int deque_pop_back(Deque *deq)` | Remove last element |
| `int deque_pop_front(Deque *deq)` | Remove first element |
| `int deque_remove(Deque *deq, size_t pos)` | Remove at position |
| `int deque_remove_range(Deque *deq, size_t from, size_t to)` | Remove range |
| `int deque_clear(Deque *deq)` | Remove all elements |

### Queries

| Function | Description |
|----------|-------------|
| `size_t deque_len(const Deque *deq)` | Number of elements |
| `size_t deque_capacity(const Deque *deq)` | Current capacity |
| `bool deque_is_empty(const Deque *deq)` | Check if empty |
| `size_t deque_find(const Deque *deq, const void *item)` | Find first occurrence |
| `size_t deque_rfind(const Deque *deq, const void *item)` | Find last occurrence |
| `bool deque_contains(const Deque *deq, const void *item)` | Check existence |
| `bool deque_equals(const Deque *a, const Deque *b)` | Compare deques |
| `size_t deque_hash(const Deque *deq)` | Compute hash |

### Capacity

| Function | Description |
|----------|-------------|
| `int deque_reserve(Deque *deq, size_t cap)` | Reserve capacity |
| `int deque_shrink_to_fit(Deque *deq)` | Reduce capacity to fit length |
| `int deque_resize(Deque *deq, size_t new_len)` | Change length |

### Operations

| Function | Description |
|----------|-------------|
| `int deque_sort(Deque *deq, lc_Comparator cmp)` | Sort in place |
| `int deque_reverse_inplace(Deque *deq)` | Reverse in place |
| `int deque_unique(Deque *deq)` | Remove adjacent duplicates |

### Copy

| Function | Description |
|----------|-------------|
| `Deque *deque_clone(const Deque *deq)` | Deep copy |
| `Deque *deque_slice(const Deque *deq, size_t from, size_t to)` | Extract slice |
| `Deque *deque_reverse(const Deque *deq)` | New reversed deque |
| `Deque *deque_instance(const Deque *deq)` | New empty of same type |

### Iteration

| Function | Description |
|----------|-------------|
| `Iterator deque_iter(const Deque *deq)` | Forward iterator |
| `Iterator deque_iter_reversed(const Deque *deq)` | Reverse iterator |

### Conversion

| Function | Description |
|----------|-------------|
| `Array *deque_to_array(const Deque *deq)` | Export to flat array |

---

## LinkedList

Doubly-linked list with O(1) insertion/removal at known positions.

### Creation

| Function | Description |
|----------|-------------|
| `LinkedList *linkedlist_create(size_t item_size)` | Create new list |
| `LinkedList *linkedlist_create_with_comparator(size_t item_size, lc_Comparator cmp)` | Create with custom comparator |
| `LinkedList *linkedlist_create_aligned(size_t item_size, size_t align)` | Create with aligned elements |
| `LinkedList *linkedlist_str(void)` | Create string list |
| `void linkedlist_destroy(LinkedList *list)` | Destroy list |

### Insertion

| Function | Description |
|----------|-------------|
| `int linkedlist_push_back(LinkedList *list, const void *item)` | Append to back |
| `int linkedlist_push_front(LinkedList *list, const void *item)` | Prepend to front |
| `int linkedlist_insert(LinkedList *list, size_t pos, const void *item)` | Insert at position |
| `int linkedlist_insert_range(LinkedList *dst, size_t pos, const LinkedList *src, size_t from, size_t to)` | Insert range |
| `int linkedlist_append(LinkedList *dst, const LinkedList *src)` | Append all elements |

### Access

| Function | Description |
|----------|-------------|
| `const void *linkedlist_at(const LinkedList *list, size_t pos)` | Get element (read-only) |
| `void *linkedlist_at_mut(const LinkedList *list, size_t pos)` | Get element (mutable) |
| `const void *linkedlist_front(const LinkedList *list)` | Get first element |
| `const void *linkedlist_back(const LinkedList *list)` | Get last element |
| `int linkedlist_set(LinkedList *list, size_t pos, const void *item)` | Set element |

### Removal

| Function | Description |
|----------|-------------|
| `int linkedlist_pop_back(LinkedList *list)` | Remove last element |
| `int linkedlist_pop_front(LinkedList *list)` | Remove first element |
| `int linkedlist_remove(LinkedList *list, size_t pos)` | Remove at position |
| `int linkedlist_remove_range(LinkedList *list, size_t from, size_t to)` | Remove range |
| `int linkedlist_clear(LinkedList *list)` | Remove all elements |

### Queries

| Function | Description |
|----------|-------------|
| `size_t linkedlist_len(const LinkedList *list)` | Number of elements |
| `bool linkedlist_is_empty(const LinkedList *list)` | Check if empty |
| `size_t linkedlist_find(const LinkedList *list, const void *item)` | Find first occurrence |
| `size_t linkedlist_rfind(const LinkedList *list, const void *item)` | Find last occurrence |
| `bool linkedlist_contains(const LinkedList *list, const void *item)` | Check existence |
| `bool linkedlist_equals(const LinkedList *a, const LinkedList *b)` | Compare lists |
| `size_t linkedlist_hash(const LinkedList *list)` | Compute hash |

### Operations

| Function | Description |
|----------|-------------|
| `int linkedlist_sort(LinkedList *list, lc_Comparator cmp)` | Sort in place |
| `int linkedlist_reverse_inplace(LinkedList *list)` | Reverse in place |
| `int linkedlist_unique(LinkedList *list)` | Remove adjacent duplicates |

### Copy

| Function | Description |
|----------|-------------|
| `LinkedList *linkedlist_clone(const LinkedList *list)` | Deep copy |
| `LinkedList *linkedlist_sublist(const LinkedList *list, size_t from, size_t to)` | Extract sublist |
| `LinkedList *linkedlist_reverse(const LinkedList *list)` | New reversed list |
| `LinkedList *linkedlist_instance(const LinkedList *list)` | New empty of same type |

### Iteration

| Function | Description |
|----------|-------------|
| `Iterator linkedlist_iter(const LinkedList *list)` | Forward iterator |
| `Iterator linkedlist_iter_reversed(const LinkedList *list)` | Reverse iterator |

### Conversion

| Function | Description |
|----------|-------------|
| `Array *linkedlist_to_array(const LinkedList *list)` | Export to flat array |

---

## HashSet

Hash set with separate chaining and automatic rehashing.

### Creation

| Function | Description |
|----------|-------------|
| `HashSet *hashset_create(size_t item_size)` | Create new set |
| `HashSet *hashset_create_with_capacity(size_t item_size, size_t cap)` | Create with initial capacity |
| `HashSet *hashset_create_with_hasher(size_t item_size, lc_Hasher hasher)` | Create with custom hash |
| `HashSet *hashset_create_with_comparator(size_t item_size, lc_Comparator cmp)` | Create with custom comparator |
| `HashSet *hashset_create_aligned(size_t item_size, size_t align)` | Create with aligned elements |
| `HashSet *hashset_str(void)` | Create string set |
| `void hashset_destroy(HashSet *set)` | Destroy set |

### Operations

| Function | Description |
|----------|-------------|
| `int hashset_insert(HashSet *set, const void *item)` | Insert element |
| `int hashset_remove(HashSet *set, const void *item)` | Remove element |
| `bool hashset_contains(const HashSet *set, const void *item)` | Check existence |
| `int hashset_merge(HashSet *dst, const HashSet *src)` | Merge another set |
| `int hashset_clear(HashSet *set)` | Remove all elements |

### Queries

| Function | Description |
|----------|-------------|
| `size_t hashset_len(const HashSet *set)` | Number of elements |
| `size_t hashset_capacity(const HashSet *set)` | Current bucket count |
| `bool hashset_is_empty(const HashSet *set)` | Check if empty |
| `bool hashset_subset(const HashSet *a, const HashSet *b)` | Check if A ⊆ B |
| `bool hashset_equals(const HashSet *a, const HashSet *b)` | Compare sets |
| `size_t hashset_hash(const HashSet *set)` | Compute hash |

### Set Operations

| Function | Description |
|----------|-------------|
| `HashSet *hashset_union(const HashSet *a, const HashSet *b)` | New set with union |
| `HashSet *hashset_intersection(const HashSet *a, const HashSet *b)` | New set with intersection |
| `HashSet *hashset_difference(const HashSet *a, const HashSet *b)` | New set with A \ B |

### Capacity

| Function | Description |
|----------|-------------|
| `int hashset_reserve(HashSet *set, size_t expected)` | Reserve capacity |
| `int hashset_shrink_to_fit(HashSet *set)` | Reduce bucket count |

### Copy

| Function | Description |
|----------|-------------|
| `HashSet *hashset_clone(const HashSet *set)` | Deep copy |
| `HashSet *hashset_instance(const HashSet *set)` | New empty of same type |

### Iteration

| Function | Description |
|----------|-------------|
| `Iterator hashset_iter(const HashSet *set)` | Forward iterator |

### Conversion

| Function | Description |
|----------|-------------|
| `Array *hashset_to_array(const HashSet *set)` | Export to flat array |

---

## HashMap

Hash map with separate chaining and automatic rehashing.

### Creation

| Function | Description |
|----------|-------------|
| `HashMap *hashmap_create(size_t key_size, size_t val_size)` | Create new map |
| `HashMap *hashmap_create_with_capacity(size_t key_size, size_t val_size, size_t cap)` | Create with initial capacity |
| `HashMap *hashmap_create_with_hasher(size_t key_size, size_t val_size, lc_Hasher hasher)` | Create with custom hash |
| `HashMap *hashmap_create_with_comparator(size_t key_size, size_t val_size, lc_Comparator kcmp, lc_Comparator vcmp)` | Create with custom comparators |
| `HashMap *hashmap_create_aligned(size_t key_size, size_t val_size, size_t key_align, size_t val_align)` | Create with aligned keys/values |
| `void hashmap_destroy(HashMap *map)` | Destroy map |

### Operations

| Function | Description |
|----------|-------------|
| `int hashmap_insert(HashMap *map, const void *key, const void *val)` | Insert or update |
| `int hashmap_remove(HashMap *map, const void *key)` | Remove by key |
| `int hashmap_remove_entry(HashMap *map, const void *key, const void *val)` | Remove specific entry |
| `const void *hashmap_get(const HashMap *map, const void *key)` | Get value (read-only) |
| `void *hashmap_get_mut(const HashMap *map, const void *key)` | Get value (mutable) |
| `const void *hashmap_get_or_default(const HashMap *map, const void *key, const void *dval)` | Get with default |
| `bool hashmap_contains(const HashMap *map, const void *key)` | Check key existence |
| `bool hashmap_contains_entry(const HashMap *map, const void *key, const void *val)` | Check entry existence |
| `int hashmap_merge(HashMap *dst, const HashMap *src)` | Merge another map |
| `int hashmap_clear(HashMap *map)` | Remove all entries |

### Queries

| Function | Description |
|----------|-------------|
| `size_t hashmap_len(const HashMap *map)` | Number of entries |
| `size_t hashmap_capacity(const HashMap *map)` | Current bucket count |
| `bool hashmap_is_empty(const HashMap *map)` | Check if empty |
| `bool hashmap_equals(const HashMap *a, const HashMap *b)` | Compare maps |
| `size_t hashmap_hash(const HashMap *map)` | Compute hash |

### Iteration

| Function | Description |
|----------|-------------|
| `Iterator hashmap_iter(const HashMap *map)` | Forward iterator |
| `const void *hashmap_entry_key(const HashMap *map, const void *entry)` | Get key from entry |
| `const void *hashmap_entry_value(const HashMap *map, const void *entry)` | Get value from entry |

### Capacity

| Function | Description |
|----------|-------------|
| `int hashmap_reserve(HashMap *map, size_t expected)` | Reserve capacity |
| `int hashmap_shrink_to_fit(HashMap *map)` | Reduce bucket count |

### Copy

| Function | Description |
|----------|-------------|
| `HashMap *hashmap_clone(const HashMap *map)` | Deep copy |
| `HashMap *hashmap_instance(const HashMap *map)` | New empty of same type |

### Export

| Function | Description |
|----------|-------------|
| `Array *hashmap_keys(const HashMap *map)` | Export all keys |
| `Array *hashmap_values(const HashMap *map)` | Export all values |

---

## Iterator Pipelines

Lazy evaluation with decorator pattern. For zero-overhead fused execution, use [Chainer](#chainer).

### Creation

| Function | Description |
|----------|-------------|
| `Iterator Iter(const Container *c)` | Stack-allocated forward iterator |
| `Iterator IterReverse(const Container *c)` | Stack-allocated reverse iterator |
| `Iterator *IntoIter(Container *c)` | Heap-allocated forward iterator (transferable ownership) |
| `Iterator *IntoIterReverse(Container *c)` | Heap-allocated reverse iterator (transferable ownership) |
| `void iter_destroy(Iterator *it)` | Destroy iterator |

### Decorators

| Function | Description |
|----------|-------------|
| `Iterator *iter_filter(Iterator *inner, bool (*pred)(const Container *, const void *))` | Filter elements |
| `Iterator *iter_map(Iterator *inner, void *(*map)(const Container *, const void *, void *), size_t stride)` | Transform elements |
| `Iterator *iter_skip(Iterator *inner, size_t n)` | Skip first N elements |
| `Iterator *iter_take(Iterator *inner, size_t n)` | Limit to N elements |
| `Iterator *iter_flatten(Iterator *outer)` | Flatten sequence of containers |
| `Iterator *iter_zip(Iterator *left, Iterator *right, void *(*merge)(...), size_t stride)` | Pair elements |
| `Iterator *iter_peekable(Iterator *inner)` | Make peekable |

### Peek

| Function | Description |
|----------|-------------|
| `const void *iter_peek(Iterator *it)` | Look at next element without consuming |

### Terminal Operations

| Function | Description |
|----------|-------------|
| `size_t iter_count(Iterator *it)` | Count remaining elements |
| `bool iter_any(Iterator *it, bool (*pred)(const Container *, const void *))` | Check if any match |
| `bool iter_all(Iterator *it, bool (*pred)(const Container *, const void *))` | Check if all match |
| `void iter_for_each(Iterator *it, void (*f)(const Container *, const void *))` | Apply function |
| `const void *iter_find(Iterator *it, bool (*pred)(const Container *, const void *))` | Find first match |
| `void *iter_fold(Iterator *it, void *acc, void *(*f)(const Container *, const void *, void *))` | Fold/accumulate |
| `Container *iter_collect(Iterator *it)` | Collect into new container |
| `size_t iter_collect_in(Iterator *it, Container *dst)` | Insert into existing container |
| `void iter_drop(Iterator *it, size_t n)` | Discard N elements (non-consuming) |

---

## Chainer

Fused pipeline execution with zero per-element overhead. Chainer combines multiple pipeline stages into a single loop, eliminating the per-stage function call overhead of iterator pipelines.

> See `chainer.md` for detailed documentation.

### Creation

| Function | Description |
|----------|-------------|
| `Chainer Chain(const Container *base)` | Create chainer from container (stack-allocated) |
| `void chain_bind(Chainer *c, const Container *new_base)` | Rebind to a new source container |
| `void chain_destroy(Chainer *c)` | Destroy chainer and free resources |

### Pipeline Stages

| Function | Description |
|----------|-------------|
| `Chainer *chain_skip(Chainer *c, uint32_t n)` | Discard first n elements |
| `Chainer *chain_filter(Chainer *c, bool (*pred)(const Container *, const void *))` | Keep elements where pred returns true |
| `Chainer *chain_map(Chainer *c, void *(*fn)(const Container *, const void *, void *), size_t stride)` | Transform each element |
| `Chainer *chain_take(Chainer *c, uint32_t n)` | Limit to at most n elements |

### Terminal Operations

| Function | Description |
|----------|-------------|
| `size_t chain_count(Chainer *c)` | Count surviving elements |
| `bool chain_any(Chainer *c, bool (*pred)(const Container *, const void *))` | Check if any element matches |
| `bool chain_all(Chainer *c, bool (*pred)(const Container *, const void *))` | Check if all elements match |
| `const void *chain_find(Chainer *c, bool (*pred)(const Container *, const void *))` | Find first matching element |
| `const void *chain_first(Chainer *c)` | Get first element (NULL if empty) |
| `void *chain_fold(Chainer *c, void *acc, void *(*fn)(const Container *, const void *, void *))` | Reduce to single value |
| `void chain_for_each(Chainer *c, void (*fn)(const Container *, const void *))` | Apply function to each element |
| `Container *chain_collect(Chainer *c)` | Create new container of same type |
| `size_t chain_collect_in(Chainer *c, Container *dst)` | Insert into existing container |

---

## Chainer2

Extensible lazy fused pipeline with generator support (flatten, zip) and custom chain types. API-compatible with Chainer — simply replace `#include <contain/chainer.h>` with `#include <contain/chainer2.h>`.

> See `chainer2.md` for detailed documentation.

### New Features vs. Chainer

| Feature | Chainer | Chainer2 |
|---------|:-------:|:--------:|
| Filter / Map / Skip / Take | ✅ | ✅ |
| Flatten | ❌ | ✅ |
| Zip | ❌ | ✅ |
| Custom chain types | ❌ | ✅ |

### Additional Pipeline Stages

| Function | Description |
|----------|-------------|
| `Chainer *chain_flatten(Chainer *c)` | Flatten container of containers |
| `Chainer *chain_zip(Chainer *c, const Container *other, void *(*merge)(...), size_t stride)` | Pair elements from two containers |

---

## Type-Safe Wrappers

### Declaration Macros

| Container | Macro |
|-----------|-------|
| Vector | `DECL_VECTOR_TYPE(T, size, name)` |
| Deque | `DECL_DEQUE_TYPE(T, size, name)` |
| LinkedList | `DECL_LINKEDLIST_TYPE(T, size, name)` |
| HashSet | `DECL_HASHSET_TYPE(T, size, name)` |
| HashMap | `DECL_HASHMAP_TYPE(K, V, ksize, vsize, name)` |

### Example

```c
/* Declare typed containers */
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

/* Use them */
IntVector *vec = IntVector_create();
IntVector_push(vec, 42);

StrIntMap *map = StrIntMap_create();
StrIntMap_insert(map, "key", 100);
int val = StrIntMap_get(map, "key");

IntVector_destroy(vec);
StrIntMap_destroy(map);
```

---

## Utility Functions

### Hashing

| Function | Description |
|----------|-------------|
| `size_t lc_hash_djb2(const void *key, size_t len)` | DJB2 hash |
| `size_t lc_hash_fnv_1a(const void *key, size_t len)` | FNV-1a hash |
| `size_t lc_hash_mix(size_t h)` | Mix hash for distribution |

### Comparison

| Function | Description |
|----------|-------------|
| `int lc_compare_int(const void *a, const void *b)` | Integer comparator |
| `int lc_compare_double(const void *a, const void *b)` | Double comparator |
| `int lc_compare_str(const void *a, const void *b)` | String comparator (`char**`) |
| `int lc_compare_str_direct(const void *a, const void *b)` | String comparator (`char*`) |

### Bit Operations

| Function | Description |
|----------|-------------|
| `size_t lc_bit_ceil(size_t x)` | Round up to power of two |
| `size_t lc_bit_floor(size_t x)` | Round down to power of two |
| `bool lc_is_power_of_two(size_t x)` | Check if power of two |

### Error Reporting

| Function | Description |
|----------|-------------|
| `const char *lc_error_str(lc_Error err)` | Get human-readable error string |

---

## Build Configuration

### Integration Options

libcontain supports three integration methods:

| Method | Description |
|--------|-------------|
| Header-Only | Define `*_IMPLEMENTATION` in one `.c` file (STB style) |
| Separate Compilation | Compile source files and link |
| Build System | Use Make or CMake to build and install |

### Header-Only (STB Style)

For all containers at once:
```c
#define LIBCONTAIN_IMPLEMENTATION
#include <contain/contain.h>
```

Or individual containers:
```c
#define VECTOR_IMPLEMENTATION
#define DEQUE_IMPLEMENTATION
#define LINKEDLIST_IMPLEMENTATION
#define HASHSET_IMPLEMENTATION
#define HASHMAP_IMPLEMENTATION
#include <contain/vector.h>
#include <contain/deque.h>
#include <contain/linkedlist.h>
#include <contain/hashset.h>
#include <contain/hashmap.h>
```

### Separate Compilation

```bash
gcc -c src/vector.c src/deque.c src/linkedlist.c src/hashset.c src/hashmap.c
gcc main.c vector.o deque.o linkedlist.o hashset.o hashmap.o -o program
```

### Using Make

```bash
make              # Build library (release)
make debug        # Build with debug flags
make test         # Build and run tests
make tour         # Build tour demo
sudo make install # Install system-wide
```

### Using CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # Debug build
cmake --build build
ctest --test-dir build                    # Run tests
sudo cmake --install build                # Install
```

In your CMake project:
```cmake
find_package(libcontain REQUIRED)
target_link_libraries(your_target libcontain::libcontain)
```

### Compiler Flags

```bash
# Release build
gcc -O2 -Iinclude your_program.c -o your_program

# Debug build (with bounds checking)
gcc -O0 -g -DCONTAINER_DEBUG -Iinclude your_program.c -o your_program_debug

# Minimal binary size
gcc -Os -flto -ffunction-sections -fdata-sections -Wl,--gc-sections -Iinclude your_program.c -o your_program
```

### Debug Mode

Enable debug mode for bounds checking, NULL checks, and abort with error messages:

```c
#define CONTAINER_DEBUG
#include <contain/container.h>
```

In debug mode, functions abort with clear error messages:

```
libcontain panic: IntVector_at(10) - index 10 >= length 5
```

---

## Thread Safety

libcontain is **not thread-safe**. Concurrent reads with one writer are safe only if no reallocation occurs. External synchronization is required for concurrent writes.

---

## Memory Management

- Containers take ownership of inserted items (copied via `memcpy` or `strdup`)
- String mode automatically manages `strdup`/`free`
- The caller owns the container and must destroy it with the corresponding `_destroy` function
- Use `_instance()` to create a new empty container of the same type
- Use `_clone()` for a deep copy
