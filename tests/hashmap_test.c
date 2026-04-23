/**
 * @file hashmap_test.c
 * @brief HashMap unit tests
 *
 * Tests cover:
 * - Creation and destruction (with/without capacity, comparator, hashser, allocator, alignment)
 * - Insert, update, remove operations
 * - Lookup (get, get_or_default, contains)
 * - Iteration over entries
 * - Array export (keys, values)
 * - Merge, clone, hash, equals
 * - String mode (string keys and/or values)
 * - Edge cases (empty map, not found, NULL handling)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdint.h>
#include "assertion.h"
#include "timer.h"

#define HASHMAP_IMPLEMENTATION
#include <contain/hashmap.h>

/* ============================================================================
 * Test Helpers
 * ============================================================================ */

static int cmp_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int cmp_str_ptr(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

static size_t hash_int(const void *item, size_t size) {
    (void)size;
    uint32_t x = (uint32_t)*(const int *)item;
    x ^= x >> 16; x *= 0x45d9f3b; x ^= x >> 16;
    return (size_t)x;
}

/* Alignment test types */
typedef struct {
    alignas(32) uint8_t data[32];
} Aligned32;

typedef struct {
    alignas(64) uint8_t data[64];
} Aligned64;

/* Fill map with n int→int entries [0,n) → [100,100+n) */
static void hashmap_fill(HashMap *map, int n) {
    for (int i = 0; i < n; i++) {
        int val = i + 100;
        hashmap_insert(map, &i, &val);
    }
}

/* ============================================================================
 * Creation & Destruction
 * ============================================================================ */

int test_hashmap_create(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    ASSERT_NOT_NULL(map, "hashmap_create failed");
    ASSERT_EQUAL(hashmap_len(map), 0, "new map not empty");
    ASSERT_TRUE(hashmap_capacity(map) >= HASHMAP_MIN_CAPACITY, "wrong initial capacity");
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_create_with_capacity(void) {
    HashMap *map = hashmap_create_with_capacity(sizeof(int), sizeof(int), 64);
    ASSERT_NOT_NULL(map, "create_with_capacity failed");
    ASSERT_TRUE(hashmap_capacity(map) >= 64, "capacity too small");
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_str_str(void) {
    HashMap *map = hashmap_str_str();
    ASSERT_NOT_NULL(map, "hashmap_str_str failed");
    ASSERT_TRUE(hashmap_is_empty(map), "string map not empty");
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_str_any(void) {
    HashMap *map = hashmap_str_any(sizeof(int));
    ASSERT_NOT_NULL(map, "hashmap_str_any failed");
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_any_str(void) {
    HashMap *map = hashmap_any_str(sizeof(int));
    ASSERT_NOT_NULL(map, "hashmap_any_str failed");
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_destroy_null(void) {
    hashmap_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Insert Tests
 * ============================================================================ */

int test_hashmap_insert(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    ASSERT_EQUAL(hashmap_len(map), 5, "wrong length");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)hashmap_get(map, &i), i + 100, "wrong value");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_insert_update(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    int k = 42, v1 = 100, v2 = 200;

    ASSERT_EQUAL(hashmap_insert(map, &k, &v1), LC_OK, "first insert failed");
    ASSERT_EQUAL(hashmap_insert(map, &k, &v2), LC_OK, "update failed");
    ASSERT_EQUAL(hashmap_len(map), 1, "wrong length after update");
    ASSERT_EQUAL(*(int *)hashmap_get(map, &k), 200, "wrong updated value");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_insert_str_str(void) {
    HashMap *map = hashmap_str_str();

    ASSERT_EQUAL(hashmap_insert(map, "hello", "world"), LC_OK, "insert string failed");
    ASSERT_EQUAL(hashmap_insert(map, "foo", "bar"), LC_OK, "insert string failed");
    ASSERT_EQUAL(hashmap_len(map), 2, "wrong length");

    ASSERT_STR_EQUAL((const char *)hashmap_get(map, "hello"), "world", "wrong value");
    ASSERT_STR_EQUAL((const char *)hashmap_get(map, "foo"), "bar", "wrong value");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_insert_str_any(void) {
    HashMap *map = hashmap_str_any(sizeof(int));
    int v1 = 42, v2 = 99;

    ASSERT_EQUAL(hashmap_insert(map, "alpha", &v1), LC_OK, "insert failed");
    ASSERT_EQUAL(hashmap_insert(map, "beta", &v2), LC_OK, "insert failed");
    ASSERT_EQUAL(hashmap_len(map), 2, "wrong length");
    ASSERT_EQUAL(*(int *)hashmap_get(map, "alpha"), 42, "wrong value");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_insert_any_str(void) {
    HashMap *map = hashmap_any_str(sizeof(int));
    int k1 = 1, k2 = 2;

    ASSERT_EQUAL(hashmap_insert(map, &k1, "one"), LC_OK, "insert failed");
    ASSERT_EQUAL(hashmap_insert(map, &k2, "two"), LC_OK, "insert failed");
    ASSERT_EQUAL(hashmap_len(map), 2, "wrong length");
    ASSERT_STR_EQUAL((const char *)hashmap_get(map, &k1), "one", "wrong value");

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Get Tests
 * ============================================================================ */

int test_hashmap_get(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    for (int i = 0; i < 5; i++) {
        int *val = (int *)hashmap_get(map, &i);
        ASSERT_NOT_NULL(val, "get returned NULL");
        ASSERT_EQUAL(*val, i + 100, "wrong value");
    }

    int absent = 99;
    ASSERT_NULL(hashmap_get(map, &absent), "get should return NULL for missing");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_get_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "name", "Alice");
    hashmap_insert(map, "city", "Lagos");

    ASSERT_STR_EQUAL((const char *)hashmap_get(map, "name"), "Alice", "wrong value");
    ASSERT_STR_EQUAL((const char *)hashmap_get(map, "city"), "Lagos", "wrong value");
    ASSERT_NULL(hashmap_get(map, "missing"), "get should return NULL for missing");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_get_or_default(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);  // keys 0-4 → values 100-104
    
    int key1 = 1;
    int default_val = 0;
    const int *val = hashmap_get_or_default(map, &key1, &default_val);
    ASSERT_NOT_NULL(val, "get_or_default failed");
    ASSERT_EQUAL(*val, 101, "wrong value");  // 1 + 100 = 101
    
    int key2 = 99;
    val = hashmap_get_or_default(map, &key2, &default_val);
    ASSERT_NOT_NULL(val, "get_or_default failed");
    ASSERT_EQUAL(*val, 0, "default value not returned");
    ASSERT_TRUE(val == &default_val, "should return pointer to default");
    
    int key3 = 2;
    val = hashmap_get_or_default(map, &key3, &default_val);
    ASSERT_NOT_NULL(val, "get_or_default failed");
    ASSERT_EQUAL(*val, 102, "wrong value");  // 2 + 100 = 102
    
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_get_or_default_strings(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "hello", "world");
    hashmap_insert(map, "foo", "bar");
    hashmap_insert(map, "c", "programming");

    const char *key3 = "c";
    const char *val = hashmap_get_or_default(map, key3, "default");  // no & here
    ASSERT_NOT_NULL(val, "get_or_default failed");
    ASSERT_STR_EQUAL(val, "programming", "wrong value");

    const char *key4 = "d";
    val = hashmap_get_or_default(map, key4, "default");
    ASSERT_NOT_NULL(val, "get_or_default failed");
    ASSERT_STR_EQUAL(val, "default", "wrong value");

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Remove Tests
 * ============================================================================ */

int test_hashmap_remove(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    int k = 3;
    ASSERT_EQUAL(hashmap_remove(map, &k), LC_OK, "remove failed");
    ASSERT_EQUAL(hashmap_len(map), 4, "wrong length after remove");
    ASSERT_NULL(hashmap_get(map, &k), "removed key still present");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_remove_nonexistent(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    int absent = 99;
    ASSERT_EQUAL(hashmap_remove(map, &absent), LC_ENOTFOUND, "remove nonexistent should fail");
    ASSERT_EQUAL(hashmap_len(map), 5, "wrong length after failed remove");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_remove_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "a", "1");
    hashmap_insert(map, "b", "2");
    hashmap_insert(map, "c", "3");

    ASSERT_EQUAL(hashmap_remove(map, "b"), LC_OK, "remove string failed");
    ASSERT_EQUAL(hashmap_len(map), 2, "wrong length after remove");
    ASSERT_NULL(hashmap_get(map, "b"), "removed key still present");
    ASSERT_NOT_NULL(hashmap_get(map, "a"), "other key missing");

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Contains Tests
 * ============================================================================ */

int test_hashmap_contains(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(hashmap_contains(map, &i), "should contain key");
    }

    int absent = 99;
    ASSERT_FALSE(hashmap_contains(map, &absent), "should not contain absent");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_contains_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "hello", "world");

    ASSERT_TRUE(hashmap_contains(map, "hello"), "should contain hello");
    ASSERT_FALSE(hashmap_contains(map, "missing"), "should not contain missing");

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Clear Tests
 * ============================================================================ */

int test_hashmap_clear(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 100);

    ASSERT_EQUAL(hashmap_clear(map), LC_OK, "clear failed");
    ASSERT_TRUE(hashmap_is_empty(map), "not empty after clear");

    int k = 42, v = 99;
    hashmap_insert(map, &k, &v);
    ASSERT_EQUAL(hashmap_len(map), 1, "cannot insert after clear");

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_clear_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "hello", "world");
    hashmap_insert(map, "foo", "bar");

    ASSERT_EQUAL(hashmap_clear(map), LC_OK, "clear strings failed");
    ASSERT_TRUE(hashmap_is_empty(map), "not empty after clear");

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Reserve & Capacity Tests
 * ============================================================================ */

int test_hashmap_reserve(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    ASSERT_EQUAL(hashmap_reserve(map, 256), LC_OK, "reserve failed");
    ASSERT_TRUE(hashmap_capacity(map) >= 256, "capacity not increased");

    hashmap_fill(map, 10);
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(hashmap_contains(map, &i), "element lost after reserve");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_load_factor_expand(void) {
    HashMap *map = hashmap_create_with_capacity(sizeof(int), sizeof(int), HASHMAP_MIN_CAPACITY);
    size_t initial_cap = hashmap_capacity(map);

    int n = (int)(initial_cap * 3 / 4) + 2;
    hashmap_fill(map, n);

    ASSERT_TRUE(hashmap_capacity(map) > initial_cap, "capacity should have expanded");

    for (int i = 0; i < n; i++) {
        ASSERT_TRUE(hashmap_contains(map, &i), "element lost after expand");
    }

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Equals Tests
 * ============================================================================ */

int test_hashmap_equals_identical(void) {
    HashMap *a = hashmap_create(sizeof(int), sizeof(int));
    HashMap *b = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(a, 5);
    hashmap_fill(b, 5);

    ASSERT_TRUE(hashmap_equals(a, b), "identical maps should be equal");

    hashmap_destroy(a);
    hashmap_destroy(b);
    return 1;
}

int test_hashmap_equals_different_order(void) {
    HashMap *a = hashmap_create(sizeof(int), sizeof(int));
    HashMap *b = hashmap_create(sizeof(int), sizeof(int));
    int keys[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; i++) {
        int v = keys[i] * 10;
        hashmap_insert(a, &keys[i], &v);
    }
    for (int i = 4; i >= 0; i--) {
        int v = keys[i] * 10;
        hashmap_insert(b, &keys[i], &v);
    }

    ASSERT_TRUE(hashmap_equals(a, b), "different order should be equal");

    hashmap_destroy(a);
    hashmap_destroy(b);
    return 1;
}

int test_hashmap_equals_different_values(void) {
    HashMap *a = hashmap_create(sizeof(int), sizeof(int));
    HashMap *b = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(a, 5);
    hashmap_fill(b, 5);

    int k = 2, new_v = 999;
    hashmap_insert(b, &k, &new_v);
    ASSERT_FALSE(hashmap_equals(a, b), "different values should not be equal");

    hashmap_destroy(a);
    hashmap_destroy(b);
    return 1;
}

int test_hashmap_equals_str_str(void) {
    HashMap *a = hashmap_str_str();
    HashMap *b = hashmap_str_str();
    hashmap_insert(a, "x", "1");
    hashmap_insert(a, "y", "2");
    hashmap_insert(b, "y", "2");
    hashmap_insert(b, "x", "1");

    ASSERT_TRUE(hashmap_equals(a, b), "string maps with same content should be equal");

    hashmap_destroy(a);
    hashmap_destroy(b);
    return 1;
}

/* ============================================================================
 * Hash Tests
 * ============================================================================ */

int test_hashmap_hash(void) {
    HashMap *a = hashmap_create(sizeof(int), sizeof(int));
    HashMap *b = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(a, 10);
    hashmap_fill(b, 10);

    size_t ha = hashmap_hash(a);
    size_t hb = hashmap_hash(b);
    ASSERT_EQUAL(ha, hb, "identical maps different hash");

    hashmap_destroy(a);
    hashmap_destroy(b);
    return 1;
}

int test_hashmap_hash_order_independent(void) {
    HashMap *a = hashmap_create(sizeof(int), sizeof(int));
    HashMap *b = hashmap_create(sizeof(int), sizeof(int));
    int keys[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; i++) {
        int v = keys[i] * 10;
        hashmap_insert(a, &keys[i], &v);
    }
    for (int i = 4; i >= 0; i--) {
        int v = keys[i] * 10;
        hashmap_insert(b, &keys[i], &v);
    }

    ASSERT_EQUAL(hashmap_hash(a), hashmap_hash(b), "order should not affect hash");

    hashmap_destroy(a);
    hashmap_destroy(b);
    return 1;
}

int test_hashmap_hash_cached(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    size_t h1 = hashmap_hash(map);
    size_t h2 = hashmap_hash(map);
    ASSERT_EQUAL(h1, h2, "cached hash should be consistent");

    int k = 0, new_v = 999;
    hashmap_insert(map, &k, &new_v);
    size_t h3 = hashmap_hash(map);
    ASSERT_NOT_EQUAL(h1, h3, "hash should change after mutation");

    hashmap_destroy(map);
    return 1;
}


/* ============================================================================
 * Clone Tests
 * ============================================================================ */

int test_hashmap_clone(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 10);

    HashMap *clone = hashmap_clone(map);
    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(hashmap_len(clone), 10, "clone wrong length");

    for (int i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)hashmap_get(clone, &i), i + 100, "clone wrong value");
    }

    int k = 0, new_v = 999;
    hashmap_insert(clone, &k, &new_v);
    ASSERT_EQUAL(*(int *)hashmap_get(map, &k), 100, "original changed after clone modification");

    hashmap_destroy(map);
    hashmap_destroy(clone);
    return 1;
}

int test_hashmap_clone_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "hello", "world");
    hashmap_insert(map, "foo", "bar");

    HashMap *clone = hashmap_clone(map);
    ASSERT_NOT_NULL(clone, "clone strings failed");
    ASSERT_STR_EQUAL((const char *)hashmap_get(clone, "hello"), "world", "clone wrong value");

    hashmap_destroy(map);
    hashmap_destroy(clone);
    return 1;
}

/* ============================================================================
 * Merge Tests
 * ============================================================================ */

int test_hashmap_merge(void) {
    HashMap *dst = hashmap_create(sizeof(int), sizeof(int));
    HashMap *src = hashmap_create(sizeof(int), sizeof(int));
    
    for (int i = 0; i < 5; i++) {
        int val = i + 100;
        hashmap_insert(dst, &i, &val);
    }
    for (int i = 5; i < 10; i++) {
        int val = i + 100;
        hashmap_insert(src, &i, &val);
    }

    ASSERT_EQUAL(hashmap_merge(dst, src), LC_OK, "merge failed");
    ASSERT_EQUAL(hashmap_len(dst), 10, "wrong length after merge");

    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(hashmap_contains(dst, &i), "missing element after merge");
    }

    hashmap_destroy(dst);
    hashmap_destroy(src);
    return 1;
}

int test_hashmap_merge_overlapping(void) {
    HashMap *dst = hashmap_create(sizeof(int), sizeof(int));
    HashMap *src = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < 5; i++) hashmap_fill(dst, i);
    for (int i = 3; i < 8; i++) {
        int val = i + 200;
        hashmap_insert(src, &i, &val);
    }

    ASSERT_EQUAL(hashmap_merge(dst, src), LC_OK, "merge overlapping failed");
    ASSERT_EQUAL(hashmap_len(dst), 8, "wrong length after merge");

    int k3 = 3, k4 = 4;
    ASSERT_EQUAL(*(int *)hashmap_get(dst, &k3), 203, "overlapping key should have src value");
    ASSERT_EQUAL(*(int *)hashmap_get(dst, &k4), 204, "overlapping key should have src value");

    hashmap_destroy(dst);
    hashmap_destroy(src);
    return 1;
}

int test_hashmap_merge_str_str(void) {
    HashMap *dst = hashmap_str_str();
    HashMap *src = hashmap_str_str();
    hashmap_insert(dst, "a", "1");
    hashmap_insert(dst, "b", "2");
    hashmap_insert(src, "c", "3");
    hashmap_insert(src, "d", "4");

    ASSERT_EQUAL(hashmap_merge(dst, src), LC_OK, "merge strings failed");
    ASSERT_EQUAL(hashmap_len(dst), 4, "wrong length after merge");
    ASSERT_STR_EQUAL((const char *)hashmap_get(dst, "c"), "3", "missing merged value");

    hashmap_destroy(dst);
    hashmap_destroy(src);
    return 1;
}

/* ============================================================================
 * Keys & Values Tests
 * ============================================================================ */

int test_hashmap_keys(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    Array *keys = hashmap_keys(map);
    ASSERT_NOT_NULL(keys, "keys failed");
    ASSERT_EQUAL(keys->len, 5, "keys wrong length");

    bool found[5] = {false};
    for (size_t i = 0; i < 5; i++) {
        int k = *(int *)((uint8_t *)keys->items + i * keys->stride);
        if (k >= 0 && k < 5) found[k] = true;
    }
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(found[i], "keys missing element");
    }

    free(keys);
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_values(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    Array *vals = hashmap_values(map);
    ASSERT_NOT_NULL(vals, "values failed");
    ASSERT_EQUAL(vals->len, 5, "values wrong length");

    bool found[5] = {false};
    for (size_t i = 0; i < 5; i++) {
        int v = *(int *)((uint8_t *)vals->items + i * vals->stride);
        int idx = v - 100;
        if (idx >= 0 && idx < 5) found[idx] = true;
    }
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(found[i], "values missing element");
    }

    free(vals);
    hashmap_destroy(map);
    return 1;
}

int test_hashmap_keys_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "hello", "1");
    hashmap_insert(map, "world", "2");

    Array *keys = hashmap_keys(map);
    ASSERT_NOT_NULL(keys, "keys strings failed");
    ASSERT_EQUAL(keys->len, 2, "keys wrong length");

    char **table = (char **)keys->items;
    bool found_hello = false, found_world = false;
    for (size_t i = 0; i < 2; i++) {
        if (strcmp(table[i], "hello") == 0) found_hello = true;
        if (strcmp(table[i], "world") == 0) found_world = true;
    }
    ASSERT_TRUE(found_hello, "keys missing hello");
    ASSERT_TRUE(found_world, "keys missing world");

    free(keys);
    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Iterator Tests
 * ============================================================================ */

int test_hashmap_iterator(void) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_fill(map, 5);

    Iterator it = hashmap_iter(map);
    bool found[5] = {false};
    int count = 0;
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const int *k = (const int *)hashmap_entry_key(map, entry);
        if (*k >= 0 && *k < 5) found[*k] = true;
        count++;
    }
    ASSERT_EQUAL(count, 5, "iterator wrong count");
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(found[i], "iterator missing key");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_iterator_str_str(void) {
    HashMap *map = hashmap_str_str();
    hashmap_insert(map, "a", "1");
    hashmap_insert(map, "b", "2");
    hashmap_insert(map, "c", "3");

    Iterator it = hashmap_iter(map);
    int count = 0;
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        count++;
    }
    ASSERT_EQUAL(count, 3, "iterator wrong count");

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Alignment Tests
 * ============================================================================ */

int test_hashmap_alignment_key_32(void) {
    HashMap *map = hashmap_create_aligned(sizeof(Aligned32), sizeof(int), 32, 1);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned32 keys[8];
    for (int i = 0; i < 8; i++) {
        memset(&keys[i], i + 1, sizeof(Aligned32));
        int v = i;
        ASSERT_EQUAL(hashmap_insert(map, &keys[i], &v), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *k = hashmap_entry_key(map, entry);
        ASSERT_TRUE(((uintptr_t)k % 32) == 0, "key not 32-byte aligned");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_alignment_value_32(void) {
    HashMap *map = hashmap_create_aligned(sizeof(int), sizeof(Aligned32), 1, 32);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned32 vals[8];
    for (int i = 0; i < 8; i++) {
        memset(&vals[i], i + 1, sizeof(Aligned32));
        ASSERT_EQUAL(hashmap_insert(map, &i, &vals[i]), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *v = hashmap_entry_value(map, entry);
        ASSERT_TRUE(((uintptr_t)v % 32) == 0, "value not 32-byte aligned");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_alignment_both_32(void) {
    HashMap *map = hashmap_create_aligned(sizeof(Aligned32), sizeof(Aligned32), 32, 32);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned32 keys[8];
    Aligned32 vals[8];
    for (int i = 0; i < 8; i++) {
        memset(&keys[i], i + 1, sizeof(Aligned32));
        memset(&vals[i], i + 1, sizeof(Aligned32));
        ASSERT_EQUAL(hashmap_insert(map, &keys[i], &vals[i]), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *k = hashmap_entry_key(map, entry);
        const void *v = hashmap_entry_value(map, entry);
        ASSERT_TRUE(((uintptr_t)k % 32) == 0, "key not 32-byte aligned");
        ASSERT_TRUE(((uintptr_t)v % 32) == 0, "value not 32-byte aligned");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_alignment_key_64(void) {
    HashMap *map = hashmap_create_aligned(sizeof(Aligned64), sizeof(int), 64, 1);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned64 keys[8];
    for (int i = 0; i < 8; i++) {
        memset(&keys[i], i + 1, sizeof(Aligned64));
        int v = i;
        ASSERT_EQUAL(hashmap_insert(map, &keys[i], &v), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *k = hashmap_entry_key(map, entry);
        ASSERT_TRUE(((uintptr_t)k % 64) == 0, "key not 64-byte aligned");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_alignment_value_64(void) {
    HashMap *map = hashmap_create_aligned(sizeof(int), sizeof(Aligned64), 1, 64);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned64 vals[8];
    for (int i = 0; i < 8; i++) {
        memset(&vals[i], i + 1, sizeof(Aligned64));
        ASSERT_EQUAL(hashmap_insert(map, &i, &vals[i]), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *v = hashmap_entry_value(map, entry);
        ASSERT_TRUE(((uintptr_t)v % 64) == 0, "value not 64-byte aligned");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_alignment_both_64(void) {
    HashMap *map = hashmap_create_aligned(sizeof(Aligned64), sizeof(Aligned64), 64, 64);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned64 keys[8];
    Aligned64 vals[8];
    for (int i = 0; i < 8; i++) {
        memset(&keys[i], i + 1, sizeof(Aligned64));
        memset(&vals[i], i + 1, sizeof(Aligned64));
        ASSERT_EQUAL(hashmap_insert(map, &keys[i], &vals[i]), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *k = hashmap_entry_key(map, entry);
        const void *v = hashmap_entry_value(map, entry);
        ASSERT_TRUE(((uintptr_t)k % 64) == 0, "key not 64-byte aligned");
        ASSERT_TRUE(((uintptr_t)v % 64) == 0, "value not 64-byte aligned");
    }

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_alignment_after_rehash(void) {
    HashMap *map = hashmap_create_aligned(sizeof(Aligned32), sizeof(int), 32, 1);
    ASSERT_NOT_NULL(map, "create_aligned failed");

    Aligned32 keys[64];
    for (int i = 0; i < 64; i++) {
        memset(&keys[i], i + 1, sizeof(Aligned32));
        int v = i;
        ASSERT_EQUAL(hashmap_insert(map, &keys[i], &v), LC_OK, "insert failed");
    }

    Iterator it = hashmap_iter(map);
    void const *entry;
    while ((entry = iter_next(&it)) != NULL) {
        const void *k = hashmap_entry_key(map, entry);
        ASSERT_TRUE(((uintptr_t)k % 32) == 0, "key misaligned after rehash");
    }

    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests
 * ============================================================================ */

int test_hashmap_large_insert(void) {
    const size_t N = 1000000;
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    ASSERT_NOT_NULL(map, "create failed");
    
    /* Reserve capacity to avoid repeated rehashing */
    ASSERT_EQUAL(hashmap_reserve(map, N), LC_OK, "reserve failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)(i * 2);
        ASSERT_EQUAL(hashmap_insert(map, &i, &val), LC_OK, "insert failed at %zu", i);
    }
    double insert_time = timer_elapsed(&t);
    printf("    Insert %zu entries: %.3f ms\n", N, insert_time * 1000);

    ASSERT_EQUAL(hashmap_len(map), N, "wrong length after inserts");

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int *val = (int *)hashmap_get(map, &i);
        ASSERT_NOT_NULL(val, "missing element at %zu", i);
        ASSERT_EQUAL(*val, (int)(i * 2), "wrong value at %zu", i);
    }
    double get_time = timer_elapsed(&t);
    printf("    Get %zu entries: %.3f ms\n", N, get_time * 1000);

    hashmap_destroy(map);
    return 1;
}

int test_hashmap_large_clone(void) {
    const size_t N = 1000000;
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    
    /* Reserve capacity before inserts */
    ASSERT_EQUAL(hashmap_reserve(map, N), LC_OK, "reserve failed");
    
    for (size_t i = 0; i < N; i++) {
        int val = (int)(i * 2);
        hashmap_insert(map, &i, &val);
    }

    Timer t = timer_start();
    HashMap *clone = hashmap_clone(map);
    double clone_time = timer_elapsed(&t);
    printf("    Clone %zu entries: %.3f ms\n", N, clone_time * 1000);

    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(hashmap_len(clone), N, "clone wrong length");

    for (size_t i = 0; i < N; i++) {
        int *val = (int *)hashmap_get(clone, &i);
        ASSERT_NOT_NULL(val, "clone missing element at %zu", i);
        ASSERT_EQUAL(*val, (int)(i * 2), "clone wrong value at %zu", i);
    }

    hashmap_destroy(map);
    hashmap_destroy(clone);
    return 1;
}

int test_hashmap_large_strings(void) {
    const size_t N = 10000;
    HashMap *map = hashmap_str_str();
    
    /* Reserve capacity to avoid rehashing */
    ASSERT_EQUAL(hashmap_reserve(map, N), LC_OK, "reserve failed");
    
    /* Pre-allocate key and value arrays to avoid snprintf overhead */
    char **keys = (char **)malloc(N * sizeof(char *));
    char **vals = (char **)malloc(N * sizeof(char *));
    ASSERT_NOT_NULL(keys, "keys allocation failed");
    ASSERT_NOT_NULL(vals, "vals allocation failed");
    
    /* Generate keys and values once */
    for (size_t i = 0; i < N; i++) {
        keys[i] = (char *)malloc(32);
        vals[i] = (char *)malloc(32);
        snprintf(keys[i], 32, "key_%zu", i);
        snprintf(vals[i], 32, "val_%zu", i);
    }

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL(hashmap_insert(map, keys[i], vals[i]), LC_OK, "insert failed at %zu", i);
    }
    double insert_time = timer_elapsed(&t);
    printf("    Insert %zu string entries: %.3f ms\n", N, insert_time * 1000);

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        const char *val = (const char *)hashmap_get(map, keys[i]);
        ASSERT_NOT_NULL(val, "missing key at %zu", i);
        ASSERT_STR_EQUAL(val, vals[i], "wrong value at %zu", i);
    }
    double get_time = timer_elapsed(&t);
    printf("    Get %zu string entries: %.3f ms\n", N, get_time * 1000);

    /* Cleanup */
    for (size_t i = 0; i < N; i++) {
        free(keys[i]);
        free(vals[i]);
    }
    free(keys);
    free(vals);
    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  HashMap Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Creation & Destruction */
    TEST(test_hashmap_create);
    TEST(test_hashmap_create_with_capacity);
    TEST(test_hashmap_str_str);
    TEST(test_hashmap_str_any);
    TEST(test_hashmap_any_str);
    TEST(test_hashmap_destroy_null);

    /* Insert */
    TEST(test_hashmap_insert);
    TEST(test_hashmap_insert_update);
    TEST(test_hashmap_insert_str_str);
    TEST(test_hashmap_insert_str_any);
    TEST(test_hashmap_insert_any_str);

    /* Get */
    TEST(test_hashmap_get);
    TEST(test_hashmap_get_str_str);
    TEST(test_hashmap_get_or_default);
    TEST(test_hashmap_get_or_default_strings);

    /* Remove */
    TEST(test_hashmap_remove);
    TEST(test_hashmap_remove_nonexistent);
    TEST(test_hashmap_remove_str_str);

    /* Contains */
    TEST(test_hashmap_contains);
    TEST(test_hashmap_contains_str_str);

    /* Clear */
    TEST(test_hashmap_clear);
    TEST(test_hashmap_clear_str_str);

    /* Reserve & Capacity */
    TEST(test_hashmap_reserve);
    TEST(test_hashmap_load_factor_expand);

    /* Equals */
    TEST(test_hashmap_equals_identical);
    TEST(test_hashmap_equals_different_order);
    TEST(test_hashmap_equals_different_values);
    TEST(test_hashmap_equals_str_str);

    /* Hash */
    TEST(test_hashmap_hash);
    TEST(test_hashmap_hash_order_independent);
    TEST(test_hashmap_hash_cached);

    /* Clone */
    TEST(test_hashmap_clone);
    TEST(test_hashmap_clone_str_str);

    /* Merge */
    TEST(test_hashmap_merge);
    TEST(test_hashmap_merge_overlapping);
    TEST(test_hashmap_merge_str_str);

    /* Keys & Values */
    TEST(test_hashmap_keys);
    TEST(test_hashmap_values);
    TEST(test_hashmap_keys_str_str);

    /* Iterator */
    TEST(test_hashmap_iterator);
    TEST(test_hashmap_iterator_str_str);

    /* Alignment */
    TEST(test_hashmap_alignment_key_32);
    TEST(test_hashmap_alignment_value_32);
    TEST(test_hashmap_alignment_both_32);
    TEST(test_hashmap_alignment_key_64);
    TEST(test_hashmap_alignment_value_64);
    TEST(test_hashmap_alignment_both_64);
    TEST(test_hashmap_alignment_after_rehash);

    /* Large Dataset Tests */
    printf("\n--- Large Dataset Tests ---\n");
    TEST(test_hashmap_large_insert);
    TEST(test_hashmap_large_clone);
    TEST(test_hashmap_large_strings);

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