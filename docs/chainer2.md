# Chainer2

**Extensible lazy fused pipeline with generator support**

Chainer2 is API-compatible with Chainer but adds support for **generator chains** and **extensibility** (custom chain types).

---

## What's New in Chainer2

| Feature | Chainer | Chainer2 |
|---------|:-------:|:--------:|
| Filter / Map / Skip / Take | ✅ | ✅ |
| Flatten | ❌ | ✅ |
| Zip | ❌ | ✅ |
| Custom chain types | ❌ | ✅ |
| Generator support | ❌ | ✅ |

---

## API (Identical to Chainer)

```c
/* Creation */
Chainer c = Chain(container);
chain_bind(&c, new_container);
chain_destroy(&c);

/* Builders */
chain_filter(&c, pred);
chain_map(&c, fn, stride);
chain_skip(&c, n);
chain_take(&c, n);
chain_flatten(&c);     /* New in Chainer2 */
chain_zip(&c, other, merge, stride);  /* New in Chainer2 */

/* Terminals */
size_t n = chain_count(&c);
bool b = chain_any(&c, pred);
bool b = chain_all(&c, pred);
chain_for_each(&c, f);
void *acc = chain_fold(&c, acc, f);
const void *e = chain_find(&c, pred);
const void *e = chain_first(&c);
Container *result = chain_collect(&c);
size_t inserted = chain_collect_into(&c, dst);
```

---

## New Builders

### Flatten

Flattens a container of containers into a single stream:

```c
/* Vector of vectors */
Vector *outer = vector_create(sizeof(Vector*));
vector_push(outer, &vec1);
vector_push(outer, &vec2);
vector_push(outer, &vec3);

Chainer c = Chain((Container *)outer);
chain_flatten(&c);      /* Flatten into single stream */
chain_filter(&c, is_even);
size_t n = chain_count(&c);
```

### Zip

Pairs elements from two containers:

```c
typedef struct { int a; int b; } Pair;

void *merge_pairs(const Container *ca, const void *a,
                  const Container *cb, const void *b,
                  void *buf) {
    ((Pair *)buf)->a = *(const int *)a;
    ((Pair *)buf)->b = *(const int *)b;
    return buf;
}

Chainer c = Chain(vec1);
chain_zip(&c, vec2, merge_pairs, sizeof(Pair));
Vector *pairs = vector_create(sizeof(Pair));
chain_collect_into(&c, pairs);
```

---

## Generator Chains

Chains that produce multiple outputs per input (flatten, zip) are called generators. They signal "busy" state to prevent the source iterator from advancing while still yielding elements.

Chainer2 automatically detects generator chains and uses an optimized slow path that supports them. Pipelines without generators use the original fast path with zero overhead.

---

## Custom Chain Types

Chainer2 is fully extensible — users can add custom chain types without modifying core library code.

### ChainType Structure

```c
typedef struct ChainType {
    ChainProcessFunc process;   /* Process one element */
    ChainBusyFunc    is_busy;   /* Check if busy (for generators) */
    ChainResetFunc   reset;     /* Reset to initial state */
    ChainDestroyFunc destroy;   /* Free resources */
    /* Chain-specific data follows */
} ChainType;
```

### ChainStatus Return Values

| Status | Meaning |
|--------|---------|
| `CHAIN_PASS` | Element passes through, keep same source element |
| `CHAIN_SKIP` | Skip this element, get next from source |
| `CHAIN_STOP` | Stop iteration entirely |

### Custom Chain Example

```c
/* Custom chain that prints each element */
typedef struct {
    ChainType base;
    FILE *output;
} PrintChain;

static ChainStatus print_process(PrintChain *pc, const Container *c, void **item) {
    fprintf(pc->output, "%d\n", *(int *)*item);
    return CHAIN_PASS;
}

static void print_destroy(PrintChain *pc) {
    /* Nothing to free */
}

Chainer *chain_print(Chainer *c, FILE *output) {
    if (!_chain_grow(c)) return c;
    
    PrintChain *pc = malloc(sizeof(PrintChain));
    if (!pc) return c;
    
    pc->base.process = (ChainProcessFunc)print_process;
    pc->base.is_busy = NULL;
    pc->base.reset = NULL;
    pc->base.destroy = (ChainDestroyFunc)print_destroy;
    pc->output = output;
    
    c->chains[c->chain_len++] = (ChainType *)pc;
    return c;
}
```

### Custom Generator Chain Example

```c
/* Custom chain that repeats each element 3 times */
typedef struct {
    ChainType base;
    void *stashed;
    int repeat_count;
} RepeatChain;

static bool repeat_is_busy(const RepeatChain *rc) {
    return rc->repeat_count > 0;
}

static ChainStatus repeat_process(RepeatChain *rc, const Container *c, void **item) {
    (void)c;
    
    if (rc->repeat_count > 0) {
        /* Still repeating previous element */
        *item = rc->stashed;
        rc->repeat_count--;
        if (rc->repeat_count == 0) rc->stashed = NULL;
        return CHAIN_PASS;
    }
    
    /* New element - stash it and repeat 3 times */
    rc->stashed = *item;
    rc->repeat_count = 2;  /* Will emit current + 2 more = 3 total */
    return CHAIN_PASS;
}

static void repeat_reset(RepeatChain *rc) {
    rc->repeat_count = 0;
    rc->stashed = NULL;
}

Chainer *chain_repeat(Chainer *c) {
    if (!_chain_grow(c)) return c;
    
    RepeatChain *rc = malloc(sizeof(RepeatChain));
    if (!rc) return c;
    
    rc->base.process = (ChainProcessFunc)repeat_process;
    rc->base.is_busy = (ChainBusyFunc)repeat_is_busy;
    rc->base.reset = (ChainResetFunc)repeat_reset;
    rc->base.destroy = NULL;
    rc->repeat_count = 0;
    rc->stashed = NULL;
    
    c->chains[c->chain_len++] = (ChainType *)rc;
    c->has_generators = true;
    return c;
}
```

Output: `[1, 2, 3] → [1, 1, 1, 2, 2, 2, 3, 3, 3]`

Usage:
```c
chain_repeat(&c);
```

---

## Migration from Chainer

Chainer2 is a **drop-in replacement** for Chainer. Simply change the include:

```c
/* Replace */
#include <contain/chainer.h>

/* With */
#include <contain/chainer2.h>
```

All existing code continues to work unchanged. New features (flatten, zip) are available without breaking changes.

---

## Performance

| Configuration | Performance |
|---------------|-------------|
| Fast path (no generators) | Identical to Chainer — single fused loop, zero per-stage overhead |
| Slow path (with generators) | Slightly more overhead but required for flatten/zip operations |

---

## When to Use

| Use Case | Recommendation |
|----------|----------------|
| Existing Chainer code | Use Chainer2 (drop-in replacement) |
| Need flatten or zip operations | Use Chainer2 |
| Need custom chain types | Use Chainer2 |
| Simple filter/map/skip/take pipelines | Either (identical API and performance) |

---

## See Also

- [Chainer](chainer.md) — Original lightweight implementation
- [Iterator Pipelines](iterator.md) — For one-off transformations