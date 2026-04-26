/**
 * @file hashset_test.c
 * @brief HashSet unit tests
 *
 * Tests cover:
 * - Creation and destruction (with/without capacity, comparator, hashser, allocator, alignment)
 * - Insert, remove, contains operations
 * - Set operations (union, intersection, difference)
 * - Subset and equality checks
 * - Merge (in-place union)
 * - Iteration over elements
 * - Array export (to_array)
 * - String mode (string elements)
 * - Edge cases (empty set, not found, duplicate insertion)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdint.h>
#include "assertion.h"
#include "timer.h"

#include <contain/hashset.h>

/* ============================================================================
 * Test Helpers
 * ============================================================================ */

/* Alignment test types */
typedef struct {
    alignas(32) uint8_t data[32];
} Aligned32;

typedef struct {
    alignas(64) uint8_t data[64];
} Aligned64;

/* Fill set with ints [0, n) */
static void hashset_fill(HashSet *set, int n) {
    for (int i = 0; i < n; i++) {
        int val = i;
        hashset_insert(set, &val);
    }
}

/* ============================================================================
 * Creation & Destruction
 * ============================================================================ */

int test_hashset_create(void) {
    HashSet *set = hashset_create(sizeof(int));
    ASSERT_NOT_NULL(set, "hashset_create failed");
    ASSERT_EQUAL(hashset_len(set), 0, "new set not empty");
    ASSERT_TRUE(hashset_capacity(set) >= HASHSET_MIN_CAPACITY, "wrong initial capacity");
    hashset_destroy(set);
    return 1;
}

int test_hashset_create_with_capacity(void) {
    HashSet *set = hashset_create_with_capacity(sizeof(int), 64);
    ASSERT_NOT_NULL(set, "create_with_capacity failed");
    ASSERT_TRUE(hashset_capacity(set) >= 64, "capacity too small");
    hashset_destroy(set);
    return 1;
}

int test_hashset_str_create(void) {
    HashSet *set = hashset_str();
    ASSERT_NOT_NULL(set, "hashset_str failed");
    ASSERT_TRUE(hashset_is_empty(set), "string set not empty");
    hashset_destroy(set);
    return 1;
}

int test_hashset_destroy_null(void) {
    hashset_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Alignment Tests
 * ============================================================================ */

int test_hashset_alignment_32(void) {
    HashSet *set = hashset_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(set, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 8; i++) {
        memset(&item, i + 1, sizeof(Aligned32));
        ASSERT_EQUAL(hashset_insert(set, &item), LC_OK, "insert failed");
    }

    Iterator it = hashset_iter(set);
    Aligned32 *p;
    while ((p = (Aligned32 *)iter_next(&it)) != NULL) {
        ASSERT_TRUE(((uintptr_t)p % 32) == 0, "element not 32-byte aligned");
    }

    hashset_destroy(set);
    return 1;
}

int test_hashset_alignment_64(void) {
    HashSet *set = hashset_create_aligned(sizeof(Aligned64), 64);
    ASSERT_NOT_NULL(set, "create_aligned failed");

    Aligned64 items[8];
    for (int i = 0; i < 8; i++) {
        memset(&items[i], i + 1, sizeof(Aligned64));
        ASSERT_EQUAL(hashset_insert(set, &items[i]), LC_OK, "insert failed");
    }

    Iterator it = hashset_iter(set);
    Aligned64 *p;
    while ((p = (Aligned64 *)iter_next(&it)) != NULL) {
        ASSERT_TRUE(((uintptr_t)p % 64) == 0, "element not 64-byte aligned");
    }

    hashset_destroy(set);
    return 1;
}

int test_hashset_alignment_after_rehash(void) {
    HashSet *set = hashset_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(set, "create_aligned failed");

    Aligned32 items[64];
    for (int i = 0; i < 64; i++) {
        memset(&items[i], i + 1, sizeof(Aligned32));
        ASSERT_EQUAL(hashset_insert(set, &items[i]), LC_OK, "insert failed");
    }

    Iterator it = hashset_iter(set);
    Aligned32 *p;
    while ((p = (Aligned32 *)iter_next(&it)) != NULL) {
        ASSERT_TRUE(((uintptr_t)p % 32) == 0, "element misaligned after rehash");
    }

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Insert Tests
 * ============================================================================ */

int test_hashset_insert(void) {
    HashSet *set = hashset_create(sizeof(int));
    int vals[] = {10, 20, 30, 40, 50};

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(hashset_insert(set, &vals[i]), LC_OK, "insert failed");
    }
    ASSERT_EQUAL(hashset_len(set), 5, "wrong length");

    hashset_destroy(set);
    return 1;
}

int test_hashset_insert_duplicate(void) {
    HashSet *set = hashset_create(sizeof(int));
    int val = 42;

    ASSERT_EQUAL(hashset_insert(set, &val), LC_OK, "first insert failed");
    ASSERT_EQUAL(hashset_insert(set, &val), LC_OK, "duplicate insert should be no-op");
    ASSERT_EQUAL(hashset_len(set), 1, "wrong length after duplicate");

    hashset_destroy(set);
    return 1;
}

int test_hashset_insert_strings(void) {
    HashSet *set = hashset_str();
    const char *strings[] = {"hello", "world", "test"};

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(hashset_insert(set, strings[i]), LC_OK, "insert string failed");
    }
    
    ASSERT_EQUAL(hashset_len(set), 3, "wrong length");

    hashset_destroy(set);
    return 1;
}

int test_hashset_insert_string_duplicate(void) {
    HashSet *set = hashset_str();

    ASSERT_EQUAL(hashset_insert(set, "hello"), LC_OK, "first insert failed");
    ASSERT_EQUAL(hashset_insert(set, "hello"), LC_OK, "duplicate should be no-op");
    ASSERT_EQUAL(hashset_len(set), 1, "wrong length after duplicate");

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Remove Tests
 * ============================================================================ */

int test_hashset_remove(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 10);

    int val = 5;
    ASSERT_EQUAL(hashset_remove(set, &val), LC_OK, "remove failed");
    ASSERT_EQUAL(hashset_len(set), 9, "wrong length after remove");
    ASSERT_FALSE(hashset_contains(set, &val), "removed element still present");

    hashset_destroy(set);
    return 1;
}

int test_hashset_remove_nonexistent(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 5);

    int val = 99;
    ASSERT_EQUAL(hashset_remove(set, &val), LC_ENOTFOUND, "remove nonexistent should fail");
    ASSERT_EQUAL(hashset_len(set), 5, "wrong length after failed remove");

    hashset_destroy(set);
    return 1;
}

int test_hashset_remove_strings(void) {
    HashSet *set = hashset_str();
    hashset_insert(set, "hello");
    hashset_insert(set, "world");
    hashset_insert(set, "test");

    ASSERT_EQUAL(hashset_remove(set, "world"), LC_OK, "remove string failed");
    ASSERT_EQUAL(hashset_len(set), 2, "wrong length after remove");
    ASSERT_FALSE(hashset_contains(set, "world"), "removed string still present");
    ASSERT_TRUE(hashset_contains(set, "hello"), "other string missing");

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Contains Tests
 * ============================================================================ */

int test_hashset_contains(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 10);

    for (int i = 0; i < 10; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(set, &val), "should contain value");
    }

    int absent = 99;
    ASSERT_FALSE(hashset_contains(set, &absent), "should not contain absent");

    hashset_destroy(set);
    return 1;
}

int test_hashset_contains_strings(void) {
    HashSet *set = hashset_str();
    hashset_insert(set, "hello");
    hashset_insert(set, "world");

    ASSERT_TRUE(hashset_contains(set, "hello"), "should contain hello");
    ASSERT_TRUE(hashset_contains(set, "world"), "should contain world");
    ASSERT_FALSE(hashset_contains(set, "missing"), "should not contain missing");

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Clear Tests
 * ============================================================================ */

int test_hashset_clear(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 100);

    ASSERT_EQUAL(hashset_clear(set), LC_OK, "clear failed");
    ASSERT_TRUE(hashset_is_empty(set), "not empty after clear");

    int val = 42;
    hashset_insert(set, &val);
    ASSERT_EQUAL(hashset_len(set), 1, "cannot insert after clear");

    hashset_destroy(set);
    return 1;
}

int test_hashset_clear_strings(void) {
    HashSet *set = hashset_str();
    hashset_insert(set, "hello");
    hashset_insert(set, "world");

    ASSERT_EQUAL(hashset_clear(set), LC_OK, "clear strings failed");
    ASSERT_TRUE(hashset_is_empty(set), "not empty after clear");

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Reserve & Shrink Tests
 * ============================================================================ */

int test_hashset_reserve(void) {
    HashSet *set = hashset_create(sizeof(int));
    ASSERT_EQUAL(hashset_reserve(set, 256), LC_OK, "reserve failed");
    ASSERT_TRUE(hashset_capacity(set) >= 256, "capacity not increased");

    hashset_fill(set, 10);
    for (int i = 0; i < 10; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(set, &val), "element lost after reserve");
    }

    hashset_destroy(set);
    return 1;
}

int test_hashset_shrink_to_fit2(void) {
    HashSet *set = hashset_create_with_capacity(sizeof(int), 256);
    hashset_fill(set, 10);

    size_t cap_before = hashset_capacity(set);
    ASSERT_EQUAL(hashset_shrink_to_fit(set), LC_OK, "shrink_to_fit failed");
    ASSERT_TRUE(hashset_capacity(set) < cap_before, "capacity not reduced");

    for (int i = 0; i < 10; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(set, &val), "element lost after shrink");
    }

    hashset_destroy(set);
    return 1;
}

int test_hashset_shrink_to_fit(void) {
    HashSet *set = hashset_create_with_capacity(sizeof(int), 256);
    hashset_fill(set, 10);
    
    size_t cap_before = hashset_capacity(set);
    printf("Capacity before: %zu\n", cap_before);  // Should be 256
    
    ASSERT_EQUAL(hashset_shrink_to_fit(set), LC_OK, "shrink_to_fit failed");
    
    size_t cap_after = hashset_capacity(set);
    printf("Capacity after: %zu\n", cap_after);  // Should be 16
    
    ASSERT_TRUE(cap_after < cap_before, "capacity not reduced");
    
    for (int i = 0; i < 10; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(set, &val), "element lost after shrink");
    }

    hashset_destroy(set);
    return 1;
    // ... rest of test
}
/* ============================================================================
 * Load Factor Tests
 * ============================================================================ */

int test_hashset_load_factor_expand(void) {
    HashSet *set = hashset_create_with_capacity(sizeof(int), HASHSET_MIN_CAPACITY);
    size_t initial_cap = hashset_capacity(set);

    int n = (int)(initial_cap * 3 / 4) + 2;
    hashset_fill(set, n);

    ASSERT_TRUE(hashset_capacity(set) > initial_cap, "capacity should have expanded");

    for (int i = 0; i < n; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(set, &val), "element lost after expand");
    }

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Equals Tests
 * ============================================================================ */

int test_hashset_equals_identical(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    hashset_fill(a, 5);
    hashset_fill(b, 5);

    ASSERT_TRUE(hashset_equals(a, b), "identical sets should be equal");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

int test_hashset_equals_different_order(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    int vals_a[] = {1, 2, 3, 4, 5};
    int vals_b[] = {5, 4, 3, 2, 1};

    for (int i = 0; i < 5; i++) hashset_insert(a, &vals_a[i]);
    for (int i = 0; i < 5; i++) hashset_insert(b, &vals_b[i]);

    ASSERT_TRUE(hashset_equals(a, b), "different insertion order should be equal");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

int test_hashset_equals_different_values(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    hashset_fill(a, 5);
    hashset_fill(b, 4);

    int extra = 99;
    hashset_insert(b, &extra);
    ASSERT_FALSE(hashset_equals(a, b), "different values should not be equal");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

int test_hashset_equals_strings(void) {
    HashSet *a = hashset_str();
    HashSet *b = hashset_str();
    hashset_insert(a, "hello");
    hashset_insert(a, "world");
    hashset_insert(b, "world");
    hashset_insert(b, "hello");

    ASSERT_TRUE(hashset_equals(a, b), "string sets with same content should be equal");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

/* ============================================================================
 * Subset Tests
 * ============================================================================ */

int test_hashset_subset(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    hashset_fill(a, 3);
    hashset_fill(b, 5);

    ASSERT_TRUE(hashset_subset(a, b), "a should be subset of b");
    ASSERT_FALSE(hashset_subset(b, a), "b should not be subset of a");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

/* ============================================================================
 * Hash Tests
 * ============================================================================ */

int test_hashset_hash(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    hashset_fill(a, 10);
    hashset_fill(b, 10);

    size_t ha = hashset_hash(a);
    size_t hb = hashset_hash(b);
    ASSERT_EQUAL(ha, hb, "identical sets different hash");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

int test_hashset_hash_order_independent(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    int vals_a[] = {1, 2, 3, 4, 5};
    int vals_b[] = {5, 4, 3, 2, 1};

    for (int i = 0; i < 5; i++) hashset_insert(a, &vals_a[i]);
    for (int i = 0; i < 5; i++) hashset_insert(b, &vals_b[i]);

    ASSERT_EQUAL(hashset_hash(a), hashset_hash(b), "order should not affect hash");

    hashset_destroy(a);
    hashset_destroy(b);
    return 1;
}

int test_hashset_hash_cached(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 5);

    size_t h1 = hashset_hash(set);
    size_t h2 = hashset_hash(set);
    ASSERT_EQUAL(h1, h2, "cached hash should be consistent");

    int val = 99;
    hashset_insert(set, &val);
    size_t h3 = hashset_hash(set);
    ASSERT_NOT_EQUAL(h1, h3, "hash should change after mutation");

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Clone Tests
 * ============================================================================ */

int test_hashset_clone(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 10);

    HashSet *clone = hashset_clone(set);
    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(hashset_len(clone), 10, "clone wrong length");

    for (int i = 0; i < 10; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(clone, &val), "clone missing element");
    }

    int newval = 99;
    hashset_insert(clone, &newval);
    ASSERT_FALSE(hashset_contains(set, &newval), "original changed after clone modification");

    hashset_destroy(set);
    hashset_destroy(clone);
    return 1;
}

int test_hashset_clone_strings(void) {
    HashSet *set = hashset_str();
    hashset_insert(set, "hello");
    hashset_insert(set, "world");

    HashSet *clone = hashset_clone(set);
    ASSERT_NOT_NULL(clone, "clone strings failed");
    ASSERT_TRUE(hashset_contains(clone, "hello"), "clone missing string");

    hashset_destroy(set);
    hashset_destroy(clone);
    return 1;
}

/* ============================================================================
 * Merge Tests
 * ============================================================================ */

int test_hashset_merge(void) {
    HashSet *dst = hashset_create(sizeof(int));
    HashSet *src = hashset_create(sizeof(int));
    for (int i = 0; i < 5; i++) {
        int val = i;
        hashset_insert(dst, &val);
    }
    for (int i = 5; i < 10; i++) {
        int val = i;
        hashset_insert(src, &val);
    }

    ASSERT_EQUAL(hashset_merge(dst, src), LC_OK, "merge failed");
    ASSERT_EQUAL(hashset_len(dst), 10, "wrong length after merge");

    for (int i = 0; i < 10; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(dst, &val), "missing element after merge");
    }

    hashset_destroy(dst);
    hashset_destroy(src);
    return 1;
}

int test_hashset_merge_overlapping(void) {
    HashSet *dst = hashset_create(sizeof(int));
    HashSet *src = hashset_create(sizeof(int));
    for (int i = 0; i < 5; i++) hashset_insert(dst, &i);
    for (int i = 3; i < 8; i++) hashset_insert(src, &i);

    ASSERT_EQUAL(hashset_merge(dst, src), LC_OK, "merge overlapping failed");
    ASSERT_EQUAL(hashset_len(dst), 8, "wrong length after merge");

    hashset_destroy(dst);
    hashset_destroy(src);
    return 1;
}

int test_hashset_merge_strings(void) {
    HashSet *dst = hashset_str();
    HashSet *src = hashset_str();
    hashset_insert(dst, "hello");
    hashset_insert(dst, "world");
    hashset_insert(src, "test");
    hashset_insert(src, "merge");

    ASSERT_EQUAL(hashset_merge(dst, src), LC_OK, "merge strings failed");
    ASSERT_EQUAL(hashset_len(dst), 4, "wrong length after merge");
    ASSERT_TRUE(hashset_contains(dst, "test"), "missing merged string");

    hashset_destroy(dst);
    hashset_destroy(src);
    return 1;
}

/* ============================================================================
 * Set Operations Tests
 * ============================================================================ */

int test_hashset_union(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < 5; i++) hashset_insert(a, &i);
    for (int i = 3; i < 8; i++) hashset_insert(b, &i);

    HashSet *u = hashset_union(a, b);
    ASSERT_NOT_NULL(u, "union failed");
    ASSERT_EQUAL(hashset_len(u), 8, "wrong union size");

    for (int i = 0; i < 8; i++) {
        int val = i;
        ASSERT_TRUE(hashset_contains(u, &val), "union missing element");
    }

    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(u);
    return 1;
}

int test_hashset_intersection(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < 5; i++) hashset_insert(a, &i);
    for (int i = 3; i < 8; i++) hashset_insert(b, &i);

    HashSet *inter = hashset_intersection(a, b);
    ASSERT_NOT_NULL(inter, "intersection failed");
    ASSERT_EQUAL(hashset_len(inter), 2, "wrong intersection size");
    ASSERT_TRUE(hashset_contains(inter, &(int){3}), "missing intersection element");
    ASSERT_TRUE(hashset_contains(inter, &(int){4}), "missing intersection element");

    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(inter);
    return 1;
}

int test_hashset_difference(void) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < 5; i++) hashset_insert(a, &i);
    for (int i = 3; i < 8; i++) hashset_insert(b, &i);

    HashSet *diff = hashset_difference(a, b);
    ASSERT_NOT_NULL(diff, "difference failed");
    ASSERT_EQUAL(hashset_len(diff), 3, "wrong difference size");
    ASSERT_TRUE(hashset_contains(diff, &(int){0}), "missing difference element");
    ASSERT_TRUE(hashset_contains(diff, &(int){1}), "missing difference element");
    ASSERT_TRUE(hashset_contains(diff, &(int){2}), "missing difference element");

    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(diff);
    return 1;
}

int test_hashset_set_ops_strings(void) {
    HashSet *a = hashset_str();
    HashSet *b = hashset_str();
    hashset_insert(a, "apple");
    hashset_insert(a, "banana");
    hashset_insert(a, "cherry");
    hashset_insert(b, "banana");
    hashset_insert(b, "cherry");
    hashset_insert(b, "date");

    HashSet *u = hashset_union(a, b);
    HashSet *inter = hashset_intersection(a, b);
    HashSet *diff = hashset_difference(a, b);

    ASSERT_NOT_NULL(u, "string union failed");
    ASSERT_NOT_NULL(inter, "string intersection failed");
    ASSERT_NOT_NULL(diff, "string difference failed");

    ASSERT_EQUAL(hashset_len(u), 4, "wrong union size");
    ASSERT_EQUAL(hashset_len(inter), 2, "wrong intersection size");
    ASSERT_EQUAL(hashset_len(diff), 1, "wrong difference size");

    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(u);
    hashset_destroy(inter);
    hashset_destroy(diff);
    return 1;
}

/* ============================================================================
 * To Array Tests
 * ============================================================================ */

int test_hashset_to_array(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 10);

    Array *arr = hashset_to_array(set);
    ASSERT_NOT_NULL(arr, "to_array failed");
    ASSERT_EQUAL(arr->len, 10, "array wrong length");

    bool found[10] = {false};
    for (size_t i = 0; i < 10; i++) {
        int val = *(int *)((uint8_t *)arr->items + i * arr->stride);
        if (val >= 0 && val < 10) found[val] = true;
    }
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(found[i], "array missing element");
    }

    free(arr);
    hashset_destroy(set);
    return 1;
}

int test_hashset_to_array_strings(void) {
    HashSet *set = hashset_str();
    hashset_insert(set, "hello");
    hashset_insert(set, "world");

    Array *arr = hashset_to_array(set);
    ASSERT_NOT_NULL(arr, "to_array strings failed");
    ASSERT_EQUAL(arr->len, 2, "array wrong length");

    char **table = (char **)arr->items;
    bool found_hello = false, found_world = false;
    for (size_t i = 0; i < 2; i++) {
        if (strcmp(table[i], "hello") == 0) found_hello = true;
        if (strcmp(table[i], "world") == 0) found_world = true;
    }
    ASSERT_TRUE(found_hello, "array missing hello");
    ASSERT_TRUE(found_world, "array missing world");

    free(arr);
    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Iterator Tests
 * ============================================================================ */

int test_hashset_iterator(void) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_fill(set, 10);

    Iterator it = hashset_iter(set);
    bool found[10] = {false};
    int count = 0;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        if (*p >= 0 && *p < 10) found[*p] = true;
        count++;
    }
    ASSERT_EQUAL(count, 10, "iterator wrong count");
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(found[i], "iterator missing element");
    }

    hashset_destroy(set);
    return 1;
}

int test_hashset_iterator_strings(void) {
    HashSet *set = hashset_str();
    hashset_insert(set, "alpha");
    hashset_insert(set, "beta");
    hashset_insert(set, "gamma");

    Iterator it = hashset_iter(set);
    int count = 0;
    char *p;
    while ((p = (char *)iter_next(&it)) != NULL) {
        count++;
    }
    ASSERT_EQUAL(count, 3, "iterator wrong count");

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests
 * ============================================================================ */

int test_hashset_large_insert(void) {
    const size_t N = 1000000;
    HashSet *set = hashset_create(sizeof(int));
    ASSERT_NOT_NULL(set, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_EQUAL(hashset_insert(set, &val), LC_OK, "insert failed at %zu", i);
    }
    double insert_time = timer_elapsed(&t);
    printf("    Insert %zu elements: %.3f ms\n", N, insert_time * 1000);

    ASSERT_EQUAL(hashset_len(set), N, "wrong length after inserts");

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_TRUE(hashset_contains(set, &val), "missing element at %zu", i);
    }
    double contains_time = timer_elapsed(&t);
    printf("    Contains %zu elements: %.3f ms\n", N, contains_time * 1000);

    hashset_destroy(set);
    return 1;
}

int test_hashset_large_clone(void) {
    const size_t N = 1000000;
    HashSet *set = hashset_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        hashset_insert(set, &val);
    }

    Timer t = timer_start();
    HashSet *clone = hashset_clone(set);
    double clone_time = timer_elapsed(&t);
    printf("    Clone %zu elements: %.3f ms\n", N, clone_time * 1000);

    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(hashset_len(clone), N, "clone wrong length");

    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_TRUE(hashset_contains(clone, &val), "clone missing element at %zu", i);
    }

    hashset_destroy(set);
    hashset_destroy(clone);
    return 1;
}

int test_hashset_large_strings(void) {
    const size_t N = 100000;
    HashSet *set = hashset_str();
    char buf[32];

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_EQUAL(hashset_insert(set, buf), LC_OK, "insert string failed at %zu", i);
    }
    double insert_time = timer_elapsed(&t);
    printf("    Insert %zu strings: %.3f ms\n", N, insert_time * 1000);

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_TRUE(hashset_contains(set, buf), "missing string at %zu", i);
    }
    double contains_time = timer_elapsed(&t);
    printf("    Contains %zu strings: %.3f ms\n", N, contains_time * 1000);

    hashset_destroy(set);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  HashSet Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Creation & Destruction */
    TEST(test_hashset_create);
    TEST(test_hashset_create_with_capacity);
    TEST(test_hashset_str_create);
    TEST(test_hashset_destroy_null);

    /* Alignment */
    TEST(test_hashset_alignment_32);
    TEST(test_hashset_alignment_64);
    TEST(test_hashset_alignment_after_rehash);

    /* Insert */
    TEST(test_hashset_insert);
    TEST(test_hashset_insert_duplicate);
    TEST(test_hashset_insert_strings);
    TEST(test_hashset_insert_string_duplicate);

    /* Remove */
    TEST(test_hashset_remove);
    TEST(test_hashset_remove_nonexistent);
    TEST(test_hashset_remove_strings);

    /* Contains */
    TEST(test_hashset_contains);
    TEST(test_hashset_contains_strings);

    /* Clear */
    TEST(test_hashset_clear);
    TEST(test_hashset_clear_strings);

    /* Reserve & Shrink */
    TEST(test_hashset_reserve);
    TEST(test_hashset_shrink_to_fit);

    /* Load Factor */
    TEST(test_hashset_load_factor_expand);

    /* Equals */
    TEST(test_hashset_equals_identical);
    TEST(test_hashset_equals_different_order);
    TEST(test_hashset_equals_different_values);
    TEST(test_hashset_equals_strings);

    /* Subset */
    TEST(test_hashset_subset);

    /* Hash */
    TEST(test_hashset_hash);
    TEST(test_hashset_hash_order_independent);
    TEST(test_hashset_hash_cached);

    /* Clone */
    TEST(test_hashset_clone);
    TEST(test_hashset_clone_strings);

    /* Merge */
    TEST(test_hashset_merge);
    TEST(test_hashset_merge_overlapping);
    TEST(test_hashset_merge_strings);

    /* Set Operations */
    TEST(test_hashset_union);
    TEST(test_hashset_intersection);
    TEST(test_hashset_difference);
    TEST(test_hashset_set_ops_strings);

    /* To Array */
    TEST(test_hashset_to_array);
    TEST(test_hashset_to_array_strings);

    /* Iterator */
    TEST(test_hashset_iterator);
    TEST(test_hashset_iterator_strings);

    /* Large Dataset Tests */
    printf("\n--- Large Dataset Tests ---\n");
    TEST(test_hashset_large_insert);
    TEST(test_hashset_large_clone);
    TEST(test_hashset_large_strings);

    double total_time = timer_elapsed(&total);

    printf("\n========================================\n");
    printf("  Results\n");
    printf("========================================\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Passed:       %d\n", tests_passed);
    printf("  Failed:       %d\n", tests_run - tests_passed);
    printf("  Duration:     %.3f seconds\n", total_time);
    printf("========================================\n");

    return tests_passed == tests_run ? 0 : 1;
}