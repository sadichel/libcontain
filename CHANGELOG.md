# Changelog

## [1.0.1] - 2026-08-13

### Added

- **Reference Strings**: Strings can now be stored as references (`owned = 0`) without copying or freeing. The caller is responsible for ensuring the string outlives the container.
- **HashMap Mixed Ownership**: Keys and values can now have independent ownership modes (key owned + value referenced, or vice versa).
- **Type-Safe Ref Macros**: New macros for all containers with explicit ownership control:
  - `DECL_VECTOR_REF_TYPE(T, size, name, owned)`
  - `DECL_DEQUE_REF_TYPE(T, size, name, owned)`
  - `DECL_LINKEDLIST_REF_TYPE(T, size, name, owned)`
  - `DECL_HASHSET_REF_TYPE(T, size, name, owned)`
  - `DECL_HASHMAP_REF_TYPE(K, V, ksize, vsize, name, kowned, vowned)`
- **Typed Iterators**: Each typed container now provides a typed iterator with type-safe `next` functions:
  - `name##Iterator_next(it, T *out)` — Advances iterator and returns element
  - `name##_iter()` — Forward iterator
  - `name##_iter_reversed()` — Reverse iterator (where applicable)
  - `name##_as_iterator()` — Export to generic iterator
- **String Ref Convenience Functions**: 
  - `vector_str_ref()`, `deque_str_ref()`, `linkedlist_str_ref()`, `hashset_str_ref()`
  - `hashmap_str_ref_str_ref()`, `hashmap_str_ref_any()`, `hashmap_any_str_ref()`
- **Builder Ownership Control**: Builders now accept `owned` parameter for string ownership configuration

### Changed

- Typed wrapper macros now use builders internally for consistent construction
- Documentation expanded with reference string usage examples
- Typed wrapper iteration now returns type-safe iterators
**Performance**: Removed runtime null checks in release builds

---

## [1.0.0] - 2026-04-23

### Initial Release

**Containers:**
- Vector (dynamic array)
- Deque (double-ended queue)
- LinkedList (doubly-linked list)
- HashSet (hash table with separate chaining)
- HashMap (key-value hash table)

**Features:**
- Generic Container interface for polymorphic operations
- Type-safe wrapper macros (zero-cost abstraction)
- Iterator pipelines (filter, map, skip, take, flatten, zip)
- Chainer for fused pipeline execution with reuse
- Pool allocator for node-based containers (reduces malloc overhead)
- First-class string mode (auto `strdup`/`free`)
- SIMD-friendly alignment support
- Set operations (union, intersection, difference)
- 500+ unit tests
- Benchmarks vs uthash, klib, stb_ds, C++ STL
- pkg-config support

**Build Systems:**
- Makefile (primary)
- CMake (optional)