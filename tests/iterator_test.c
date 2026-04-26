/** @file iterator_test.c
 * @brief Iterator Unit Tests
 * 
 * Comprehensive test suite for the iterator module, covering all iterator types
 * (filter, map, skip, take, flatten, zip, peek) and terminal operations (count,
 * any, all, for_each, fold, find, collect). Tests include basic functionality,
 * edge cases (empty containers, no matches), chaining behavior, and type
 * handling. Uses a simple Vector of integers as the primary test container.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "assertion.h"
#include "timer.h"

#include <contain/vector.h>
#include <contain/hashmap.h>
#include <contain/iterator.h>

/* ============================================================================
 * Test Helpers
 * ============================================================================ */

static bool is_even(const Container *c, const void *e) {
    (void)c;
    return (*(const int *)e) % 2 == 0;
}

static bool is_positive(const Container *c, const void *e) {
    (void)c;
    return *(const int *)e >= 0;
}

static bool is_negative(const Container *c, const void *e) {
    (void)c;
    return *(const int *)e < 0;
}

static void *double_int(const Container *c, const void *e, void *buf) {
    (void)c;
    *(int *)buf = *(const int *)e * 2;
    return buf;
}

static void *add_ten(const Container *c, const void *e, void *buf) {
    (void)c;
    *(int *)buf = *(const int *)e + 10;
    return buf;
}

static void *sum_fold(const Container *c, const void *e, void *acc) {
    (void)c;
    *(int *)acc += *(const int *)e;
    return acc;
}

static volatile int sink = 0;
static void sum_sink(const Container *c, const void *e) {
    (void)c;
    sink += *(const int *)e;
}

/* Create test vector with ints [0, n) */
static Vector *create_test_vector(size_t n) {
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < n; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }
    return vec;
}

/* Create test vector with negative and positive ints */
static Vector *create_mixed_vector(size_t n) {
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < n; i++) {
        int val = (int)i - (int)(n / 2);
        vector_push(vec, &val);
    }
    return vec;
}

/* Pair struct for HashMap collection */
typedef struct {
    const void *key;
    const void *value;
    char key_buf[32];
    char val_buf[32];
} StrPair;

/* Map function: convert "key:value" string to StrPair */
static void *make_pair(const Container *c, const void *item, void *buf) {
    (void)c;
    const char *src = (const char *)item;
    StrPair *pair = (StrPair *)buf;
    
    char *colon = strchr(src, ':');
    if (!colon) return NULL;
    
    size_t key_len = colon - src;
    memcpy(pair->key_buf, src, key_len);
    pair->key_buf[key_len] = '\0';
    pair->key = pair->key_buf;
    
    strcpy(pair->val_buf, colon + 1);
    pair->value = pair->val_buf;
    
    return pair;
}

/* ============================================================================
 * Filter Iterator Tests
 * ============================================================================ */

int test_iter_filter_basic(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    ASSERT_NOT_NULL(it, "filter iterator creation failed");

    int expected[] = {0, 2, 4, 6, 8};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong filtered value");
        count++;
    }
    ASSERT_EQUAL(count, 5, "wrong filtered count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_filter_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    ASSERT_NOT_NULL(it, "filter iterator creation failed");
    ASSERT_NULL(iter_next(it), "next on empty should be NULL");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_filter_all_match(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_positive);
    ASSERT_NOT_NULL(it, "filter iterator creation failed");

    size_t count = 0;
    while (iter_next(it)) count++;
    ASSERT_EQUAL(count, 5, "positive elements should be 5");
    
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_filter_no_match(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_negative);
    ASSERT_NOT_NULL(it, "filter iterator creation failed");
    ASSERT_NULL(iter_next(it), "next should return NULL when no match");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_filter_null_predicate(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_filter(HeapIter((Container *)vec), NULL);
    ASSERT_NOT_NULL(it, "filter iterator with NULL predicate should pass through");
    size_t count = 0;
    while (iter_next(it)) count++;
    ASSERT_EQUAL(count, 5, "all elements should pass");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}



/* ============================================================================
 * Map Iterator Tests
 * ============================================================================ */

int test_iter_map_basic(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_map(HeapIter((Container *)vec), double_int, sizeof(int));
    ASSERT_NOT_NULL(it, "map iterator creation failed");

    int expected[] = {0, 2, 4, 6, 8};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong mapped value");
        count++;
    }
    ASSERT_EQUAL(count, 5, "wrong mapped count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_map_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = iter_map(HeapIter((Container *)vec), double_int, sizeof(int));
    ASSERT_NOT_NULL(it, "map iterator creation failed");
    ASSERT_NULL(iter_next(it), "next on empty should be NULL");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_map_stride_zero(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_map(HeapIter((Container *)vec), double_int, 0);
    ASSERT_NULL(it, "map with stride 0 should fail");
    vector_destroy(vec);
    return 1;
}

int test_iter_map_null_mapper(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_map(HeapIter((Container *)vec), NULL, sizeof(int));
    ASSERT_NOT_NULL(it, "map with NULL mapper should pass through");
    size_t count = 0;
    while (iter_next(it)) count++;
    ASSERT_EQUAL(count, 5, "all elements should pass through unchanged");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Filter + Map Chained Tests
 * ============================================================================ */

int test_iter_filter_map_chain(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_map(
        iter_filter(HeapIter((Container *)vec), is_even),
        double_int, sizeof(int));
    ASSERT_NOT_NULL(it, "chained filter+map creation failed");

    int expected[] = {0, 4, 8, 12, 16};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong chained value");
        count++;
    }
    ASSERT_EQUAL(count, 5, "wrong chained count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_map_filter_chain(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_filter(
        iter_map(HeapIter((Container *)vec), double_int, sizeof(int)),
        is_even);
    ASSERT_NOT_NULL(it, "chained map+filter creation failed");

    int expected[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong chained value");
        count++;
    }
    ASSERT_EQUAL(count, 10, "wrong chained count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Skip Iterator Tests
 * ============================================================================ */

int test_iter_skip_basic(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_skip(HeapIter((Container *)vec), 3);
    ASSERT_NOT_NULL(it, "skip iterator creation failed");

    int expected[] = {3, 4, 5, 6, 7, 8, 9};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong skipped value");
        count++;
    }
    ASSERT_EQUAL(count, 7, "wrong skipped count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_skip_all(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_skip(HeapIter((Container *)vec), 10);
    ASSERT_NOT_NULL(it, "skip iterator creation failed");
    ASSERT_NULL(iter_next(it), "skip beyond length should return NULL");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_skip_zero(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_skip(HeapIter((Container *)vec), 0);
    ASSERT_NOT_NULL(it, "skip zero should succeed");

    size_t count = 0;
    while (iter_next(it)) count++;
    ASSERT_EQUAL(count, 5, "skip zero should yield all elements");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Take Iterator Tests
 * ============================================================================ */

int test_iter_take_basic(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_take(HeapIter((Container *)vec), 3);
    ASSERT_NOT_NULL(it, "take iterator creation failed");

    int expected[] = {0, 1, 2};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong taken value");
        count++;
    }
    ASSERT_EQUAL(count, 3, "wrong taken count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_take_zero(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_take(HeapIter((Container *)vec), 0);
    ASSERT_NOT_NULL(it, "take zero should succeed");
    ASSERT_NULL(iter_next(it), "take zero should return NULL immediately");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_take_more_than_length(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = iter_take(HeapIter((Container *)vec), 10);
    ASSERT_NOT_NULL(it, "take more than length should succeed");

    size_t count = 0;
    while (iter_next(it)) count++;
    ASSERT_EQUAL(count, 5, "take more than length should yield all elements");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Skip + Take Chain Tests
 * ============================================================================ */

int test_iter_skip_take_chain(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_take(
        iter_skip(HeapIter((Container *)vec), 2),
        4);
    ASSERT_NOT_NULL(it, "skip+take chain creation failed");

    int expected[] = {2, 3, 4, 5};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong chain value");
        count++;
    }
    ASSERT_EQUAL(count, 4, "wrong chain count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Flatten Iterator Tests
 * ============================================================================ */

int test_iter_flatten_basic(void) {
    Vector *outer = vector_create(sizeof(Vector *));
    Vector *inner1 = create_test_vector(3);
    Vector *inner2 = create_test_vector(2);
    Vector *inner3 = create_test_vector(4);

    vector_push(outer, &inner1);
    vector_push(outer, &inner2);
    vector_push(outer, &inner3);

    Iterator *it = iter_flatten(HeapIter((Container *)outer));
    ASSERT_NOT_NULL(it, "flatten iterator creation failed");

    int expected[] = {0, 1, 2, 0, 1, 0, 1, 2, 3};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong flattened value");
        count++;
    }
    ASSERT_EQUAL(count, 9, "wrong flattened count");

    iter_destroy(it);
    vector_destroy(inner1);
    vector_destroy(inner2);
    vector_destroy(inner3);
    vector_destroy(outer);
    return 1;
}

/* ============================================================================
 * Zip Iterator Tests
 * ============================================================================ */

typedef struct {
    int a;
    int b;
} Pair;

static void *make_int_pair(const Container *ca, const void *a,
                           const Container *cb, const void *b, void *buf) {
    (void)ca;
    (void)cb;
    Pair *p = (Pair *)buf;
    p->a = *(const int *)a;
    p->b = *(const int *)b;
    return buf;
}

int test_iter_zip_basic(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = create_test_vector(5);

    Iterator *it = iter_zip(HeapIter((Container *)vec1),
                            HeapIter((Container *)vec2),
                            make_int_pair, sizeof(Pair));
    ASSERT_NOT_NULL(it, "zip iterator creation failed");

    size_t count = 0;
    Pair *p;
    while ((p = (Pair *)iter_next(it))) {
        ASSERT_EQUAL(p->a, (int)count, "wrong left value");
        ASSERT_EQUAL(p->b, (int)count, "wrong right value");
        count++;
    }
    ASSERT_EQUAL(count, 5, "wrong zip count");

    iter_destroy(it);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

/* ============================================================================
 * Peek Iterator Tests
 * ============================================================================ */

int test_iter_peek_basic(void) {
    Vector *vec = create_test_vector(5); // [0, 1, 2, 3, 4]
    Iterator *it = iter_peekable(HeapIter((Container *)vec));
    ASSERT_NOT_NULL(it, "iter_peekable creation failed");

    const int *p1 = (const int *)iter_peek(it);
    ASSERT_NOT_NULL(p1, "First peek failed");
    ASSERT_EQUAL(*p1, 0, "Peeked wrong value");

    const int *p2 = (const int *)iter_peek(it);
    ASSERT_EQUAL(*p2, 0, "Second peek failed");
    ASSERT_EQUAL((uintptr_t)p1, (uintptr_t)p2, "Pointers must match");

    const int *n1 = (const int *)iter_next(it);
    ASSERT_EQUAL(*n1, 0, "Next returned wrong value");

    const int *p3 = (const int *)iter_peek(it);
    ASSERT_EQUAL(*p3, 1, "Peek after next failed");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_peek_and_filter(void) {
    Vector *vec = create_test_vector(10);
    // [0, 2, 4, 6, 8]
    Iterator *it = iter_peekable(iter_filter(HeapIter((Container *)vec), is_even));

    const int *p1 = (const int *)iter_peek(it);
    ASSERT_EQUAL(*p1, 0, "Filter-peek failed");

    iter_next(it);
    iter_next(it);

    const int *p2 = (const int *)iter_peek(it);
    ASSERT_EQUAL(*p2, 4, "Peek after consumption failed");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_peek_at_end(void) {
    Vector *vec = create_test_vector(1);
    Iterator *it = iter_peekable(HeapIter((Container *)vec));

    iter_next(it);
    ASSERT_NULL(iter_peek(it), "Peek at end should be NULL");
    ASSERT_NULL(iter_next(it), "Next at end should be NULL");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_peek_non_peekable(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = HeapIter((Container *)vec);

    ASSERT_NULL(iter_peek(it), "Non-peekable should return NULL");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_peek_complex_chain(void) {
    Vector *vec = create_test_vector(10);
    
    Iterator *filt = iter_filter(HeapIter((Container *)vec), is_even);
    Iterator *peekable = iter_peekable(filt);
    Iterator *final = iter_map(peekable, add_ten, sizeof(int));

    const int *p = (const int *)iter_peek(peekable);
    ASSERT_EQUAL(*p, 0, "Middle peek failed");

    const int *n = (const int *)iter_next(final);
    ASSERT_EQUAL(*n, 10, "Mapped value failed");

    iter_destroy(final);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: count
 * ============================================================================ */

int test_iter_count_basic(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    size_t count = iter_count(it);
    ASSERT_EQUAL(count, 5, "iter_count wrong");
    vector_destroy(vec);
    return 1;
}

int test_iter_count_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = HeapIter((Container *)vec);
    size_t count = iter_count(it);
    ASSERT_EQUAL(count, 0, "iter_count on empty should be 0");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: any
 * ============================================================================ */

int test_iter_any_true(void) {
    Vector *vec = create_mixed_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    bool result = iter_any(it, is_positive);
    ASSERT_TRUE(result, "iter_any should return true");
    vector_destroy(vec);
    return 1;
}

int test_iter_any_false(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    bool result = iter_any(it, is_negative);
    ASSERT_FALSE(result, "iter_any should return false");
    vector_destroy(vec);
    return 1;
}

int test_iter_any_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = HeapIter((Container *)vec);
    bool result = iter_any(it, is_positive);
    ASSERT_FALSE(result, "iter_any on empty should be false");
    vector_destroy(vec);
    return 1;
}

int test_iter_any_null_predicate(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = HeapIter((Container *)vec);
    bool result = iter_any(it, NULL);
    ASSERT_TRUE(result, "iter_any with NULL predicate should return true if any element exists");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: all
 * ============================================================================ */

int test_iter_all_true(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = iter_skip(HeapIter((Container *)vec), 1);
    bool result = iter_all(it, is_positive);
    ASSERT_TRUE(result, "iter_all should return true for positive numbers");
    vector_destroy(vec);
    return 1;
}

int test_iter_all_false(void) {
    Vector *vec = create_mixed_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    bool result = iter_all(it, is_positive);
    ASSERT_FALSE(result, "iter_all should return false");
    vector_destroy(vec);
    return 1;
}

int test_iter_all_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = HeapIter((Container *)vec);
    bool result = iter_all(it, is_positive);
    ASSERT_TRUE(result, "iter_all on empty should be vacuously true");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: for_each
 * ============================================================================ */

int test_iter_for_each_basic(void) {
    Vector *vec = create_test_vector(10);
    sink = 0;
    Iterator *it = HeapIter((Container *)vec);
    iter_for_each(it, sum_sink);
    ASSERT_EQUAL(sink, 45, "iter_for_each sum wrong");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: fold
 * ============================================================================ */

int test_iter_fold_basic(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    int sum = 0;
    iter_fold(it, &sum, sum_fold);
    ASSERT_EQUAL(sum, 45, "iter_fold sum wrong");
    vector_destroy(vec);
    return 1;
}

int test_iter_fold_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = HeapIter((Container *)vec);
    int sum = 0;
    iter_fold(it, &sum, sum_fold);
    ASSERT_EQUAL(sum, 0, "iter_fold on empty should return initial acc");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: find
 * ============================================================================ */

int test_iter_find_basic(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    int *found = (int *)iter_find(it, is_even);
    ASSERT_NOT_NULL(found, "iter_find should find element");
    ASSERT_EQUAL(*found, 0, "iter_find should return first even (0)");
    vector_destroy(vec);
    return 1;
}

int test_iter_find_not_found(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    int *found = (int *)iter_find(it, is_negative);
    ASSERT_NULL(found, "iter_find should return NULL when not found");
    vector_destroy(vec);
    return 1;
}

int test_iter_find_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Iterator *it = HeapIter((Container *)vec);
    int *found = (int *)iter_find(it, is_even);
    ASSERT_NULL(found, "iter_find on empty should return NULL");
    vector_destroy(vec);
    return 1;
}

int test_iter_find_null_predicate(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = HeapIter((Container *)vec);
    int *found = (int *)iter_find(it, NULL);
    ASSERT_NOT_NULL(found, "iter_find with NULL predicate should return first element");
    ASSERT_EQUAL(*found, 0, "first element should be 0");
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: collect
 * ============================================================================ */

int test_iter_collect_basic(void) {
    Vector *src = create_test_vector(10);
    Iterator *it = iter_filter(HeapIter((Container *)src), is_even);
    Container *dst = iter_collect(it);
    ASSERT_NOT_NULL(dst, "iter_collect failed");
    ASSERT_EQUAL(container_len(dst), 5, "collected wrong count");

    for (size_t i = 0; i < 5; i++) {
        int *val = (int *)vector_at((Vector *)dst, i);
        ASSERT_EQUAL(*val, (int)(i * 2), "collected wrong value");
    }

    container_destroy(dst);
    vector_destroy(src);
    return 1;
}

int test_iter_collect_empty(void) {
    Vector *src = vector_create(sizeof(int));
    Iterator *it = HeapIter((Container *)src);
    Container *dst = iter_collect(it);
    ASSERT_NULL(dst, "iter_collect on empty should return NULL");
    vector_destroy(src);
    return 1;
}

int test_iter_collect_strings(void) {
    Vector *src = vector_str();
    vector_push(src, "hello");
    vector_push(src, "world");
    vector_push(src, "test");

    Iterator *it = HeapIter((Container *)src);
    Container *dst = iter_collect(it);
    ASSERT_NOT_NULL(dst, "iter_collect strings failed");
    ASSERT_EQUAL(container_len(dst), 3, "collected wrong count");

    for (size_t i = 0; i < 3; i++) {
        char *s = (char *)vector_at((Vector *)dst, i);
        if (i == 0) ASSERT_STR_EQUAL(s, "hello", "wrong string");
        if (i == 1) ASSERT_STR_EQUAL(s, "world", "wrong string");
        if (i == 2) ASSERT_STR_EQUAL(s, "test", "wrong string");
    }

    container_destroy(dst);
    vector_destroy(src);
    return 1;
}

/* ============================================================================
 * Terminal: collect_in (Vector)
 * ============================================================================ */

int test_iter_collect_in_basic(void) {
    Vector *src = create_test_vector(10);
    Vector *dst = vector_create(sizeof(int));

    Iterator *it = iter_filter(HeapIter((Container *)src), is_even);
    size_t n = iter_collect_in(it, (Container *)dst);
    ASSERT_EQUAL(n, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 5, "destination wrong length");

    for (size_t i = 0; i < 5; i++) {
        int *val = (int *)vector_at(dst, i);
        ASSERT_EQUAL(*val, (int)(i * 2), "collected wrong value");
    }

    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

int test_iter_collect_in_existing(void) {
    Vector *src = create_test_vector(10);
    Vector *dst = vector_create(sizeof(int));
    int existing = 999;
    vector_push(dst, &existing);

    Iterator *it = iter_filter(HeapIter((Container *)src), is_even);
    size_t n = iter_collect_in(it, (Container *)dst);
    ASSERT_EQUAL(n, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 6, "destination should have existing + new");

    ASSERT_EQUAL(*(int *)vector_at(dst, 0), 999, "existing element should remain");
    for (size_t i = 0; i < 5; i++) {
        int *val = (int *)vector_at(dst, i + 1);
        ASSERT_EQUAL(*val, (int)(i * 2), "collected wrong value");
    }

    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

/* ============================================================================
 * Terminal: collect_in (HashMap)
 * ============================================================================ */

int test_iter_collect_into_hashmap(void) {
    /* Create vector of "key:value" strings */
    Vector *src = vector_str();
    vector_push(src, "name:Alice");
    vector_push(src, "city:Paris");
    vector_push(src, "country:France");

    /* Map strings to pairs */
    Iterator *it = iter_map(HeapIter((Container *)src), make_pair, sizeof(StrPair));

    /* Collect into HashMap */
    HashMap *map = hashmap_str_str();
    size_t n = iter_collect_in(it, (Container *)map);
    ASSERT_EQUAL(n, 3, "collected wrong count into hashmap");
    ASSERT_EQUAL(hashmap_len(map), 3, "hashmap wrong size");

    /* Verify entries */
    const char *val = (const char *)hashmap_get(map, "name");
    ASSERT_STR_EQUAL(val, "Alice", "wrong value for key 'name'");
    
    val = (const char *)hashmap_get(map, "city");
    ASSERT_STR_EQUAL(val, "Paris", "wrong value for key 'city'");
    
    val = (const char *)hashmap_get(map, "country");
    ASSERT_STR_EQUAL(val, "France", "wrong value for key 'country'");

    vector_destroy(src);
    hashmap_destroy(map);
    return 1;
}

int test_iter_collect_into_hashmap_empty(void) {
    Vector *src = vector_str();
    Iterator *it = iter_map(HeapIter((Container *)src), make_pair, sizeof(StrPair));

    HashMap *map = hashmap_str_str();
    size_t n = iter_collect_in(it, (Container *)map);
    ASSERT_EQUAL(n, 0, "collect into hashmap from empty should be 0");
    ASSERT_EQUAL(hashmap_len(map), 0, "hashmap should be empty");

    vector_destroy(src);
    hashmap_destroy(map);
    return 1;
}

int test_iter_collect_into_hashmap_with_duplicates(void) {
    Vector *src = vector_str();
    vector_push(src, "color:red");
    vector_push(src, "color:blue");
    vector_push(src, "color:green");

    Iterator *it = iter_map(HeapIter((Container *)src), make_pair, sizeof(StrPair));

    HashMap *map = hashmap_str_str();
    size_t n = iter_collect_in(it, (Container *)map);
    ASSERT_EQUAL(n, 3, "all three should insert");
    ASSERT_EQUAL(hashmap_len(map), 1, "only one key 'color' exists");
    
    const char *val = (const char *)hashmap_get(map, "color");
    ASSERT_STR_EQUAL(val, "green", "last value should overwrite");

    vector_destroy(src);
    hashmap_destroy(map);
    return 1;
}

/* ============================================================================
 * Complex Pipeline Tests
 * ============================================================================ */

int test_iter_complex_pipeline(void) {
    Vector *vec = create_test_vector(20);
    Iterator *it = iter_take(
        iter_map(
            iter_filter(
                iter_skip(HeapIter((Container *)vec), 5),
                is_even),
            double_int, sizeof(int)),
        5);
    ASSERT_NOT_NULL(it, "complex pipeline creation failed");

    int expected[] = {12, 16, 20, 24, 28};
    size_t count = 0;
    int *val;
    while ((val = (int *)iter_next(it))) {
        ASSERT_EQUAL(*val, expected[count], "wrong pipeline value");
        count++;
    }
    ASSERT_EQUAL(count, 5, "wrong pipeline count");

    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Iterator Drop Test
 * ============================================================================ */

int test_iter_drop(void) {
    Vector *vec = create_test_vector(10);
    Iterator *it = HeapIter((Container *)vec);
    iter_drop(it, 3);
    int *val = (int *)iter_next(it);
    ASSERT_NOT_NULL(val, "next after drop should not be NULL");
    ASSERT_EQUAL(*val, 3, "wrong value after drop");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

int test_iter_drop_all(void) {
    Vector *vec = create_test_vector(5);
    Iterator *it = HeapIter((Container *)vec);
    iter_drop(it, 10);
    ASSERT_NULL(iter_next(it), "next after drop beyond length should be NULL");
    iter_destroy(it);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests
 * ============================================================================ */

int test_iter_large_filter(void) {
    const size_t N = 1000000;
    Vector *vec = create_test_vector(N);

    Timer t = timer_start();
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    size_t count = iter_count(it);
    double time = timer_elapsed(&t);
    printf("    Filter %zu elements: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, N / 2, "wrong filtered count");

    vector_destroy(vec);
    return 1;
}

int test_iter_large_filter_map_take(void) {
    const size_t N = 1000000;
    Vector *vec = create_test_vector(N);

    Timer t = timer_start();
    Iterator *it = iter_take(
        iter_map(
            iter_filter(HeapIter((Container *)vec), is_even),
            double_int, sizeof(int)),
        1000);
    size_t count = iter_count(it);
    double time = timer_elapsed(&t);
    printf("    Filter+Map+Take %zu → 1000: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, 1000, "wrong count");

    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Null & Error Handling
 * ============================================================================ */

int test_iter_null_operations(void) {
    ASSERT_NULL(iter_next(NULL), "iter_next(NULL) should be NULL");
    iter_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  Iterator Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Filter Iterator */
    TEST(test_iter_filter_basic);
    TEST(test_iter_filter_empty);
    TEST(test_iter_filter_all_match);
    TEST(test_iter_filter_no_match);
    TEST(test_iter_filter_null_predicate);

    /* Map Iterator */
    TEST(test_iter_map_basic);
    TEST(test_iter_map_empty);
    TEST(test_iter_map_stride_zero);
    TEST(test_iter_map_null_mapper);

    /* Filter + Map Chain */
    TEST(test_iter_filter_map_chain);
    TEST(test_iter_map_filter_chain);

    /* Skip Iterator */
    TEST(test_iter_skip_basic);
    TEST(test_iter_skip_all);
    TEST(test_iter_skip_zero);

    /* Take Iterator */
    TEST(test_iter_take_basic);
    TEST(test_iter_take_zero);
    TEST(test_iter_take_more_than_length);

    /* Skip + Take Chain */
    TEST(test_iter_skip_take_chain);

    /* Flatten Iterator */
    TEST(test_iter_flatten_basic);

    /* Zip Iterator */
    TEST(test_iter_zip_basic);
    
        /* Peek Iterator */
    TEST(test_iter_peek_basic);
    TEST(test_iter_peek_and_filter);
    TEST(test_iter_peek_at_end);
    TEST(test_iter_peek_non_peekable);
    TEST(test_iter_peek_complex_chain);

    /* Terminals */
    TEST(test_iter_count_basic);
    TEST(test_iter_count_empty);
    TEST(test_iter_any_true);
    TEST(test_iter_any_false);
    TEST(test_iter_any_empty);
    TEST(test_iter_any_null_predicate);
    TEST(test_iter_all_true);
    TEST(test_iter_all_false);
    TEST(test_iter_all_empty);
    TEST(test_iter_for_each_basic);
    TEST(test_iter_fold_basic);
    TEST(test_iter_fold_empty);
    TEST(test_iter_find_basic);
    TEST(test_iter_find_not_found);
    TEST(test_iter_find_empty);
    TEST(test_iter_find_null_predicate);
    TEST(test_iter_collect_basic);
    TEST(test_iter_collect_empty);
    TEST(test_iter_collect_strings);
    TEST(test_iter_collect_in_basic);
    TEST(test_iter_collect_in_existing);

    /* HashMap Collection Tests */
    TEST(test_iter_collect_into_hashmap);
    TEST(test_iter_collect_into_hashmap_empty);
    TEST(test_iter_collect_into_hashmap_with_duplicates);

    /* Complex Pipeline */
    TEST(test_iter_complex_pipeline);

    /* Drop */
    TEST(test_iter_drop);
    TEST(test_iter_drop_all);

    /* Large Dataset Tests */
    printf("\n--- Large Dataset Tests ---\n");
    TEST(test_iter_large_filter);
    TEST(test_iter_large_filter_map_take);

    /* Null Handling */
    TEST(test_iter_null_operations);

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