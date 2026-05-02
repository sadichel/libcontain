# libcontain

**Generic containers with iterator pipelines for C99**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![CI](https://github.com/sadichel/libcontain/actions/workflows/ci.yml/badge.svg)](https://github.com/sadichel/libcontain/actions/workflows/ci.yml)

## Overview

libcontain provides five container types (Vector, Deque, LinkedList, HashSet, HashMap) with a unified interface, functional iterator pipelines, and zero-overhead abstractions.

Unlike other C container libraries, libcontain lets you write:

```c
Iterator *it = IntoIter(vec);
it = iter_filter(it, is_even);
it = iter_map(it, double_int, sizeof(int));
Container *result = iter_collect(it);
```

## Quick Start

### Generic Vector
```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>

int main(void) {
    Vector *vec = vector_create(sizeof(int));
    int val = 42;
    vector_push(vec, &val);
    printf("%d\n", *(int*)vector_front(vec));
    vector_destroy(vec);
    return 0;
}
```

### Type-Safe Vector
For zero-casting type safety, use the typed vector wrapper:

```c
#define VECTOR_IMPLEMENTATION
#include <contain/typed/vector.h>

// Declare a type-safe IntVector that stores ints
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)

int main(void) {
    IntVector *vec = IntVector_create();
    
    // No casting needed - type checked at compile time
    IntVector_push(vec, 42);
    IntVector_push(vec, 123);
    IntVector_push(vec, 789);
    
    printf("First element: %d\n", IntVector_front(vec));
    printf("Second element: %d\n", IntVector_at(vec, 1));
    printf("Last element: %d\n", IntVector_back(vec));
    printf("Size: %zu\n", IntVector_len(vec));
    
    // Iterate with type safety
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("[%zu] = %d\n", i, IntVector_at(vec, i));
    }
    
    IntVector_destroy(vec);
    return 0;
}
```

### String Vector (Automatic strdup/free)
For string handling, use `item_size = 0` for automatic `strdup()` on insert and `free()` on destroy:

```c
#define VECTOR_IMPLEMENTATION
#include <contain/typed/vector.h>

// Declare StringVector - item_size 0 enables automatic string handling
DECL_VECTOR_TYPE(const char*, 0, StringVector)

int main(void) {
    StringVector *vec = StringVector_create();
    
    // Strings are automatically strdup'd
    StringVector_push(vec, "Hello");
    StringVector_push(vec, "World");
    StringVector_push(vec, "libcontain");
    
    printf("First string: %s\n", StringVector_front(vec));
    printf("Second string: %s\n", StringVector_at(vec, 1));
    printf("Last string: %s\n", StringVector_back(vec));
    printf("Number of strings: %zu\n", StringVector_len(vec));
    
    // All strings are automatically freed on destroy
    StringVector_destroy(vec);
    return 0;
}
```

### Casting Between Generic and Typed
Since all containers have `Container` as their first member, you can safely cast between generic and typed interfaces:

```c
// Create generic vector
Vector *vec = vector_create(sizeof(int));

// Can be passed to any generic function
container_len((Container*)vec);
Iterator it = Iter((Container*)vec);

// Can be cast to typed vector with 100% safety
IntVector *typed = (IntVector*)vec;

// All typed operations work perfectly
IntVector_push(typed, 42);
int val = IntVector_at(typed, 0);

// Destroy works for both types
vector_destroy(vec);
// OR: IntVector_destroy(typed);
```

This is the magic of the C inheritance pattern — zero overhead, fully safe.

## Features

- 5 containers — Vector, Deque, LinkedList, HashSet, HashMap
- Iterator pipelines — filter, map, take, skip, flatten, zip, fold
- Chainer — Zero-overhead fused pipelines with reuse
- Type-safe wrappers — DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
- First-class strings — item_size == 0 handles strdup/free automatically
- Aligned allocation — SIMD, cache-line, atomics
- Pool allocators — Reduced malloc overhead for node-based containers
- Custom comparators & hashers — User-defined comparison and hash functions
- Custom allocators — Pluggable memory management
- STB style or separate compilation — Your choice

## Documentation

- [API Reference](docs/api.md)
- [Containers Guide](docs/container.md)
- [Iterator Pipelines](docs/iterator.md)
- [Chainer](docs/chainer.md)
- [Configuration](docs/configuration.md)
- [Examples](docs/examples.md)

## Example

Word frequency analysis in 15 lines:

```c
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), WordCount)

WordCount *freq = WordCount_create();
Iterator *it = IntoIter((Container*)tokens);
it = iter_filter(it, is_not_stopword);
iter_fold(it, freq, count_words);

Iterator it2 = WordCount_iter(freq);
const void *entry;
while ((entry = iter_next(&it2))) {
    printf("%s: %d\n",
           WordCount_entry_key(freq, entry),
           WordCount_entry_value(freq, entry));
}
```

## Chainer Example

```c
Chainer c = Chain(vec);
chain_skip(&c, 10);
chain_filter(&c, is_even);
chain_map(&c, square_int, sizeof(int));
chain_take(&c, 10);

size_t count = chain_count(&c);
Container *result = chain_collect(&c);

/* Reuse on different data */
chain_bind(&c, vec2);
count = chain_count(&c);

chain_destroy(&c);
```

## Build

Using Make

```bash
make          # Build library
make test     # Run tests
make tour     # Build tour
sudo make install  # Install system-wide
```

Using CMake

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

Header-Only (no build)

For individual containers:
```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
```

For all containers at once:
```c
#define LIBCONTAIN_IMPLEMENTATION
#include <contain/contain.h>
```

## Complete Tour

```bash
gcc -O2 tests/tour.c -Iinclude -o tour
./tour
```

The tour demonstrates all containers, iterator pipelines, chainer, and real-world usage.

## License

MIT

## Author

Sadiq Idris (@sadichel)
