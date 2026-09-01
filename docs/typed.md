# Type-Safe Wrappers

Typed wrapper macros generate compile-time type-safe interfaces for all containers, eliminating the need for manual casting. All generated functions are static inline — no linking issues, zero runtime overhead.

---

## Declaration Macros

| Container | Macro Signature |
|-----------|-----------------|
| Vector | `DECL_VECTOR_TYPE(T, size, name)` |
| Vector | `DECL_VECTOR_REF_TYPE(T, size, name, owned)` |
| Deque | `DECL_DEQUE_TYPE(T, size, name)` |
| Deque | `DECL_DEQUE_REF_TYPE(T, size, name, owned)` |
| LinkedList | `DECL_LINKEDLIST_TYPE(T, size, name)` |
| LinkedList | `DECL_LINKEDLIST_REF_TYPE(T, size, name, owned)` |
| HashSet | `DECL_HASHSET_TYPE(T, size, name)` |
| HashSet | `DECL_HASHSET_REF_TYPE(T, size, name, owned)` |
| HashMap | `DECL_HASHMAP_TYPE(K, V, ksize, vsize, name)` |
| HashMap | `DECL_HASHMAP_REF_TYPE(K, V, ksize, vsize, name, kowned, vowned)` |

Every container has a plain `_TYPE` macro and a `_REF_TYPE` macro. The `_REF_TYPE` variant is for **string elements only** (`size == 0`, i.e. `const char*`): it adds one or two extra `owned` flags that control whether the container `strdup`s/frees its strings or just stores pointers to strings you manage yourself — see [Reference Types](#reference-types) below. `_REF_TYPE` isn't meaningful for fixed-size elements, since those are always copied by value.

---

## Ownership Model

The typed wrapper **is** the container. There is no separate wrapper object — the typed pointer points directly to the generic container memory. Casting between typed and generic is **zero cost** and does not transfer ownership:

```c
/* Generic creation */
Vector *raw = vector_create(sizeof(int));

/* Zero-cost cast to typed (same memory, no new allocation) */
IntVector *typed = (IntVector*)raw;

/* Destroy either one — same memory freed */
IntVector_destroy(typed);  /* or vector_destroy(raw) */
```

Rule: Destroy the container once, using either the typed or generic API. Never destroy both.

---

## String Ownership

libcontain supports two string ownership modes for pointer-type elements (`size == 0`):

| Mode | Insert Behavior | Destroy Behavior | User Responsibility |
|------|-----------------|-------------------|----------------------|
| `owned = 1` | `strdup` copy | `free` | None — the container owns its own copy |
| `owned = 0` | Store pointer | No `free` | String must outlive the container |

`DECL_*_TYPE` macros always behave as `owned = 1` in string mode (see [String Mode](#string-mode)). To get `owned = 0` behavior — or to make the choice explicit — use the matching `DECL_*_REF_TYPE` macro.

---

## Examples

Vector

```c
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)

IntVector *vec = IntVector_create();
IntVector_push(vec, 42);
int val = IntVector_at(vec, 0);
IntVector_destroy(vec);
```

Vector Iteration

```c
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)

IntVector *vec = IntVector_create();
IntVector_push(vec, 10);
IntVector_push(vec, 20);
IntVector_push(vec, 30);

/* Forward */
IntVectorIterator it = IntVector_iter(vec);
int val;
while (IntVector_next(&it, &val)) {
    printf("%d\n", val);
}
/* 10, 20, 30 */

/* Reverse */
IntVectorIterator rit = IntVector_iter_reversed(vec);
while (IntVector_next(&rit, &val)) {
    printf("%d\n", val);
}
/* 30, 20, 10 */

IntVector_destroy(vec);
```

Deque

```c
DECL_DEQUE_TYPE(double, sizeof(double), DoubleDeque)

DoubleDeque *deq = DoubleDeque_create();
DoubleDeque_push_back(deq, 3.14);
double val = DoubleDeque_front(deq);
DoubleDeque_destroy(deq);
```

Deque Iteration 

```c
DECL_DEQUE_TYPE(double, sizeof(double), DoubleDeque)

DoubleDeque *deq = DoubleDeque_create();
DoubleDeque_push_back(deq, 3.14);
DoubleDeque_push_back(deq, 2.71);
DoubleDeque_push_back(deq, 6.62);

/* Forward */
DoubleDequeIterator it = DoubleDeque_iter(deq);
double val;
while (DoubleDeque_next(&it, &val)) {
    printf("%f\n", val);
}
/* 3.14, 2.71, 6.62 */

/* Reverse */
DoubleDequeIterator rit = DoubleDeque_iter_reversed(deq);
while (DoubleDeque_next(&rit, &val)) {
    printf("%f\n", val);
}
/* 6.62, 2.71, 3.14 */

DoubleDeque_destroy(deq);
```

LinkedList

```c
DECL_LINKEDLIST_TYPE(const char*, 0, StringList)

StringList *list = StringList_create();
StringList_push_back(list, "hello");
const char *s = StringList_front(list);
StringList_destroy(list);
```

LinkedList Iteration 

```c
DECL_LINKEDLIST_TYPE(const char*, 0, StringList)

StringList *list = StringList_create();
StringList_push_back(list, "hello");
StringList_push_back(list, "world");
StringList_push_back(list, "libcontain");

/* Forward */
StringListIterator it = StringList_iter(list);
const char *val;
while (StringList_next(&it, &val)) {
    printf("%s\n", val);
}
/* hello, world, libcontain */

/* Reverse */
StringListIterator rit = StringList_iter_reversed(list);
while (StringList_next(&rit, &val)) {
    printf("%s\n", val);
}
/* libcontain, world, hello */

StringList_destroy(list);
```

HashSet

```c
DECL_HASHSET_TYPE(int, sizeof(int), IntSet)

IntSet *set = IntSet_create();
IntSet_insert(set, 42);
bool exists = IntSet_contains(set, 42);
IntSet_destroy(set);
```

HashSet Iteration

```c
DECL_HASHSET_TYPE(int, sizeof(int), IntSet)

IntSet *set = IntSet_create();
IntSet_insert(set, 1);
IntSet_insert(set, 2);
IntSet_insert(set, 3);

IntSetIterator it = IntSet_iter(set);
int val;
while (IntSet_next(&it, &val)) {
    printf("%d\n", val);
}
/* prints each element once, order unspecified */

IntSet_destroy(set);
```

Note: unlike Vector, HashSet only supports forward iteration — there is no `_iter_reversed` for HashSet, since sets have no meaningful order.

HashMap

```c
/* string -> int */
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), WordCount)

WordCount *map = WordCount_create();
WordCount_insert(map, "apple", 5);
int count = WordCount_get(map, "apple");
WordCount_destroy(map);

/* string -> string */
DECL_HASHMAP_TYPE(const char*, const char*, 0, 0, StrStrMap)

StrStrMap *map = StrStrMap_create();
StrStrMap_insert(map, "name", "Alice");
const char *name = StrStrMap_get(map, "name");
StrStrMap_destroy(map);
```

HashMap Iteration

```c
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

StrIntMap *map = StrIntMap_create();
StrIntMap_insert(map, "one", 1);
StrIntMap_insert(map, "two", 2);

StrIntMapIterator it = StrIntMap_iter(map);
const char *key;
int val;
while (StrIntMap_next(&it, &key, &val)) {
    printf("%s = %d\n", key, val);
}

StrIntMap_destroy(map);
```

Pipeline Support (Chainer)

Typed containers work seamlessly with the Chainer pipeline API:

```c
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_HASHSET_TYPE(int, sizeof(int), IntSet)

IntVector *vec = IntVector_create();
for (int i = 0; i < 100; i++) IntVector_push(vec, i);

/* Build pipeline on generic container */
Chainer c = Chain((Container*)vec);
chain_filter(&c, is_even);
chain_map(&c, double_int, sizeof(int));
chain_take(&c, 10);

/* Collect into new typed container */
IntVector *result = (IntVector*)chain_collect(&c);
chain_destroy(&c);

IntVector_destroy(result);
IntVector_destroy(vec);
```

---

## Reference Types

`_REF_TYPE` macros are the string-mode (`size == 0`) counterpart to `_TYPE` macros. They let you choose ownership explicitly instead of accepting the default `owned = 1` behavior, so the container can store a raw `const char*` instead of `strdup`ing it.

| `owned` value | Insert Behavior | Destroy Behavior | Notes |
|----------------|-----------------|-------------------|-------|
| `1` | `strdup` copy on insert | `free`s each string | Identical to the plain `_TYPE` macro's string mode |
| `0` | Stores the raw pointer, no copy | Does **not** free strings | Caller must guarantee the string outlives the container |

`DECL_HASHMAP_REF_TYPE` takes two independent flags, `kowned` and `vowned`, so the key string and value string can each be owned or reference-only:

| `kowned` | `vowned` | Meaning |
|----------|----------|---------|
| `1` | `1` | Both key and value are `strdup`'d and freed on destroy |
| `1` | `0` | Key is copied/freed; value is a reference the caller must keep alive |
| `0` | `1` | Key is a reference the caller must keep alive; value is copied/freed |
| `0` | `0` | Both key and value are references; container never allocates or frees strings |

**Warning:** In `owned = 0` mode, the container never validates that a referenced string is still alive. Freeing or invalidating a string while it's still stored in the container is undefined behavior.

### Vector reference

```c
DECL_VECTOR_REF_TYPE(const char*, 0, StringRefVector, 0)

StringRefVector *ref = StringRefVector_create();
char str[10] = "hello";
StringRefVector_push(ref, str);  /* no strdup — stores pointer */

strcpy(str, "world");
const char *s = StringRefVector_at(ref, 0);  /* "world" — sees the modification! */

StringRefVector_destroy(ref);  /* no free — caller owns str */
```

### Deque reference

```c
DECL_DEQUE_REF_TYPE(const char*, 0, StringRefDeque, 0)

StringRefDeque *ref = StringRefDeque_create();
const char *label = "queued";
StringRefDeque_push_back(ref, label);  /* stores pointer, no copy */

const char *s = StringRefDeque_front(ref);  /* "queued" */

StringRefDeque_destroy(ref);  /* no free — caller still owns label */
```

### LinkedList reference

```c
DECL_LINKEDLIST_REF_TYPE(const char*, 0, StringRefList, 0)

StringRefList *ref = StringRefList_create();
static const char *tag = "node-a";  /* must outlive the list */
StringRefList_push_back(ref, tag);

StringRefList_destroy(ref);  /* no free — tag is still valid */
```

### HashSet reference

```c
DECL_HASHSET_REF_TYPE(const char*, 0, StringRefSet, 0)

StringRefSet *ref = StringRefSet_create();
const char *name = "alice";
StringRefSet_insert(ref, name);  /* stores pointer, no strdup */

bool exists = StringRefSet_contains(ref, "alice");  /* true — compares by value */

StringRefSet_destroy(ref);  /* no free — caller owns name */
```

### HashMap reference

```c
/* Both key and value are references — neither is copied nor freed */
DECL_HASHMAP_REF_TYPE(const char*, const char*, 0, 0, StrStrRefMap, 0, 0)

StrStrRefMap *ref = StrStrRefMap_create();
const char *key = "name";
const char *val = "Alice";
StrStrRefMap_insert(ref, key, val);  /* stores both pointers directly */

const char *v = StrStrRefMap_get(ref, "name");  /* "Alice" */

StrStrRefMap_destroy(ref);  /* no free — caller owns key and val */
```

```c
/* Mixed ownership: key is copied/freed, value is a reference */
DECL_HASHMAP_REF_TYPE(const char*, const char*, 0, 0, StrKeyOwnedValueRefMap, 1, 0)

StrKeyOwnedValueRefMap *m = StrKeyOwnedValueRefMap_create();
const char *val = "cached-result";
StrKeyOwnedValueRefMap_insert(m, "some-key", val);
/* "some-key" is strdup'd internally; val is only referenced */

StrKeyOwnedValueRefMap_destroy(m);
/* frees the internal copy of the key; does NOT free val */
```

Generic to Typed Conversion

Since typed and generic pointers point to the same memory, conversion is a simple cast:

```c
/* Generic factory returns Container* */
Container *generic = vector_create(sizeof(int));

/* Cast to typed (zero-cost, no ownership transfer) */
IntVector *typed = (IntVector*)generic;

/* Cast back to generic */
Vector *raw = (Vector*)typed;

/* Destroy once — either works */
IntVector_destroy(typed);
```

---

## Generated Functions

For `DECL_VECTOR_TYPE(int, sizeof(int), IntVector)`, the following functions are generated:

### Creation & Destruction

| Function | Description |
|----------|-------------|
| `IntVector *IntVector_create(void)` | Create new empty vector |
| `IntVector *IntVector_create_with_capacity(size_t cap)` | Create with initial capacity |
| `IntVector *IntVector_create_with_comparator(lc_Comparator cmp)` | Create with custom comparator |
| `IntVector *IntVector_create_aligned(size_t align)` | Create with aligned elements |
| `void IntVector_destroy(IntVector *n)` | Destroy vector and all elements |

### Container Access

| Function | Description |
|----------|-------------|
| `Vector *IntVector_unwrap(IntVector *n)` | Cast to generic (zero-cost) |
| `const Vector *IntVector_unwrap_const(const IntVector *n)` | Const cast to generic |
| `IntVector *IntVector_wrap(Container *c)` | Cast from generic (zero-cost, no ownership) |

---

### Queries

| Function | Description |
|----------|-------------|
| `size_t IntVector_len(const IntVector *n)` | Number of elements |
| `size_t IntVector_capacity(const IntVector *n)` | Current capacity |
| `bool IntVector_is_empty(const IntVector *n)` | Check if empty |
| `size_t IntVector_find(const IntVector *n, int val)` | Find first occurrence |
| `size_t IntVector_rfind(const IntVector *n, int val)` | Find last occurrence |
| `bool IntVector_contains(const IntVector *n, int val)` | Check existence |

---

### Insertion

| Function | Description |
|----------|-------------|
| `int IntVector_push(IntVector *n, int val)` | Append element |
| `int IntVector_insert(IntVector *n, size_t pos, int val)` | Insert at position |
| `int IntVector_insert_range(IntVector *dst, size_t pos, const IntVector *src, size_t from, size_t to)` | Insert range |
| `int IntVector_append(IntVector *dst, const IntVector *src)` | Append all elements |
| `bool IntVector_try_push(IntVector *n, int val)` | Push if capacity permits |

---

### Access & Modification

| Function | Description |
|----------|-------------|
| `int IntVector_set(IntVector *n, size_t idx, int val)` | Set element at index |
| `int IntVector_at(const IntVector *n, size_t idx)` | Get element (by value) |
| `int IntVector_at_or_default(const IntVector *n, size_t idx, int default_val)` | Get or default |
| `int IntVector_front(const IntVector *n)` | Get first element |
| `int IntVector_back(const IntVector *n)` | Get last element |
| `int const *IntVector_get_ptr(const IntVector *n, size_t idx)` | Get a read-only pointer to element |
| `int *IntVector_get_mut(IntVector *n, size_t idx)` | Get a mutable pointer to element |
| `int *IntVector_as_slice(IntVector *n)` | Get a pointer to underlying array |

---

### Removal

| Function | Description |
|----------|-------------|
| `int IntVector_pop(IntVector *n)` | Remove last element |
| `int IntVector_remove(IntVector *n, size_t idx)` | Remove at index |
| `int IntVector_remove_range(IntVector *dst, size_t from, size_t to)` | Remove range |
| `void IntVector_clear(IntVector *n)` | Remove all elements |

---

### Capacity Management

| Function | Description |
|----------|-------------|
| `int IntVector_reserve(IntVector *n, size_t cap)` | Reserve capacity |
| `int IntVector_shrink_to_fit(IntVector *n)` | Shrink to fit length |
| `int IntVector_set_comparator(IntVector *n, lc_Comparator cmp)` | Set comparator (empty only) |

---

### In-place Operations

| Function | Description |
|----------|-------------|
| `void IntVector_reverse_inplace(IntVector *n)` | Reverse in place |
| `int IntVector_sort(IntVector *n, lc_Comparator cmp)` | Sort in place |

---

### Copy & View

| Function | Description |
|----------|-------------|
| `IntVector *IntVector_reverse(const IntVector *n)` | New reversed vector |
| `IntVector *IntVector_clone(const IntVector *n)` | Deep copy |
| `IntVector *IntVector_slice(const IntVector *n, size_t from, size_t to)` | Extract slice |
| `IntVector *IntVector_instance(const IntVector *n)` | New empty of same type |
| `void IntVector_swap(IntVector *a, IntVector *b)` | Swap contents |

---

### Iteration

| Function | Description |
|----------|-------------|
| `IntIterator IntVector_iter(const IntVector *n)` | Forward iterator |
| `IntIterator IntVector_iter_reversed(const IntVector *n)` | Reverse iterator |
| `Iterator IntVector_as_iterator(IntIterator it)` | Export to generic iterator |
| `bool IntVector_next(IntIterator *it, int *out)` | Advance iterator and return current element |

Note: this table is for `DECL_VECTOR_TYPE`. HashSet and HashMap generates the same `_iter`, `_as_iterator`, and `_next` functions but no `_iter_reversed` — sets and maps have no meaningful order, so only forward iteration is generated. Other containers follow the same shape as the ones shown here; check the generated header for the exact signatures.

---

## String Mode

For string mode (`size == 0`), use `const char*` as the element type. Plain `_TYPE` macros always behave as `owned = 1`:

```c
DECL_VECTOR_TYPE(const char*, 0, StringVector)

StringVector *vec = StringVector_create();
StringVector_push(vec, "hello");  /* strdup'd automatically */
const char *s = StringVector_at(vec, 0);  /* returns const char* */
StringVector_destroy(vec); /* strings freed automatically */
```

For explicit control over ownership — including `owned = 0` — use the `_REF_TYPE` macro instead. See [Reference Types](#reference-types) above for the full set of examples across every container.

---

## HashMap Key/Value Types

For HashMap, `ksize` and `vsize` determine the key/value types:

```c
/* Fixed-size key, fixed-size value */
DECL_HASHMAP_TYPE(int, double, sizeof(int), sizeof(double), IntDoubleMap)

/* String key, fixed-size value */
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

/* String key, string value */
DECL_HASHMAP_TYPE(const char*, const char*, 0, 0, StrStrMap)

/* String key, reference string value (key owned, value referenced) */
DECL_HASHMAP_REF_TYPE(const char*, const char*, 0, 0, StrStrRefMap, 1, 0)
```

For the full matrix of `kowned`/`vowned` combinations and worked examples, see [HashMap reference](#hashmap-reference) above.

---

## Notes

- All functions are static inline — include the header in exactly one place.
- Each `DECL_*_TYPE` / `DECL_*_REF_TYPE` must have a unique name.
- For large structs, use the generic API with pointers to avoid copying.
- String mode automatically manages `strdup`/`free` when `owned = 1` (the default for plain `_TYPE` macros).
- `_REF_TYPE` macros are otherwise identical to `_TYPE` macros — same generated functions, same container behavior — they only add explicit control over the `owned` flag(s), and only apply to string elements (`size == 0`).
- Reference mode (`owned = 0`) is a zero-cost store — it does **not** take ownership. The caller must ensure the referenced string outlives the container.
- `_wrap` is a zero-cost cast — it does **not** take ownership. Destroy the typed pointer, not the original.