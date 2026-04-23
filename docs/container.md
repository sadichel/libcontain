# Containers

All containers implement a common polymorphic interface, enabling generic algorithms and iterator pipelines to operate uniformly regardless of the underlying implementation.

---

## Common Interface

Every container can be cast to `Container*` and used with these generic functions:

| Function | Description |
|----------|-------------|
| `container_len(c)` | Get number of elements |
| `container_hash(c)` | Compute cached content hash |
| `container_as_array(c)` | Export to flat contiguous Array |
| `container_clone(c)` | Create deep independent copy |
| `container_instance(c)` | Create new empty container of same type |
| `container_clear(c)` | Remove all elements, retain capacity |
| `container_destroy(c)` | Free all resources |

---

## Vector

Dynamic array with exponential growth. **O(1)** amortized push/pop, **O(1)** random access.

### Creation
```c
Vector *vec = vector_create(sizeof(int));
Vector *vec = vector_create_with_capacity(sizeof(double), 1024);
Vector *vec = vector_create_with_comparator(sizeof(MyStruct), my_compare);
Vector *vec = vector_create_aligned(sizeof(float), 64);
Vector *vec = vector_str();  /* automatic string handling mode */
```

### Operations

| Operation | Function | Complexity |
|-----------|----------|:----------:|
| Push back | `vector_push(vec, &val)` | O(1)* |
| Pop back | `vector_pop(vec)` | O(1) |
| Insert | `vector_insert(vec, pos, &val)` | O(n) |
| Insert range | `vector_insert_range(dst, pos, src, from, to)` | O(n) |
| Append | `vector_append(dst, src)` | O(n) |
| Remove | `vector_remove(vec, pos)` | O(n) |
| Remove range | `vector_remove_range(vec, from, to)` | O(n) |
| Get | `vector_at(vec, idx)` | O(1) |
| Get or default | `vector_at_or_default(vec, idx, &default)` | O(1) |
| Set | `vector_set(vec, idx, &val)` | O(1) |
| Front | `vector_front(vec)` | O(1) |
| Back | `vector_back(vec)` | O(1) |
| Get mutable | `vector_at_mut(vec, idx)` | O(1) |
| Length | `vector_len(vec)` | O(1) |
| Capacity | `vector_capacity(vec)` | O(1) |
| Is empty | `vector_is_empty(vec)` | O(1) |
| Find | `vector_find(vec, &val)` | O(n) |
| Reverse find | `vector_rfind(vec, &val)` | O(n) |
| Contains | `vector_contains(vec, &val)` | O(n) |
| Clear | `vector_clear(vec)` | O(n) |
| Reserve | `vector_reserve(vec, cap)` | O(1)* |
| Shrink to fit | `vector_shrink_to_fit(vec)` | O(n) |
| Resize | `vector_resize(vec, new_len)` | O(n) |
| Reverse | `vector_reverse_inplace(vec)` | O(n) |
| Sort | `vector_sort(vec, cmp)` | O(n log n) |
| Unique | `vector_unique(vec)` | O(n) |
| Splice | `vector_splice(dst, pos, remove, src, from, to)` | O(n) |
| Clone | `vector_clone(vec)` | O(n) |
| Slice | `vector_slice(vec, from, to)` | O(n) |
| Instance | `vector_instance(vec)` | O(1) |
| To array | `vector_to_array(vec)` | O(n) |
| Iterate | `vector_iter(vec)` | O(1) |
| Iterate reverse | `vector_iter_reversed(vec)` | O(1) |

\* Amortized complexity

---

## Deque

Double-ended queue with circular buffer. **O(1)** amortized push/pop at both ends, **O(1)** random access.

### Creation
```c
Deque *deq = deque_create(sizeof(int));
Deque *deq = deque_create_with_capacity(sizeof(double), 1024);
Deque *deq = deque_create_with_comparator(sizeof(MyStruct), my_compare);
Deque *deq = deque_create_aligned(sizeof(float), 64);
Deque *deq = deque_str();  /* automatic string handling mode */
```

### Operations

| Operation | Function | Complexity |
|-----------|----------|:----------:|
| Push back | `deque_push_back(deq, &val)` | O(1)* |
| Push front | `deque_push_front(deq, &val)` | O(1)* |
| Pop back | `deque_pop_back(deq)` | O(1) |
| Pop front | `deque_pop_front(deq)` | O(1) |
| Insert | `deque_insert(deq, pos, &val)` | O(n) |
| Insert range | `deque_insert_range(dst, pos, src, from, to)` | O(n) |
| Append | `deque_append(dst, src)` | O(n) |
| Remove | `deque_remove(deq, pos)` | O(n) |
| Remove range | `deque_remove_range(deq, from, to)` | O(n) |
| Get | `deque_at(deq, idx)` | O(1) |
| Get or default | `deque_at_or_default(deq, idx, &default)` | O(1) |
| Set | `deque_set(deq, idx, &val)` | O(1) |
| Front | `deque_front(deq)` | O(1) |
| Back | `deque_back(deq)` | O(1) |
| Get mutable | `deque_at_mut(deq, idx)` | O(1) |
| Length | `deque_len(deq)` | O(1) |
| Capacity | `deque_capacity(deq)` | O(1) |
| Is empty | `deque_is_empty(deq)` | O(1) |
| Find | `deque_find(deq, &val)` | O(n) |
| Reverse find | `deque_rfind(deq, &val)` | O(n) |
| Contains | `deque_contains(deq, &val)` | O(n) |
| Clear | `deque_clear(deq)` | O(n) |
| Reserve | `deque_reserve(deq, cap)` | O(1)* |
| Shrink to fit | `deque_shrink_to_fit(deq)` | O(n) |
| Resize | `deque_resize(deq, new_len)` | O(n) |
| Reverse | `deque_reverse_inplace(deq)` | O(n) |
| Sort | `deque_sort(deq, cmp)` | O(n log n) |
| Unique | `deque_unique(deq)` | O(n) |
| Splice | `deque_splice(dst, pos, remove, src, from, to)` | O(n) |
| Clone | `deque_clone(deq)` | O(n) |
| Slice | `deque_slice(deq, from, to)` | O(n) |
| Instance | `deque_instance(deq)` | O(1) |
| To array | `deque_to_array(deq)` | O(n) |
| Iterate | `deque_iter(deq)` | O(1) |
| Iterate reverse | `deque_iter_reversed(deq)` | O(1) |

\* Amortized complexity

---

## LinkedList

Doubly-linked list with pool allocator. **O(1)** push/pop at both ends, **O(n)** random access.

### Creation
```c
LinkedList *list = linkedlist_create(sizeof(int));
LinkedList *list = linkedlist_create_with_comparator(sizeof(MyStruct), my_compare);
LinkedList *list = linkedlist_create_aligned(sizeof(float), 64);
LinkedList *list = linkedlist_str();  /* automatic string handling mode */
LinkedList *list = linkedlist_create_with_allocator(sizeof(int), alloc);
```

### Operations

| Operation | Function | Complexity |
|-----------|----------|:----------:|
| Push back | `linkedlist_push_back(list, &val)` | O(1) |
| Push front | `linkedlist_push_front(list, &val)` | O(1) |
| Pop back | `linkedlist_pop_back(list)` | O(1) |
| Pop front | `linkedlist_pop_front(list)` | O(1) |
| Insert | `linkedlist_insert(list, pos, &val)` | O(n) |
| Insert range | `linkedlist_insert_range(dst, pos, src, from, to)` | O(n) |
| Append | `linkedlist_append(dst, src)` | O(n) |
| Remove | `linkedlist_remove(list, pos)` | O(n) |
| Remove range | `linkedlist_remove_range(list, from, to)` | O(n) |
| Get | `linkedlist_at(list, idx)` | O(n) |
| Get or default | `linkedlist_at_or_default(list, idx, &default)` | O(n) |
| Set | `linkedlist_set(list, idx, &val)` | O(n) |
| Front | `linkedlist_front(list)` | O(1) |
| Back | `linkedlist_back(list)` | O(1) |
| Get mutable | `linkedlist_at_mut(list, idx)` | O(n) |
| Length | `linkedlist_len(list)` | O(1) |
| Is empty | `linkedlist_is_empty(list)` | O(1) |
| Find | `linkedlist_find(list, &val)` | O(n) |
| Reverse find | `linkedlist_rfind(list, &val)` | O(n) |
| Contains | `linkedlist_contains(list, &val)` | O(n) |
| Clear | `linkedlist_clear(list)` | O(n) |
| Reverse | `linkedlist_reverse_inplace(list)` | O(n) |
| Sort | `linkedlist_sort(list, cmp)` | O(n log n) |
| Unique | `linkedlist_unique(list)` | O(n) |
| Splice | `linkedlist_splice(dst, pos, src, from, to)` | O(1)* |
| Clone | `linkedlist_clone(list)` | O(n) |
| Sublist | `linkedlist_sublist(list, from, to)` | O(n) |
| Instance | `linkedlist_instance(list)` | O(1) |
| To array | `linkedlist_to_array(list)` | O(n) |
| Iterate | `linkedlist_iter(list)` | O(1) |
| Iterate reverse | `linkedlist_iter_reversed(list)` | O(1) |

\* When allocators match, otherwise O(n)

---

## HashSet

Hash set with open addressing, automatic rehashing. **O(1)** average operations.

### Creation
```c
HashSet *set = hashset_create(sizeof(int));
HashSet *set = hashset_create_with_capacity(sizeof(double), 1024);
HashSet *set = hashset_create_with_hasher(sizeof(const char*), hash_fn);
HashSet *set = hashset_create_with_comparator(sizeof(MyStruct), my_compare);
HashSet *set = hashset_create_aligned(sizeof(float), 64);
HashSet *set = hashset_str();  /* automatic string handling mode */
HashSet *set = hashset_create_with_allocator(sizeof(int), alloc);
```

### Operations

| Operation | Function | Complexity |
|-----------|----------|:----------:|
| Insert | `hashset_insert(set, &val)` | O(1)* |
| Remove | `hashset_remove(set, &val)` | O(1)* |
| Contains | `hashset_contains(set, &val)` | O(1)* |
| Length | `hashset_len(set)` | O(1) |
| Capacity | `hashset_capacity(set)` | O(1) |
| Is empty | `hashset_is_empty(set)` | O(1) |
| Clear | `hashset_clear(set)` | O(n) |
| Clone | `hashset_clone(set)` | O(n) |
| Instance | `hashset_instance(set)` | O(1) |
| Merge | `hashset_merge(dst, src)` | O(n) |
| Union | `hashset_union(a, b)` | O(n) |
| Intersection | `hashset_intersection(a, b)` | O(n) |
| Difference | `hashset_difference(a, b)` | O(n) |
| Subset | `hashset_subset(a, b)` | O(n) |
| Equals | `hashset_equals(a, b)` | O(n) |
| Hash | `hashset_hash(set)` | O(1)** |
| Reserve | `hashset_reserve(set, expected)` | O(n) |
| Shrink | `hashset_shrink_to_fit(set)` | O(n) |
| To array | `hashset_to_array(set)` | O(n) |
| Iterate | `hashset_iter(set)` | O(1) |

\* Average case, O(n) worst case
\*\* Cached, recomputed only on mutation

---

## HashMap

Hash map with open addressing, automatic rehashing. **O(1)** average operations.

### Creation
```c
HashMap *map = hashmap_create(sizeof(int), sizeof(double));
HashMap *map = hashmap_create_with_capacity(sizeof(int), sizeof(double), 1024);
HashMap *map = hashmap_create_with_hasher(sizeof(const char*), sizeof(int), hash_fn);
HashMap *map = hashmap_create_with_comparator(sizeof(int), sizeof(double), kcmp, vcmp);
HashMap *map = hashmap_create_aligned(sizeof(int), sizeof(double), 64, 64);
HashMap *map = hashmap_str_any(sizeof(int));      /* string -> value */
HashMap *map = hashmap_any_str(sizeof(double));   /* key -> string */
HashMap *map = hashmap_str_str();                 /* string -> string */
HashMap *map = hashmap_create_with_allocator(sizeof(int), sizeof(double), alloc);
```

### Operations

| Operation | Function | Complexity |
|-----------|----------|:----------:|
| Insert | `hashmap_insert(map, &key, &val)` | O(1)* |
| Remove | `hashmap_remove(map, &key)` | O(1)* |
| Remove entry | `hashmap_remove_entry(map, &key, &val)` | O(1)* |
| Get | `hashmap_get(map, &key)` | O(1)* |
| Get or default | `hashmap_get_or_default(map, &key, &dval)` | O(1)* |
| Get mutable | `hashmap_get_mut(map, &key)` | O(1)* |
| Contains | `hashmap_contains(map, &key)` | O(1)* |
| Contains entry | `hashmap_contains_entry(map, &key, &val)` | O(1)* |
| Length | `hashmap_len(map)` | O(1) |
| Capacity | `hashmap_capacity(map)` | O(1) |
| Is empty | `hashmap_is_empty(map)` | O(1) |
| Clear | `hashmap_clear(map)` | O(n) |
| Clone | `hashmap_clone(map)` | O(n) |
| Instance | `hashmap_instance(map)` | O(1) |
| Merge | `hashmap_merge(dst, src)` | O(n) |
| Equals | `hashmap_equals(a, b)` | O(n) |
| Hash | `hashmap_hash(map)` | O(1)** |
| Keys | `hashmap_keys(map)` | O(n) |
| Values | `hashmap_values(map)` | O(n) |
| Reserve | `hashmap_reserve(map, expected)` | O(n) |
| Shrink | `hashmap_shrink_to_fit(map)` | O(n) |
| Iterate | `hashmap_iter(map)` | O(1) |
| Entry key | `hashmap_entry_key(map, entry)` | O(1) |
| Entry value | `hashmap_entry_value(map, entry)` | O(1) |

\* Average case, O(n) worst case
\*\* Cached, recomputed only on mutation