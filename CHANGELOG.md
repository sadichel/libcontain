# Changelog

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
- Chainer and Chainer2 for fused pipeline execution
- Pool allocator for node-based containers (reduces malloc overhead)
- First-class string mode (auto strdup/free)
- SIMD-friendly alignment support
- Set operations (union, intersection, difference)
- 500+ unit tests
- Benchmarks vs uthash, klib, stb_ds, C++ STL
- pkg-config support

**Build Systems:**
- Makefile (primary)
- CMake (optional)