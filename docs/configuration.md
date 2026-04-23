# Configuration

libcontain provides extensive configuration options for alignment, allocators, comparators, hash functions, and build modes.

---

## Alignment

Aligned allocation is critical for:
- **SIMD operations** — 16-byte (SSE), 32-byte (AVX), 64-byte (AVX-512)
- **Atomic operations** — Platform-specific alignment requirements
- **Cache-line optimization** — 64-byte alignment to prevent false sharing

```c
/* 64-byte alignment for cache-line friendly vector */
Vector *vec = vector_create_aligned(sizeof(atomic_int), 64);

/* 32-byte alignment for AVX operations */
Vector *vec = vector_create_aligned(sizeof(float), 32);

/* Deque with SIMD alignment */
Deque *deq = deque_create_aligned(sizeof(__m128), 16);

/* LinkedList with custom alignment */
LinkedList *list = linkedlist_create_aligned(sizeof(MyStruct), 64);

/* HashSet with aligned items */
HashSet *set = hashset_create_aligned(sizeof(__m256), 32);

/* HashMap with aligned keys and values */
HashMap *map = hashmap_create_aligned(sizeof(__m128), sizeof(__m256), 16, 32);
```

---

## Pool Allocator

libcontain includes a high performance pool allocator for node-based containers (LinkedList, HashSet, HashMap). Pool allocators drastically reduce malloc overhead and eliminate fragmentation for small fixed-size allocations.

```c
/* Create pool allocator:
   stride = entry size
   block_capacity = entries per block
   alignment = entry alignment
   growth = GROW_GOLDEN or GROW_DOUBLE
 */
Allocator *pool = pool_allocator_create(64, 100, 8, GROW_DOUBLE);

/* Containers accept pool at creation */
LinkedList *list = linkedlist_create_with_allocator(sizeof(int), pool);
HashSet *set = hashset_create_with_allocator(sizeof(int), pool);
HashMap *map = hashmap_create_with_allocator(sizeof(int), sizeof(int), pool);

/* You can destroy the pool pointer immediately */
allocator_destroy(pool);

/* All containers continue to work normally */
for (int i = 0; i < 100; i++) {
    linkedlist_push_back(list, &i);
    hashset_insert(set, &i);
    hashmap_insert(map, &i, &i);
}

/* Allocator is automatically freed when last container is destroyed */
linkedlist_destroy(list);
hashset_destroy(set);
hashmap_destroy(map);
```

> ✅ **Zero manual lifetime management**. Containers automatically take references. Allocator cleans itself up.
> ✅ **Multiple containers can safely share the same pool**. Memory is reused across all containers.

---

## Custom Comparators

```c
/* Comparator for fixed-size types */
int compare_int(const void *a, const void *b) {
    int ia = *(const int *)a, ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

/* Comparator for strings (char*) */
int compare_str(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

/* Use with containers */
Vector *vec = vector_create_with_comparator(sizeof(MyStruct), my_compare);
HashSet *set = hashset_create_with_comparator(sizeof(const char*), compare_str);
```

---

## Custom Hashers

```c
/* Hash function type */
typedef size_t (*lc_Hasher)(const void *key, size_t len);

/* Built-in hash functions */
size_t lc_hash_djb2(const void *key, size_t len);      /* Simple, good for short strings */
size_t lc_hash_fnv_1a(const void *key, size_t len);    /* Good for arbitrary binary data */

/* Custom hash function */
size_t hash_int(const void *key, size_t len) {
    (void)len;
    return *(const int *)key * 2654435761u;
}

/* Use with containers */
HashSet *set = hashset_create_with_hasher(sizeof(int), hash_int);
HashMap *map = hashmap_create_with_hasher(sizeof(const char*), sizeof(int), lc_hash_djb2_str);
```

---

## Debug Mode

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

## Compiler Flags

```bash
# Release build
gcc -O2 -std=c99 -Iinclude your_program.c -o your_program

# Debug build
gcc -O0 -g -std=c99 -DCONTAINER_DEBUG -Iinclude your_program.c -o your_program_debug

# Minimal binary size
gcc -Os -flto -ffunction-sections -fdata-sections -Wl,--gc-sections -Iinclude your_program.c -o your_program
```

---

## Integration Options

### Header-Only (STB Style)

Define implementation in exactly **ONE** .c file:

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

---

### Separate Compilation

Compile the provided .c files:

```bash
gcc -c src/vector.c src/deque.c src/linkedlist.c src/hashset.c src/hashmap.c
gcc main.c vector.o deque.o linkedlist.o hashset.o hashmap.o -o program
```

---

### Selective Inclusion

Include only the containers you need:

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>

/* Deque, LinkedList, HashSet, HashMap not included */
```

---