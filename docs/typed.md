# Type-Safe Wrappers

Typed wrapper macros generate compile-time type-safe interfaces for all containers, eliminating the need for manual casting. All generated functions are static inline — no linking issues, zero runtime overhead.

---

## Declaration Macros

| Container | Macro Signature |
|-----------|-----------------|
| Vector | `DECL_VECTOR_TYPE(T, size, name)` |
| Deque | `DECL_DEQUE_TYPE(T, size, name)` |
| LinkedList | `DECL_LINKEDLIST_TYPE(T, size, name)` |
| HashSet | `DECL_HASHSET_TYPE(T, size, name)` |
| HashMap | `DECL_HASHMAP_TYPE(K, V, ksize, vsize, name)` |

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

Examples

Vector

```c
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)

IntVector *vec = IntVector_create();
IntVector_push(vec, 42);
int val = IntVector_at(vec, 0);
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

LinkedList

```c
DECL_LINKEDLIST_TYPE(const char*, 0, StringList)

StringList *list = StringList_create();
StringList_push_back(list, "hello");
const char *s = StringList_front(list);
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

HashMap iteration yields entry pointers. Use _entry_key and _entry_value to access:

```c
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

StrIntMap *map = StrIntMap_create();
StrIntMap_insert(map, "one", 1);
StrIntMap_insert(map, "two", 2);

Iterator it = StrIntMap_iter(map);
const void *entry;
while ((entry = iter_next(&it))) {
    const char *key = StrIntMap_entry_key(map, entry);
    int val = StrIntMap_entry_value(map, entry);
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

Generated Functions

For DECL_VECTOR_TYPE(int, sizeof(int), IntVector), the following functions are generated:

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
| `int* IntVector_get_ptr(IntVector *n, size_t idx)` | Get mutable pointer |
| `const int* IntVector_at_const(const IntVector *n, size_t idx)` | Get const pointer |

---

### Removal

| Function | Description |
|----------|-------------|
| `int IntVector_pop(IntVector *n)` | Remove last element |
| `int IntVector_remove(IntVector *n, size_t idx)` | Remove at index |
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
| `Iterator IntVector_iter(const IntVector *n)` | Forward iterator |
| `Iterator IntVector_iter_reversed(const IntVector *n)` | Reverse iterator |

String Mode

For string mode (size == 0), use const char* as the element type:

```c
DECL_VECTOR_TYPE(const char*, 0, StringVector)

StringVector *vec = StringVector_create();
StringVector_push(vec, "hello");  /* strdup'd automatically */
const char *s = StringVector_at(vec, 0);  /* returns const char* */
StringVector_destroy(vec);        /* strings freed automatically */
```

Note: String mode returns const char* to prevent modification of internal strings.

HashMap Key/Value Types

For HashMap, ksize and vsize determine the key/value types:

```c
/* Fixed-size key, fixed-size value */
DECL_HASHMAP_TYPE(int, double, sizeof(int), sizeof(double), IntDoubleMap)

/* String key, fixed-size value */
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

/* String key, string value */
DECL_HASHMAP_TYPE(const char*, const char*, 0, 0, StrStrMap)
```

Notes

· All functions are static inline — include the header in exactly one place
· Each DECL_*_TYPE must have a unique name
· For large structs, use the generic API with pointers to avoid copying
· String mode automatically manages strdup/free
· _wrap is a zero-cost cast — it does NOT take ownership. Destroy the typed pointer, not the original.
· Const-correct variants (_at_const, _get_const) are provided for read-only access
