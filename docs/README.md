# libcontain Documentation

Lightweight generic containers for C with zero overhead.

---

## 📋 Getting Started

- [Installation](#installation)
- [Basic Concepts](#basic-concepts)

---

## 📚 Guides

| Guide | Description |
|-------|-------------|
| [Containers Guide](container.md) | Vector, Deque, LinkedList, HashSet, HashMap |
| [Iterator Pipelines](iterator.md) | filter, map, take, skip, flatten, zip, fold |
| [Chainer](chainer.md) | Zero-overhead fused pipelines |
| [Chainer2](chainer2.md) | Extensible pipelines with flatten/zip support (API-compatible with Chainer) |
| [Type-Safe Wrappers](typed.md) | `DECL_*_TYPE` type-safe macros |
| [Configuration](configuration.md) | Alignment, allocators, comparators, hashers |

---

## 📖 Reference

- [API Reference](api.md) — Complete function listing

---

## 💡 Examples

- [Tour](../tests/tour.c) — Complete tour with demos
- [Word Frequency](examples.md#word-frequency) — Real-world example
- [Custom Allocator](examples.md#custom-allocator) — Pool allocator usage

---

## Installation

libcontain supports three usage models:

---

### ✅ Header-Only Mode (No Build)

Copy the `include/contain/` directory to your project's include path.

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

No linking, no separate compilation required.

---

### ✅ Library Mode (System Install)

**GNU Make:**
```bash
make          # Build library
make test     # Run test suite
sudo make install  # Install system-wide
```

**CMake:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build                    # Run test suite
sudo cmake --install build                # Install
```

In your CMake project:
```cmake
find_package(libcontain REQUIRED)
target_link_libraries(your_target libcontain::libcontain)
```

All modes are fully supported and ABI-compatible.

---

## Basic Concepts

### Generic Containers

Every container implements the `Container` interface:

```c
typedef struct Container {
    void *items;
    size_t len;
    size_t capacity;
    const ContainerVTable *ops;
} Container;
```

Generic functions work on **any** container type:

```c
size_t len = container_len((Container *)vec);
size_t hash = container_hash((Container *)set);
Array *snapshot = container_as_array((Container *)deq);
Container *clone = container_clone((Container *)list);
container_clear((Container *)map);
container_destroy((Container *)map);
```

---

### Iterators

All containers support forward and reverse iteration:

```c
Iterator it = Iter((Container *)vec);
const void *item;

while ((item = iter_next(&it))) {
    process(item);
}
```

---

### Type-Safe Wrappers

Generate compile-time type-safe interfaces with macros:

```c
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)

IntVector *vec = IntVector_create();
IntVector_push(vec, 42);
int val = IntVector_at(vec, 0);
IntVector_destroy(vec);
```

---

### String Mode

Set `item_size = 0` for automatic string handling:

```c
Vector *strings = vector_create(0);
vector_push(strings, "hello");  /* strdup'd automatically */
vector_push(strings, "world");
/* Strings automatically freed on destroy */
```

---