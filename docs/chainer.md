# Chainer

Fused pipeline execution with **zero per-element overhead**.

---

> ✅ For extended features including **flatten**, **zip**, and custom pipeline generators see [Chainer 2](chainer2.md)

---

## Why Chainer?

Iterator pipelines are convenient but each decorator adds function call overhead per element:

```
iter_filter → iter_map → iter_take → iter_collect
    ↓            ↓           ↓
  pred()       map()       count()
```

Chainer fuses all stages into a single loop with direct function pointer calls — no per-stage overhead.

---

## Basic Usage

```c
#include <contain/chainer.h>

Chainer c = Chain((Container *)vec);
chain_skip(&c, 10);
chain_filter(&c, is_even);
chain_map(&c, double_int, sizeof(int));
chain_take(&c, 10);

size_t n = chain_count(&c);
Container *result = chain_collect(&c);

chain_destroy(&c);
```

---

## Pipeline Stages

| Stage | Function | Description |
|-------|----------|-------------|
| Skip | `chain_skip(&c, n)` | Discard first `n` elements |
| Filter | `chain_filter(&c, pred)` | Keep only elements where predicate returns `true` |
| Map | `chain_map(&c, fn, stride)` | Transform each element into buffer of `stride` bytes |
| Take | `chain_take(&c, n)` | Limit results to at most `n` elements |
| Flatten | `chain_flatten(&c)` | Flatten container of containers |
| Zip | `chain_zip(&c, other, merge, stride)` | Pair elements with another container |

---

## Terminal Operations

| Operation | Description |
|-----------|-------------|
| `chain_count(&c)` | Count surviving elements |
| `chain_any(&c, pred)` | Return `true` if any element matches |
| `chain_all(&c, pred)` | Return `true` if all elements match |
| `chain_find(&c, pred)` | Return first matching element |
| `chain_first(&c)` | Get first element (NULL if empty) |
| `chain_fold(&c, acc, fn)` | Reduce elements to single value |
| `chain_for_each(&c, fn)` | Apply function to each element |
| `chain_collect(&c)` | Create new container of same type |
| `chain_collect_into(&c, dst)` | Insert into existing container |

---

## Pipeline Reuse

Chainer supports unlimited reuse. Build the pipeline once, run it on any number of containers:

```c
Chainer c = Chain(vec1);
chain_filter(&c, is_even);
chain_map(&c, double_int, sizeof(int));

size_t n1 = chain_count(&c);      // Runs on vec1

chain_bind(&c, vec2);
size_t n2 = chain_count(&c);      // Runs on vec2 with same pipeline

chain_bind(&c, vec3);
size_t n3 = chain_count(&c);      // Runs on vec3
chain_bind(&c, vec4);
size_t n4 = chain_count(&c);      // Runs on vec4

chain_destroy(&c);
```

> ✅ Pipeline stages are preserved after each terminal operation. There is no limit on how many times a chainer can be rebound and reused.

---

## Performance

Chainer executes in a single fused loop:

```c
/* Iterator pipeline - function call per element per stage */
it = iter_filter(it, pred);   // pred called per element
it = iter_map(it, map, sz);   // map called per element
it = iter_take(it, n);        // count checked per element

/* Chainer - one loop, direct calls */
while ((e = iter_next(&it))) {
    // Skip
    if (skip_remaining) { skip_remaining--; continue; }
    
    // Filter
    if (!pred(&c, e)) continue;
    
    // Map
    e = map(&c, e, buf);
    
    // Take
    if (take_remaining == 0) break;
    take_remaining--;
    
    // Emit
    output[out_len++] = e;
}
```

✅ No per-stage function call overhead
✅ No intermediate allocations
✅ No virtual dispatch

---

## Memory Management

- `chain_destroy()` frees all chain resources
- Terminals reset state automatically, enabling reuse
- Chainer **does not** take ownership of the source container

---

## Thread Safety

Chainer is **not thread-safe**. Each thread must use its own instance.

---

## Error Handling

Chain builders silently skip allocation failures (chain continues without the failed stage). Terminals return sensible defaults:
- `0` for count operations
- `NULL` for collect operations
- `false` for boolean checks