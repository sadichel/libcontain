/**
 * @file vector_test.c
 * @brief Vector unit tests
 *
 * Tests cover:
 * - Creation and destruction (with/without capacity, comparator, alignment)
 * - Push, pop, insert, remove operations
 * - Element access (at, front, back, get_ptr, at_or_default)
 * - Range operations (insert_range, append, remove_range)
 * - Slicing, cloning, hash, equals
 * - Find, contains, rfind
 * - Capacity management (reserve, shrink_to_fit, resize)
 * - In-place operations (reverse, sort, unique)
 * - Instance creation (new empty vector of same type)
 * - String mode (automatic strdup/free)
 * - Edge cases (empty vector, out of bounds, NULL)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdint.h>
#include "assertion.h"
#include "timer.h"

#include <contain/vector.h>

/* ============================================================================
 * Test Helpers
 * ============================================================================ */


static int cmp_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int cmp_str_ptr(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* Alignment test types */
typedef struct {
    uint8_t data[32];
} Aligned32;

typedef struct {
    uint8_t data[64];
} Aligned64;

/* ============================================================================
 * Creation & Destruction
 * ============================================================================ */

int test_vector_create(void) {
    Vector *vec = vector_create(sizeof(int));
    ASSERT_NOT_NULL(vec, "vector_create failed");
    ASSERT_EQUAL(vector_len(vec), 0, "new vector not empty");
    ASSERT_TRUE(vector_capacity(vec) >= VECTOR_MIN_CAPACITY, "wrong initial capacity");
    vector_destroy(vec);
    return 1;
}

int test_vector_create_with_capacity(void) {
    Vector *vec = vector_create_with_capacity(sizeof(int), 64);
    ASSERT_NOT_NULL(vec, "create_with_capacity failed");
    ASSERT_TRUE(vector_capacity(vec) >= 64, "capacity too small");
    vector_destroy(vec);
    return 1;
}

int test_vector_str_create(void) {
    Vector *vec = vector_str();
    ASSERT_NOT_NULL(vec, "vector_str failed");
    ASSERT_TRUE(vector_is_empty(vec), "string vector not empty");
    vector_destroy(vec);
    return 1;
}

int test_vector_destroy_null(void) {
    vector_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Alignment Tests
 * ============================================================================ */

int test_vector_alignment_32(void) {
    Vector *vec = vector_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(vec, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 8; i++) {
        item.data[0] = (uint8_t)i;
        ASSERT_EQUAL(vector_push(vec, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 8; i++) {
        const void *ptr = vector_at(vec, i);
        ASSERT_TRUE(((uintptr_t)ptr % 32) == 0, "element not 32-byte aligned");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_alignment_64(void) {
    Vector *vec = vector_create_aligned(sizeof(Aligned64), 64);
    ASSERT_NOT_NULL(vec, "create_aligned failed");

    Aligned64 item = {0};
    for (int i = 0; i < 8; i++) {
        item.data[0] = (uint8_t)i;
        ASSERT_EQUAL(vector_push(vec, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 8; i++) {
        const void *ptr = vector_at(vec, i);
        ASSERT_TRUE(((uintptr_t)ptr % 64) == 0, "element not 64-byte aligned");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_alignment_after_grow(void) {
    Vector *vec = vector_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(vec, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 200; i++) {
        item.data[0] = (uint8_t)(i & 0xFF);
        ASSERT_EQUAL(vector_push(vec, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 200; i++) {
        const void *ptr = vector_at(vec, i);
        ASSERT_TRUE(((uintptr_t)ptr % 32) == 0, "element misaligned after grow");
    }

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Push & Pop Tests
 * ============================================================================ */

int test_vector_push(void) {
    Vector *vec = vector_create(sizeof(int));
    int vals[] = {10, 20, 30, 40, 50};

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(vector_push(vec, &vals[i]), LC_OK, "push failed");
    }
    ASSERT_EQUAL(vector_len(vec), 5, "wrong length");

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), vals[i], "wrong value");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_push_strings(void) {
    Vector *vec = vector_str();
    const char *strings[] = {"hello", "world", "test"};

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(vector_push(vec, strings[i]), LC_OK, "push string failed");
    }

    for (int i = 0; i < 3; i++) {
        ASSERT_STR_EQUAL((char *)vector_at(vec, i), strings[i], "wrong string");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_pop(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    for (int i = 9; i >= 0; i--) {
        ASSERT_EQUAL(vector_pop(vec), LC_OK, "pop failed");
        ASSERT_EQUAL(vector_len(vec), (size_t)i, "wrong length after pop");
    }

    ASSERT_EQUAL(vector_pop(vec), LC_EBOUNDS, "pop on empty should fail");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Insert & Remove Tests
 * ============================================================================ */

int test_vector_insert_middle(void) {
    Vector *vec = vector_create(sizeof(int));
    int a = 10, b = 30, mid = 20;
    vector_push(vec, &a);
    vector_push(vec, &b);

    ASSERT_EQUAL(vector_insert(vec, 1, &mid), LC_OK, "insert failed");
    ASSERT_EQUAL(vector_len(vec), 3, "wrong length");
    ASSERT_EQUAL(*(int *)vector_at(vec, 1), 20, "wrong value at inserted position");

    vector_destroy(vec);
    return 1;
}

int test_vector_remove(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    ASSERT_EQUAL(vector_remove(vec, 5), LC_OK, "remove failed");
    ASSERT_EQUAL(vector_len(vec), 9, "wrong length");
    ASSERT_EQUAL(*(int *)vector_at(vec, 5), 6, "wrong value after removal");

    vector_destroy(vec);
    return 1;
}

int test_vector_remove_range(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    ASSERT_EQUAL(vector_remove_range(vec, 3, 7), LC_OK, "remove_range failed");
    ASSERT_EQUAL(vector_len(vec), 6, "wrong length");

    int expected[] = {0, 1, 2, 7, 8, 9};
    for (size_t i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), expected[i], "wrong value after range remove");
    }

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Access Tests
 * ============================================================================ */

int test_vector_at(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), (int)i, "wrong value at index");
    }

    ASSERT_NULL(vector_at(vec, 999), "out of bounds should return NULL");
    vector_destroy(vec);
    return 1;
}

int test_vector_front_back(void) {
    Vector *vec = vector_create(sizeof(int));
    int first = 10, last = 20;
    vector_push(vec, &first);
    vector_push(vec, &last);

    ASSERT_EQUAL(*(int *)vector_front(vec), 10, "wrong front");
    ASSERT_EQUAL(*(int *)vector_back(vec), 20, "wrong back");

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Capacity Tests
 * ============================================================================ */

int test_vector_reserve(void) {
    Vector *vec = vector_create(sizeof(int));
    ASSERT_EQUAL(vector_reserve(vec, 256), LC_OK, "reserve failed");
    ASSERT_TRUE(vector_capacity(vec) >= 256, "capacity not increased");
    vector_destroy(vec);
    return 1;
}

int test_vector_shrink_to_fit(void) {
    Vector *vec = vector_create_with_capacity(sizeof(int), 100);
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    size_t cap_before = vector_capacity(vec);
    ASSERT_EQUAL(vector_shrink_to_fit(vec), LC_OK, "shrink_to_fit failed");
    ASSERT_TRUE(vector_capacity(vec) < cap_before, "capacity not reduced");
    ASSERT_EQUAL(vector_len(vec), 10, "length changed");

    vector_destroy(vec);
    return 1;
}

int test_vector_resize_grow(void) {
    Vector *vec = vector_create(sizeof(int));
    int val = 42;
    vector_push(vec, &val);

    ASSERT_EQUAL(vector_resize(vec, 5), LC_OK, "resize grow failed");
    ASSERT_EQUAL(vector_len(vec), 5, "wrong length");
    ASSERT_EQUAL(*(int *)vector_at(vec, 0), 42, "original element lost");

    for (size_t i = 1; i < 5; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), 0, "new slot not zeroed");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_resize_shrink(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    ASSERT_EQUAL(vector_resize(vec, 4), LC_OK, "resize shrink failed");
    ASSERT_EQUAL(vector_len(vec), 4, "wrong length");
    for (int i = 0; i < 4; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), i, "wrong value after shrink");
    }

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Clear Tests
 * ============================================================================ */

int test_vector_clear(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 100; i++) vector_push(vec, &i);

    ASSERT_EQUAL(vector_clear(vec), LC_OK, "clear failed");
    ASSERT_TRUE(vector_is_empty(vec), "not empty after clear");
    ASSERT_EQUAL(vector_len(vec), 0, "length not zero");

    int val = 42;
    vector_push(vec, &val);
    ASSERT_EQUAL(vector_len(vec), 1, "cannot push after clear");

    vector_destroy(vec);
    return 1;
}

int test_vector_clear_strings(void) {
    Vector *vec = vector_str();
    vector_push(vec, "hello");
    vector_push(vec, "world");

    ASSERT_EQUAL(vector_clear(vec), LC_OK, "clear strings failed");
    ASSERT_TRUE(vector_is_empty(vec), "not empty after clear");

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Reverse Tests
 * ============================================================================ */

int test_vector_reverse_inplace(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    ASSERT_EQUAL(vector_reverse_inplace(vec), LC_OK, "reverse_inplace failed");
    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), (int)(9 - i), "wrong reverse order");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_reverse(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    Vector *rev = vector_reverse(vec);
    ASSERT_NOT_NULL(rev, "reverse failed");
    ASSERT_EQUAL(vector_len(rev), 10, "wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)vector_at(rev, i), (int)(9 - i), "wrong reverse order");
    }

    vector_destroy(vec);
    vector_destroy(rev);
    return 1;
}

/* ============================================================================
 * Sort & Unique Tests
 * ============================================================================ */

int test_vector_sort(void) {
    Vector *vec = vector_create(sizeof(int));
    int vals[] = {50, 10, 40, 20, 30};
    for (int i = 0; i < 5; i++) vector_push(vec, &vals[i]);

    ASSERT_EQUAL(vector_sort(vec, cmp_int), LC_OK, "sort failed");
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), (int)(10 + i * 10), "wrong sort order");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_sort_strings(void) {
    Vector *vec = vector_str();
    vector_push(vec, "zebra");
    vector_push(vec, "apple");
    vector_push(vec, "mango");
    vector_push(vec, "banana");

    ASSERT_EQUAL(vector_sort(vec, cmp_str_ptr), LC_OK, "sort strings failed");
    ASSERT_STR_EQUAL((char *)vector_at(vec, 0), "apple", "wrong first");
    ASSERT_STR_EQUAL((char *)vector_at(vec, 3), "zebra", "wrong last");

    vector_destroy(vec);
    return 1;
}

int test_vector_unique(void) {
    Vector *vec = vector_create(sizeof(int));
    int vals[] = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
    for (int i = 0; i < 10; i++) vector_push(vec, &vals[i]);

    ASSERT_EQUAL(vector_unique(vec), LC_OK, "unique failed");
    ASSERT_EQUAL(vector_len(vec), 5, "wrong length after unique");

    int expected[] = {1, 2, 3, 4, 5};
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), expected[i], "wrong unique value");
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_unique_strings(void) {
    Vector *vec = vector_str();
    vector_push(vec, "apple");
    vector_push(vec, "apple");
    vector_push(vec, "banana");
    vector_push(vec, "cherry");
    vector_push(vec, "cherry");
    

    ASSERT_EQUAL(vector_unique(vec), LC_OK, "unique strings failed");
    ASSERT_EQUAL(vector_len(vec), 3, "wrong length after unique");
    ASSERT_STR_EQUAL((char *)vector_at(vec, 0), "apple", "wrong first");
    ASSERT_STR_EQUAL((char *)vector_at(vec, 1), "banana", "wrong second");
    ASSERT_STR_EQUAL((char *)vector_at(vec, 2), "cherry", "wrong third");
    
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Find & Search Tests
 * ============================================================================ */

int test_vector_find(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    int needle = 5;
    ASSERT_EQUAL(vector_find(vec, &needle), 5, "find wrong position");

    needle = 99;
    ASSERT_EQUAL(vector_find(vec, &needle), VEC_NPOS, "find should return NPOS");

    vector_destroy(vec);
    return 1;
}

int test_vector_rfind(void) {
    Vector *vec = vector_create(sizeof(int));
    int vals[] = {1, 2, 3, 2, 1, 2, 3};
    for (int i = 0; i < 7; i++) vector_push(vec, &vals[i]);

    int needle = 2;
    ASSERT_EQUAL(vector_rfind(vec, &needle), 5, "rfind wrong position");

    vector_destroy(vec);
    return 1;
}

int test_vector_contains(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    int present = 5, absent = 99;
    ASSERT_TRUE(vector_contains(vec, &present), "contains returned false for existing");
    ASSERT_FALSE(vector_contains(vec, &absent), "contains returned true for missing");

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Clone & Slice Tests
 * ============================================================================ */

int test_vector_clone(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    Vector *clone = vector_clone(vec);
    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(vector_len(clone), 10, "clone wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)vector_at(clone, i), (int)i, "clone wrong value");
    }

    int newval = 999;
    vector_set(clone, 0, &newval);
    ASSERT_EQUAL(*(int *)vector_at(vec, 0), 0, "original changed after clone modification");

    vector_destroy(vec);
    vector_destroy(clone);
    return 1;
}

int test_vector_clone_strings(void) {
    Vector *vec = vector_str();
    vector_push(vec, "hello");
    vector_push(vec, "world");

    Vector *clone = vector_clone(vec);
    ASSERT_NOT_NULL(clone, "clone strings failed");
    ASSERT_STR_EQUAL((char *)vector_at(clone, 0), "hello", "clone wrong string");

    vector_destroy(vec);
    vector_destroy(clone);
    return 1;
}

int test_vector_slice(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    Vector *slice = vector_slice(vec, 3, 7);
    ASSERT_NOT_NULL(slice, "slice failed");
    ASSERT_EQUAL(vector_len(slice), 4, "slice wrong length");

    int expected[] = {3, 4, 5, 6};
    for (size_t i = 0; i < 4; i++) {
        ASSERT_EQUAL(*(int *)vector_at(slice, i), expected[i], "slice wrong value");
    }

    vector_destroy(vec);
    vector_destroy(slice);
    return 1;
}

/* ============================================================================
 * Append & Insert Range Tests
 * ============================================================================ */

int test_vector_append(void) {
    Vector *a = vector_create(sizeof(int));
    Vector *b = vector_create(sizeof(int));
    for (int i = 0; i < 3; i++) {
        vector_push(a, &i);
        vector_push(b, &(int){i + 3});
    }

    ASSERT_EQUAL(vector_append(a, b), LC_OK, "append failed");
    ASSERT_EQUAL(vector_len(a), 6, "wrong length after append");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)vector_at(a, i), i, "wrong appended value");
    }

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

int test_vector_self_append(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 3; i++) vector_push(vec, &i);

    ASSERT_EQUAL(vector_append(vec, vec), LC_OK, "self-append failed");
    ASSERT_EQUAL(vector_len(vec), 6, "wrong length after self-append");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)vector_at(vec, i), i % 3, "wrong self-appended value");
    }

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Hash Tests
 * ============================================================================ */

int test_vector_hash(void) {
    Vector *a = vector_create(sizeof(int));
    Vector *b = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        vector_push(a, &i);
        vector_push(b, &i);
    }

    size_t ha = vector_hash(a);
    size_t hb = vector_hash(b);
    ASSERT_EQUAL(ha, hb, "identical vectors different hash");

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

int test_vector_hash_order_sensitive(void) {
    Vector *a = vector_create(sizeof(int));
    Vector *b = vector_create(sizeof(int));
    int x = 1, y = 2;
    vector_push(a, &x); vector_push(a, &y);
    vector_push(b, &y); vector_push(b, &x);

    ASSERT_NOT_EQUAL(vector_hash(a), vector_hash(b), "order should affect hash");

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

/* ============================================================================
 * Equals Tests
 * ============================================================================ */

int test_vector_equals_identical(void) {
    Vector *a = vector_create(sizeof(int));
    Vector *b = vector_create(sizeof(int));
    
    for (int i = 0; i < 5; i++) {
        vector_push(a, &i);
        vector_push(b, &i);
    }

    ASSERT_TRUE(vector_equals(a, b), "identical vectors should be equal");

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

int test_vector_equals_different_values(void) {
    Vector *a = vector_create(sizeof(int));
    Vector *b = vector_create(sizeof(int));
    
    for (int i = 0; i < 5; i++) vector_push(a, &i);
    for (int i = 0; i < 4; i++) vector_push(b, &i);
    
    int extra = 99;
    vector_push(b, &extra);
    
    ASSERT_FALSE(vector_equals(a, b), "different values should not be equal");

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

int test_vector_equals_strings_same_order(void) {
    Vector *a = vector_str();
    Vector *b = vector_str();
    vector_push(a, "hello");
    vector_push(a, "world");
    vector_push(b, "hello");
    vector_push(b, "world");

    ASSERT_TRUE(vector_equals(a, b), "same order strings should be equal");

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

int test_vector_equals_strings_different_order(void) {
    Vector *a = vector_str();
    Vector *b = vector_str();
    vector_push(a, "hello");
    vector_push(a, "world");
    vector_push(b, "world");
    vector_push(b, "hello");

    ASSERT_FALSE(vector_equals(a, b), "different order should NOT be equal");

    vector_destroy(a);
    vector_destroy(b);
    return 1;
}

/* ============================================================================
 * Iterator Tests
 * ============================================================================ */

int test_vector_iterator(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    Iterator it = vector_iter(vec);
    int count = 0;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "iterator wrong value");
        count++;
    }
    ASSERT_EQUAL(count, 10, "iterator wrong count");

    vector_destroy(vec);
    return 1;
}

int test_vector_iterator_reversed(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    Iterator it = vector_iter_reversed(vec);
    int count = 9;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "reverse iterator wrong value");
        count--;
    }
    ASSERT_EQUAL(count, -1, "reverse iterator wrong count");

    vector_destroy(vec);
    return 1;
}

int test_vector_iterator_strings(void) {
    Vector *vec = vector_str();
    vector_push(vec, "hello");
    vector_push(vec, "world");

    Iterator it = vector_iter(vec);
    char *p;
    p = (char *)iter_next(&it);
    ASSERT_STR_EQUAL(p, "hello", "iterator wrong string");
    p = (char *)iter_next(&it);
    ASSERT_STR_EQUAL(p, "world", "iterator wrong string");
    ASSERT_NULL(iter_next(&it), "iterator not exhausted");

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * To Array Tests
 * ============================================================================ */

int test_vector_to_array(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < 10; i++) vector_push(vec, &i);

    Array *arr = vector_to_array(vec);
    ASSERT_NOT_NULL(arr, "to_array failed");
    ASSERT_EQUAL(arr->len, 10, "array wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)((uint8_t *)arr->items + i * arr->stride), (int)i, "array wrong value");
    }

    free(arr);
    vector_destroy(vec);
    return 1;
}

int test_vector_to_array_strings(void) {
    Vector *vec = vector_str();
    vector_push(vec, "hello");
    vector_push(vec, "world");

    Array *arr = vector_to_array(vec);
    ASSERT_NOT_NULL(arr, "to_array strings failed");
    ASSERT_EQUAL(arr->len, 2, "array wrong length");

    char **table = (char **)arr->items;
    ASSERT_STR_EQUAL(table[0], "hello", "array wrong string");
    ASSERT_STR_EQUAL(table[1], "world", "array wrong string");

    free(arr);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests (Performance & Stress)
 * ============================================================================ */

int test_vector_large_push(void) {
    const size_t N = 1000000;
    Vector *vec = vector_create(sizeof(int));
    ASSERT_NOT_NULL(vec, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_EQUAL_FMT(vector_push(vec, &val), LC_OK, "push failed at %zu", i);
    }
    double push_time = timer_elapsed(&t);

    ASSERT_EQUAL(vector_len(vec), N, "wrong length after pushes");
    printf("    Push %zu elements: %.3f ms\n", N, push_time * 1000);

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL_FMT(*(int *)vector_at(vec, i), (int)i, "wrong value at %zu", i);
    }
    double access_time = timer_elapsed(&t);
    printf("    Access %zu elements: %.3f ms\n", N, access_time * 1000);

    vector_destroy(vec);
    return 1;
}

int test_vector_large_insert_front(void) {
    const size_t N = 100000;
    Vector *vec = vector_create(sizeof(int));
    ASSERT_NOT_NULL(vec, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_EQUAL_FMT(vector_insert(vec, 0, &val), LC_OK, "insert front failed at %zu", i);
    }
    double insert_time = timer_elapsed(&t);
    printf("    Insert %zu at front: %.3f ms\n", N, insert_time * 1000);

    ASSERT_EQUAL(vector_len(vec), N, "wrong length after inserts");

    /* Verify reversed order */
    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL_FMT(*(int *)vector_at(vec, i), (int)(N - 1 - i), "wrong order at %zu", i);
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_large_remove_front(void) {
    const size_t N = 100000;
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL_FMT(vector_remove(vec, 0), LC_OK, "remove front failed at %zu", i);
        ASSERT_EQUAL(vector_len(vec), N - i - 1, "wrong length after remove");
    }
    double remove_time = timer_elapsed(&t);
    printf("    Remove %zu from front: %.3f ms\n", N, remove_time * 1000);

    ASSERT_TRUE(vector_is_empty(vec), "vector not empty after all removes");

    vector_destroy(vec);
    return 1;
}

int test_vector_large_insert_middle(void) {
    const size_t N = 100000;
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }

    Timer t = timer_start();
    for (size_t i = 0; i < N; i += 1000) {
        int val = -1;
        ASSERT_EQUAL_FMT(vector_insert(vec, i, &val), LC_OK, "insert middle failed at %zu", i);
    }
    double insert_time = timer_elapsed(&t);
    printf("    Insert %zu in middle: %.3f ms\n", N / 1000, insert_time * 1000);

    vector_destroy(vec);
    return 1;
}

int test_vector_large_remove_middle(void) {
    const size_t N = 100000;
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }

    Timer t = timer_start();
    for (size_t i = N; i > 0; i -= 1000) {
        ASSERT_EQUAL_FMT(vector_remove(vec, i / 2), LC_OK, "remove middle failed at %zu", i);
    }
    double remove_time = timer_elapsed(&t);
    printf("    Remove %zu from middle: %.3f ms\n", N / 1000, remove_time * 1000);

    vector_destroy(vec);
    return 1;
}

int test_vector_large_reserve(void) {
    const size_t N = 1000000;
    Vector *vec = vector_create(sizeof(int));
    ASSERT_NOT_NULL(vec, "create failed");

    Timer t = timer_start();
    ASSERT_EQUAL(vector_reserve(vec, N), LC_OK, "reserve failed");
    double reserve_time = timer_elapsed(&t);
    printf("    Reserve %zu capacity: %.3f ms\n", N, reserve_time * 1000);

    ASSERT_TRUE(vector_capacity(vec) >= N, "capacity not sufficient");

    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_EQUAL(vector_push(vec, &val), LC_OK, "push after reserve failed");
    }

    ASSERT_EQUAL(vector_len(vec), N, "wrong length after reserve and push");

    vector_destroy(vec);
    return 1;
}

int test_vector_large_shrink(void) {
    const size_t N = 1000000;
    Vector *vec = vector_create_with_capacity(sizeof(int), N);
    for (size_t i = 0; i < N / 10; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }

    size_t cap_before = vector_capacity(vec);
    Timer t = timer_start();
    ASSERT_EQUAL(vector_shrink_to_fit(vec), LC_OK, "shrink failed");
    double shrink_time = timer_elapsed(&t);
    printf("    Shrink from %zu to %zu: %.3f ms\n", cap_before, vector_capacity(vec), shrink_time * 1000);

    ASSERT_EQUAL(vector_len(vec), N / 10, "length changed after shrink");

    vector_destroy(vec);
    return 1;
}

int test_vector_large_sort(void) {
    const size_t N = 100000;
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)(rand() % N);
        vector_push(vec, &val);
    }

    Timer t = timer_start();
    ASSERT_EQUAL(vector_sort(vec, cmp_int), LC_OK, "sort failed");
    double sort_time = timer_elapsed(&t);
    printf("    Sort %zu elements: %.3f ms\n", N, sort_time * 1000);

    /* Verify sorted order */
    for (size_t i = 1; i < N; i++) {
        int prev = *(int *)vector_at(vec, i - 1);
        int curr = *(int *)vector_at(vec, i);
        ASSERT_TRUE_FMT(prev <= curr, "sort order violation at %zu", i);
    }

    vector_destroy(vec);
    return 1;
}

int test_vector_large_unique(void) {
    const size_t N = 100000;
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)(i / 10);  /* Creates many duplicates */
        vector_push(vec, &val);
    }

    Timer t = timer_start();
    ASSERT_EQUAL(vector_unique(vec), LC_OK, "unique failed");
    double unique_time = timer_elapsed(&t);
    printf("    Unique %zu elements (with duplicates): %.3f ms\n", N, unique_time * 1000);

    ASSERT_EQUAL(vector_len(vec), N / 10, "wrong unique length");

    vector_destroy(vec);
    return 1;
}

int test_vector_large_clone(void) {
    const size_t N = 1000000;
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }

    Timer t = timer_start();
    Vector *clone = vector_clone(vec);
    double clone_time = timer_elapsed(&t);
    printf("    Clone %zu elements: %.3f ms\n", N, clone_time * 1000);

    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(vector_len(clone), N, "clone wrong length");

    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL_FMT(*(int *)vector_at(clone, i), (int)i, "clone wrong value at %zu", i);
    }

    vector_destroy(vec);
    vector_destroy(clone);
    return 1;
}

int test_vector_large_strings(void) {
    const size_t N = 100000;
    Vector *vec = vector_str();
    char buf[32];

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_EQUAL_FMT(vector_push(vec, buf), LC_OK, "push string failed at %zu", i);
    }
    double push_time = timer_elapsed(&t);
    printf("    Push %zu strings: %.3f ms\n", N, push_time * 1000);

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        char *s = (char *)vector_at(vec, i);
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_STR_EQUAL_FMT(s, buf, "wrong string at %zu", i);
    }
    double access_time = timer_elapsed(&t);
    printf("    Access %zu strings: %.3f ms\n", N, access_time * 1000);

    vector_destroy(vec);
    return 1;
}

int test_vector_large_insert_range(void) {
    const size_t N = 50000;
    Vector *dst = vector_create(sizeof(int));
    Vector *src = vector_create(sizeof(int));

    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        vector_push(dst, &val);
        vector_push(src, &(int){i + N});
    }

    Timer t = timer_start();
    ASSERT_EQUAL(vector_insert_range(dst, N / 2, src, 0, N), LC_OK, "insert_range failed");
    double insert_time = timer_elapsed(&t);
    printf("    Insert range %zu elements into middle: %.3f ms\n", N, insert_time * 1000);

    ASSERT_EQUAL(vector_len(dst), N * 2, "wrong length after insert_range");

    vector_destroy(dst);
    vector_destroy(src);
    return 1;
}

int test_vector_large_splice(void) {
    const size_t N = 50000;
    Vector *dst = vector_create(sizeof(int));
    Vector *src = vector_create(sizeof(int));

    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        vector_push(dst, &val);
        vector_push(src, &(int){i + N});
    }

    Timer t = timer_start();
    ASSERT_EQUAL(vector_splice(dst, N / 2, N / 4, src, 0, N / 2), LC_OK, "splice failed");
    double splice_time = timer_elapsed(&t);
    printf("    Splice %zu elements: %.3f ms\n", N / 2, splice_time * 1000);

    vector_destroy(dst);
    vector_destroy(src);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  Vector Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Creation & Destruction */
    TEST(test_vector_create);
    TEST(test_vector_create_with_capacity);
    TEST(test_vector_str_create);
    TEST(test_vector_destroy_null);

    // /* Alignment */
    TEST(test_vector_alignment_32);
    TEST(test_vector_alignment_64);
    TEST(test_vector_alignment_after_grow);

    /* Push & Pop */
    TEST(test_vector_push);
    TEST(test_vector_push_strings);
    TEST(test_vector_pop);

    /* Insert & Remove */
    TEST(test_vector_insert_middle);
    TEST(test_vector_remove);
    TEST(test_vector_remove_range);

    /* Access */
    TEST(test_vector_at);
    TEST(test_vector_front_back);

    /* Capacity */
    TEST(test_vector_reserve);
    TEST(test_vector_shrink_to_fit);
    TEST(test_vector_resize_grow);
    TEST(test_vector_resize_shrink);

    /* Clear */
    TEST(test_vector_clear);
    TEST(test_vector_clear_strings);

    /* Reverse */
    TEST(test_vector_reverse_inplace);
    TEST(test_vector_reverse);

    /* Sort & Unique */
    TEST(test_vector_sort);
    TEST(test_vector_sort_strings);
    TEST(test_vector_unique);
    TEST(test_vector_unique_strings);

    /* Find & Search */
    TEST(test_vector_find);
    TEST(test_vector_rfind);
    TEST(test_vector_contains);

    /* Clone & Slice */
    TEST(test_vector_clone);
    TEST(test_vector_clone_strings);
    TEST(test_vector_slice);

    /* Append */
    TEST(test_vector_append);
    TEST(test_vector_self_append);

    /* Hash */
    TEST(test_vector_hash);
    TEST(test_vector_hash_order_sensitive);

    /* Equals */
    TEST(test_vector_equals_identical);
    TEST(test_vector_equals_different_values);
    TEST(test_vector_equals_strings_same_order);
    TEST(test_vector_equals_strings_different_order);

    /* Iterator */
    TEST(test_vector_iterator);
    TEST(test_vector_iterator_reversed);
    TEST(test_vector_iterator_strings);

    /* To Array */
    TEST(test_vector_to_array);
    TEST(test_vector_to_array_strings);

    /* Large Dataset Tests */
    TEST(test_vector_large_push);
    TEST(test_vector_large_insert_front);
    TEST(test_vector_large_remove_front);
    TEST(test_vector_large_insert_middle);
    TEST(test_vector_large_remove_middle);
    TEST(test_vector_large_reserve);
    TEST(test_vector_large_shrink);
    TEST(test_vector_large_sort);
    TEST(test_vector_large_unique);
    TEST(test_vector_large_clone);
    TEST(test_vector_large_strings);
    TEST(test_vector_large_insert_range);
    TEST(test_vector_large_splice);

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
