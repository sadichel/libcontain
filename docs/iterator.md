# Iterator Pipelines

Iterators provide functional-style transformations on container elements. All decorators are lazy — no work is performed until iteration begins.

---

## Basic Usage

```c
Iterator it = Iter((Container *)vec);
const void *item;

while ((item = iter_next(&it))) {
    process(item);
}
```

---

## Heap Iterators

Stack-allocated iterators (`Iter`/`IterReverse`) can be used directly, but must be heap allocated for pipeline composition:

```c
Iterator *it = HeapIter((Container *)vec);
it = iter_filter(it, is_even);
it = iter_map(it, double_int, sizeof(int));

Container *result = iter_collect(it);  // Iterator is automatically destroyed here
```

---

## Decorators

### Filter

Keep elements that satisfy a predicate.

```c
bool is_even(const Container *ctx, const void *item) {
    return *(const int *)item % 2 == 0;
}

Iterator *it = iter_filter(HeapIter(vec), is_even);
```

### Map

Transform each element. Stride specifies the output element size.

```c
void *double_int(const Container *ctx, const void *item, void *buf) {
    *(int *)buf = *(const int *)item * 2;
    return buf;
}

Iterator *it = iter_map(HeapIter(vec), double_int, sizeof(int));
```

### Skip

Discard the first N elements.

```c
Iterator *it = iter_skip(HeapIter(vec), 10);
```

### Take

Limit results to at most N elements.

```c
Iterator *it = iter_take(HeapIter(vec), 20);
```

### Flatten

Flatten a sequence of containers. The outer iterator must yield `Container*` pointers.

```c
// Create a vector of vectors
Vector *outer = vector_create(sizeof(Vector*));
for (int i = 0; i < 5; i++) {
    Vector *inner = vector_create(sizeof(int));
    for (int j = 0; j < 10; j++) {
        int val = i * 10 + j;
        vector_push(inner, &val);
    }
    vector_push(outer, &inner);
}

// Flatten: yields all 50 ints sequentially
Iterator *it = iter_flatten(HeapIter((Container *)outer));
const int *val;

while ((val = iter_next(it))) {
    printf("%d ", *val);  // Prints 0 1 2 ... 49
}

iter_destroy(it);

// Cleanup
for (size_t i = 0; i < vector_len(outer); i++) {
    Vector *inner = *(Vector **)vector_at(outer, i);
    vector_destroy(inner);
}
vector_destroy(outer);
```

### Zip

Pair elements from two iterators. Iteration stops when either source is exhausted.

```c
// Merge function for pairs
typedef struct { int a; int b; } Pair;

void *make_pair(const Container *ca, const void *a,
                const Container *cb, const void *b,
                void *buf) {
    ((Pair *)buf)->a = *(const int *)a;
    ((Pair *)buf)->b = *(const int *)b;
    return buf;
}

// Create two vectors
Vector *vec1 = create_int_range(1, 5);   // 1, 2, 3, 4, 5
Vector *vec2 = create_int_range(10, 14); // 10, 11, 12, 13, 14

// Zip them together
Iterator *it = iter_zip(HeapIter((Container *)vec1),
                        HeapIter((Container *)vec2),
                        make_pair, sizeof(Pair));

const Pair *pair;
while ((pair = iter_next(it))) {
    printf("(%d, %d)\n", pair->a, pair->b);
}

// Output:
// (1, 10)
// (2, 11)
// (3, 12)
// (4, 13)
// (5, 14)

iter_destroy(it);
```

### Peek

Look ahead at the next element without consuming it.

```c
Iterator *it = iter_peekable(HeapIter(vec));
const void *next = iter_peek(it);        // Not consumed
void *item = (void *)iter_next(it);      // Now consume
```

---

## Terminal Operations

These operations consume the iterator and complete the pipeline.

| Operation | Description |
|-----------|-------------|
| `iter_count(it)` | Count remaining elements |
| `iter_any(it, pred)` | Return true if any element matches |
| `iter_all(it, pred)` | Return true if all elements match |
| `iter_find(it, pred)` | Return first matching element |
| `iter_fold(it, acc, fn)` | Accumulate all elements |
| `iter_collect(it)` | Materialize into new container |
| `iter_collect_into(it, dst)` | Insert into existing container |
| `iter_for_each(it, fn)` | Apply function to each element |
| `iter_drop(it, n)` | Discard N elements (non-consuming) |

### Count
```c
size_t n = iter_count(it);
```

### Any / All
```c
bool has_even = iter_any(it, is_even);
bool all_even = iter_all(it, is_even);
```

### Find
Returns the first matching element and stops iteration.

```c
const int *first_even = iter_find(it, is_even);
```

### Fold
```c
void *sum_int(const Container *ctx, const void *item, void *acc) {
    *(int *)acc += *(const int *)item;
    return acc;
}

int sum = 0;
iter_fold(it, &sum, sum_int);
```

### Collect
Materialize into a new container of the same type as the source.

```c
Container *result = iter_collect(it);
```

### Collect In
Insert results into an existing container.

```c
size_t inserted = iter_collect_into(it, dst);
```

### For Each
```c
void print_int(const Container *ctx, const void *item) {
    printf("%d\n", *(const int *)item);
}

iter_for_each(it, print_int);
```

### Drop
Discard N elements without destroying the iterator.

```c
iter_drop(it, 10);  // Skip first 10 elements, iterator remains valid
```

---

## Pipeline Examples

### Filter → Map → Take
```c
Iterator *it = HeapIter(vec);
it = iter_filter(it, is_even);
it = iter_map(it, double_int, sizeof(int));
it = iter_take(it, 10);

Container *result = iter_collect(it);
```

### Filter → Fold
```c
int sum = 0;
Iterator *it = HeapIter(vec);
it = iter_filter(it, is_even);

iter_fold(it, &sum, sum_int);
```

### Flatten → Filter → Collect
```c
Iterator *it = HeapIter((Container *)outer);
it = iter_flatten(it);
it = iter_filter(it, is_even);

Container *evens = iter_collect(it);
```

### Zip → Map → Collect
```c
Iterator *it = iter_zip(HeapIter(vec1), HeapIter(vec2), sum_pair, sizeof(int));

Container *sums = iter_collect(it);
```

### String Pipeline
```c
void *str_to_upper(const Container *ctx, const void *item, void *buf) {
    const char *str = (const char *)item;
    char *out = (char *)buf;
    strcpy(out, str);
    for (char *p = out; *p; p++) *p = toupper(*p);
    return buf;
}

Iterator *it = iter_map(HeapIter((Container *)str_vec), str_to_upper, 256);
Container *result = iter_collect(it);
```

---

## Ownership Rules

| Operation | Ownership |
|-----------|-----------|
| Decorators | Take ownership of inner iterator |
| Terminal operations | Consume and destroy iterator automatically |
| `iter_drop` | Does **not** take ownership |

```c
Iterator *it = HeapIter(vec);
Iterator *it2 = iter_filter(it, is_even);   // Takes ownership, 'it' is now invalid

// iter_next(it);  ❌ INVALID
iter_next(it2);                              // ✅ VALID

iter_drop(it2, 5);                           // Does not destroy it2
iter_destroy(it2);                           // Must destroy manually
```

---

## Performance

Iterators are highly optimized:

- Filter operations run with minimal overhead
- Map operations transform elements in place
- Flatten and Zip handle nested structures seamlessly
- Multiple decorators compose without performance degradation

For most use cases, the overhead is negligible compared to the work performed on elements.