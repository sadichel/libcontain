/** @file chainer2_test.c
 * @brief Chainer2 Unit Tests
 * 
 * Comprehensive test suite for the Chainer2 pipeline system.
 * Tests cover creation, filter/map/skip/take/flatten/zip stages, terminals,
 * chain binding, reuse, large dataset performance, and HashMap collection.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assertion.h"
#include "timer.h"

#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
#include <contain/chainer2.h>

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

static bool is_less_than_50(const Container *c, const void *e) {
    (void)c;
    return *(const int *)e < 50;
}

static void *double_int(const Container *c, const void *e, void *buf) {
    (void)c;
    *(int *)buf = *(const int *)e * 2;
    return buf;
}

static void *int_to_double(const Container *c, const void *e, void *buf) {
    (void)c;
    *(double *)buf = (double)*(const int *)e;
    return buf;
}

static void *int_to_float(const Container *c, const void *e, void *buf) {
    (void)c;
    *(float *)buf = (float)*(const int *)e;
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

static void *sum_fold_double(const Container *c, const void *e, void *acc) {
    (void)c;
    *(double *)acc += *(const double *)e;
    return acc;
}

static void sum_sink(const Container *c, const void *e) {
    (void)c;
    static volatile int sink = 0;
    sink += *(const int *)e;
}

static void *merge_ints(const Container *ca, const void *a,
                        const Container *cb, const void *b, void *buf) {
    (void)ca; (void)cb;
    *(int *)buf = *(const int *)a + *(const int *)b;
    return buf;
}

static Vector *create_test_vector(size_t n) {
    Vector *vec = vector_create(sizeof(int));
    vector_reserve(vec, n);
    for (size_t i = 0; i < n; i++) {
        int val = (int)i;
        vector_push(vec, &val);
    }
    return vec;
}

static Vector *create_double_vector(size_t n) {
    Vector *vec = vector_create(sizeof(double));
    vector_reserve(vec, n);
    for (size_t i = 0; i < n; i++) {
        double val = (double)i;
        vector_push(vec, &val);
    }
    return vec;
}

static Vector *create_mixed_vector(size_t n) {
    Vector *vec = vector_create(sizeof(int));
    for (size_t i = 0; i < n; i++) {
        int val = (int)i - (int)(n / 2);
        vector_push(vec, &val);
    }
    return vec;
}

static Vector *create_vector_of_vectors(size_t num_vectors, size_t elements_per_vector) {
    Vector *outer = vector_create(sizeof(Vector *));
    vector_reserve(outer, num_vectors);
    
    for (size_t i = 0; i < num_vectors; i++) {
        Vector *inner = create_test_vector(elements_per_vector);
        vector_push(outer, &inner);
    }
    
    return outer;
}

/* ============================================================================
 * Creation & Destruction Tests
 * ============================================================================ */

int test_chainer_create(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_create_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Chainer c = Chain((Container *)vec);
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_destroy_null(void) {
    chain_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Filter Stage Tests
 * ============================================================================ */

int test_chainer_filter_only(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "filter only wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_filter_all_match(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_positive);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "all elements should match");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_filter_no_match(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_negative);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 0, "no elements should match");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_multiple_filters(void) {
    Vector *vec = create_test_vector(20);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_positive);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 10, "multiple filters wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Map Stage Tests
 * ============================================================================ */

int test_chainer_map_only(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_map(&c, double_int, sizeof(int));
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "map only wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_filter_map(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "filter+map wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Skip Stage Tests
 * ============================================================================ */

int test_chainer_skip_only(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 3);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 7, "skip only wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_skip_all(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 10);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 0, "skip all should return 0");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_skip_zero(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 0);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "skip zero should return all");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Take Stage Tests
 * ============================================================================ */

int test_chainer_take_only(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_take(&c, 3);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 3, "take only wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_take_more_than_length(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_take(&c, 10);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "take more than length should return all");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_take_zero(void) {
    Vector *vec = create_test_vector(5);
    Chainer c = Chain((Container *)vec);
    chain_take(&c, 0);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 0, "take zero should return 0");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Skip + Take Chain Tests
 * ============================================================================ */

int test_chainer_skip_take(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 2);
    chain_take(&c, 4);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 4, "skip+take wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Filter + Map + Take Chain Tests
 * ============================================================================ */

int test_chainer_filter_map_take(void) {
    Vector *vec = create_test_vector(20);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    chain_take(&c, 5);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "filter+map+take wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Full Pipeline Tests (Skip + Filter + Map + Take)
 * ============================================================================ */

int test_chainer_full_pipeline(void) {
    Vector *vec = create_test_vector(20);
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 5);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    chain_take(&c, 4);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 4, "full pipeline wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Flatten Stage Tests
 * ============================================================================ */

int test_chainer_flatten_basic(void) {
    Vector *outer = create_vector_of_vectors(3, 5);
    
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 15, "flatten wrong count");
    
    chain_destroy(&c);
    vector_destroy(outer);
    return 1;
}

int test_chainer_flatten_empty_outer(void) {
    Vector *outer = vector_create(sizeof(Vector *));
    
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 0, "flatten empty outer should return 0");
    
    chain_destroy(&c);
    vector_destroy(outer);
    return 1;
}

int test_chainer_flatten_empty_inner(void) {
    Vector *outer = vector_create(sizeof(Vector *));
    Vector *inner = vector_create(sizeof(int));
    vector_push(outer, &inner);
    
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 0, "flatten empty inner should return 0");
    
    chain_destroy(&c);
    vector_destroy(inner);
    vector_destroy(outer);
    return 1;
}

int test_chainer_flatten_with_filter(void) {
    Vector *outer = create_vector_of_vectors(3, 10);
    
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 15, "flatten+filter wrong count");
    
    chain_destroy(&c);
    vector_destroy(outer);
    return 1;
}

int test_chainer_flatten_nested_with_map(void) {
    Vector *outer = create_vector_of_vectors(2, 5);
    
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    chain_map(&c, double_int, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 10, "flatten+map wrong count");
    
    chain_destroy(&c);
    vector_destroy(outer);
    return 1;
}

/* ============================================================================
 * Zip Stage Tests
 * ============================================================================ */

int test_chainer_zip_basic(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = create_test_vector(5);
    
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 5, "zip wrong count");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_zip_different_lengths(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = create_test_vector(3);
    
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 3, "zip with different lengths wrong count");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_zip_with_filter(void) {
    Vector *vec1 = create_test_vector(10);
    Vector *vec2 = create_test_vector(10);
    
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 10, "zip+filter wrong count");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_zip_with_map(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = create_test_vector(5);
    
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    chain_map(&c, double_int, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 5, "zip+map wrong count");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_zip_empty_left(void) {
    Vector *vec1 = vector_create(sizeof(int));
    Vector *vec2 = create_test_vector(5);
    
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 0, "zip with empty left should return 0");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_zip_empty_right(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = vector_create(sizeof(int));
    
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 0, "zip with empty right should return 0");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

/* ============================================================================
 * Flatten + Zip Combined Tests
 * ============================================================================ */

int test_chainer_flatten_then_zip(void) {
    Vector *outer = create_vector_of_vectors(3, 4);
    Vector *vec2 = create_test_vector(12);
    
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    size_t count = chain_count(&c);
    
    ASSERT_EQUAL(count, 12, "flatten+zip wrong count");
    
    chain_destroy(&c);
    vector_destroy(outer);
    vector_destroy(vec2);
    return 1;
}

/* ============================================================================
 * Cross-Type collect_into Tests
 * ============================================================================ */

int test_chainer_collect_into_int_to_double(void) {
    Vector *src = create_test_vector(10);
    
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    chain_map(&c, int_to_double, sizeof(double));
    
    Vector *dst = vector_create(sizeof(double));
    size_t n = chain_collect_into(&c, (Container *)dst);
    
    ASSERT_EQUAL(n, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 5, "destination wrong length");
    
    double expected[] = {0.0, 2.0, 4.0, 6.0, 8.0};
    for (size_t i = 0; i < 5; i++) {
        double *val = (double *)vector_at(dst, i);
        ASSERT_EQUAL(*val, expected[i], "wrong double value at index %zu", i);
    }
    
    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

int test_chainer_collect_into_int_to_float(void) {
    Vector *src = create_test_vector(10);
    
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_positive);
    chain_map(&c, int_to_float, sizeof(float));
    
    Vector *dst = vector_create(sizeof(float));
    size_t n = chain_collect_into(&c, (Container *)dst);
    
    ASSERT_EQUAL(n, 10, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 10, "destination wrong length");
    
    for (size_t i = 0; i < 10; i++) {
        float *val = (float *)vector_at(dst, i);
        float expected = (float)i;
        ASSERT_EQUAL(*val, expected, "wrong float value at index %zu", i);
    }
    
    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

int test_chainer_collect_into_filter_map_collect(void) {
    Vector *src = create_test_vector(20);
    
    Chainer c = Chain((Container *)src);
    chain_skip(&c, 5);
    chain_filter(&c, is_even);
    chain_map(&c, int_to_double, sizeof(double));
    
    Vector *dst = vector_create(sizeof(double));
    size_t n = chain_collect_into(&c, (Container *)dst);
    
    ASSERT_EQUAL(n, 7, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 7, "destination wrong length");
    
    double expected[] = {6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0};
    for (size_t i = 0; i < 7; i++) {
        double *val = (double *)vector_at(dst, i);
        ASSERT_EQUAL(*val, expected[i], "wrong double value at index %zu", i);
    }
    
    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

int test_chainer_collect_into_existing_double_vector(void) {
    Vector *src = create_test_vector(10);
    Vector *dst = create_double_vector(3);
    
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    chain_map(&c, int_to_double, sizeof(double));
    
    size_t n = chain_collect_into(&c, (Container *)dst);
    
    ASSERT_EQUAL(n, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 8, "destination wrong length");
    
    for (size_t i = 0; i < 3; i++) {
        double *val = (double *)vector_at(dst, i);
        ASSERT_EQUAL(*val, (double)i, "existing value changed at index %zu", i);
    }
    
    double expected[] = {0.0, 2.0, 4.0, 6.0, 8.0};
    for (size_t i = 0; i < 5; i++) {
        double *val = (double *)vector_at(dst, i + 3);
        ASSERT_EQUAL(*val, expected[i], "wrong double value at index %zu", i + 3);
    }
    
    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

int test_chainer_collect_into_chain_reuse_different_types(void) {
    Vector *src = create_test_vector(10);
    
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    
    Vector *dst_int = vector_create(sizeof(int));
    size_t n1 = chain_collect_into(&c, (Container *)dst_int);
    
    ASSERT_EQUAL(n1, 5, "first collect wrong count");
    ASSERT_EQUAL(vector_len(dst_int), 5, "first destination wrong length");
    
    chain_bind(&c, (Container *)src);
    chain_map(&c, int_to_double, sizeof(double));
    
    Vector *dst_double = vector_create(sizeof(double));
    size_t n2 = chain_collect_into(&c, (Container *)dst_double);
    
    ASSERT_EQUAL(n2, 5, "second collect wrong count");
    ASSERT_EQUAL(vector_len(dst_double), 5, "second destination wrong length");
    
    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst_int);
    vector_destroy(dst_double);
    return 1;
}

/* ============================================================================
 * Terminal: chain_count
 * ============================================================================ */

int test_chainer_count_basic(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 5, "chain_count wrong");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_count_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    ASSERT_EQUAL(count, 0, "chain_count on empty should be 0");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_count_reset(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    size_t count1 = chain_count(&c);
    size_t count2 = chain_count(&c);
    ASSERT_EQUAL(count1, 5, "first count wrong");
    ASSERT_EQUAL(count2, 5, "second count should be same (reset works)");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: chain_any
 * ============================================================================ */

int test_chainer_any_true(void) {
    Vector *vec = create_mixed_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_positive);
    bool result = chain_any(&c, NULL);
    ASSERT_TRUE(result, "chain_any should return true");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_any_false(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_negative);
    bool result = chain_any(&c, NULL);
    ASSERT_FALSE(result, "chain_any should return false");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_any_with_predicate(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    bool result = chain_any(&c, is_less_than_50);
    ASSERT_TRUE(result, "chain_any with predicate should return true");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: chain_all
 * ============================================================================ */

int test_chainer_all_true(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_positive);
    bool result = chain_all(&c, is_positive);
    ASSERT_TRUE(result, "chain_all should return true");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_all_false(void) {
    Vector *vec = create_mixed_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_positive);
    bool result = chain_all(&c, is_even);
    ASSERT_FALSE(result, "chain_all should return false");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_all_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Chainer c = Chain((Container *)vec);
    bool result = chain_all(&c, is_even);
    ASSERT_TRUE(result, "chain_all on empty should be vacuously true");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: chain_for_each
 * ============================================================================ */

int test_chainer_for_each_basic(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_for_each(&c, sum_sink);
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: chain_fold
 * ============================================================================ */

int test_chainer_fold_basic(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    int sum = 0;
    chain_fold(&c, &sum, sum_fold);
    ASSERT_EQUAL(sum, 20, "chain_fold sum wrong (0+2+4+6+8=20)");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_fold_empty(void) {
    Vector *vec = vector_create(sizeof(int));
    Chainer c = Chain((Container *)vec);
    int sum = 42;
    chain_fold(&c, &sum, sum_fold);
    ASSERT_EQUAL(sum, 42, "chain_fold on empty should return initial acc");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_fold_double(void) {
    Vector *src = create_test_vector(10);
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    chain_map(&c, int_to_double, sizeof(double));
    
    double sum = 0.0;
    chain_fold(&c, &sum, sum_fold_double);
    ASSERT_EQUAL(sum, 20.0, "chain_fold double sum wrong (0+2+4+6+8=20)");
    
    chain_destroy(&c);
    vector_destroy(src);
    return 1;
}

/* ============================================================================
 * Terminal: chain_find
 * ============================================================================ */

int test_chainer_find_basic(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    int *found = (int *)chain_find(&c, NULL);
    ASSERT_NOT_NULL(found, "chain_find should find element");
    ASSERT_EQUAL(*found, 0, "first even should be 0");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_find_not_found(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_negative);
    int *found = (int *)chain_find(&c, NULL);
    ASSERT_NULL(found, "chain_find should return NULL");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_find_with_predicate(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    int *found = (int *)chain_find(&c, is_less_than_50);
    ASSERT_NOT_NULL(found, "chain_find with predicate should find element");
    ASSERT_EQUAL(*found, 0, "first even less than 50 should be 0");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: chain_first
 * ============================================================================ */

int test_chainer_first_basic(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    int *first = (int *)chain_first(&c);
    ASSERT_NOT_NULL(first, "chain_first should return element");
    ASSERT_EQUAL(*first, 0, "first even should be 0");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_first_empty(void) {
    Vector *vec = create_test_vector(10);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_negative);
    int *first = (int *)chain_first(&c);
    ASSERT_NULL(first, "chain_first on empty should return NULL");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Terminal: chain_collect
 * ============================================================================ */

int test_chainer_collect_basic(void) {
    Vector *src = create_test_vector(10);
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    Container *dst = chain_collect(&c);
    ASSERT_NOT_NULL(dst, "chain_collect failed");
    ASSERT_EQUAL(container_len(dst), 5, "collected wrong count");

    for (size_t i = 0; i < 5; i++) {
        int *val = (int *)vector_at((Vector *)dst, i);
        ASSERT_EQUAL(*val, (int)(i * 2), "collected wrong value");
    }

    container_destroy(dst);
    vector_destroy(src);
    return 1;
}

int test_chainer_collect_empty(void) {
    Vector *src = vector_create(sizeof(int));
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    Container *dst = chain_collect(&c);
    ASSERT_NULL(dst, "chain_collect on empty should return NULL");
    vector_destroy(src);
    return 1;
}

int test_chainer_collect_strings(void) {
    Vector *src = vector_str();
    vector_push(src, "hello");
    vector_push(src, "world");
    vector_push(src, "test");

    Chainer c = Chain((Container *)src);
    Container *dst = chain_collect(&c);
    ASSERT_NOT_NULL(dst, "chain_collect strings failed");
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

int test_chainer_collect_double(void) {
    Vector *src = create_test_vector(10);
    Vector *dst = vector_create(sizeof(double));  // Create destination of correct type
    
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    chain_map(&c, int_to_double, sizeof(double));
    
    size_t count = chain_collect_into(&c, (Container *)dst);
    ASSERT_EQUAL(count, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 5, "destination wrong length");
    
    for (size_t i = 0; i < 5; i++) {
        double *val = (double *)vector_at(dst, i);
        ASSERT_EQUAL(*val, (double)(i * 2), "wrong double value");
    }
    
    vector_destroy(dst);
    vector_destroy(src);
    return 1;
}

/* ============================================================================
 * Terminal: chain_collect_into
 * ============================================================================ */

int test_chainer_collect_into_basic(void) {
    Vector *src = create_test_vector(10);
    Vector *dst = vector_create(sizeof(int));

    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    size_t n = chain_collect_into(&c, (Container *)dst);
    ASSERT_EQUAL(n, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 5, "destination wrong length");

    for (size_t i = 0; i < 5; i++) {
        int *val = (int *)vector_at(dst, i);
        ASSERT_EQUAL(*val, (int)(i * 2), "collected wrong value");
    }

    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

int test_chainer_collect_into_existing(void) {
    Vector *src = create_test_vector(10);
    Vector *dst = vector_create(sizeof(int));
    int existing = 999;
    vector_push(dst, &existing);

    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    size_t n = chain_collect_into(&c, (Container *)dst);
    ASSERT_EQUAL(n, 5, "collected wrong count");
    ASSERT_EQUAL(vector_len(dst), 6, "destination should have existing + new");

    ASSERT_EQUAL(*(int *)vector_at(dst, 0), 999, "existing element overwritten");
    for (size_t i = 0; i < 5; i++) {
        int *val = (int *)vector_at(dst, i + 1);
        ASSERT_EQUAL(*val, (int)(i * 2), "collected wrong value");
    }

    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

/* ============================================================================
 * Chain Bind Tests
 * ============================================================================ */

int test_chainer_bind(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = create_test_vector(10);

    Chainer c = Chain((Container *)vec1);
    chain_filter(&c, is_even);
    size_t count1 = chain_count(&c);
    ASSERT_EQUAL(count1, 3, "first bind wrong count");

    chain_bind(&c, (Container *)vec2);
    size_t count2 = chain_count(&c);
    ASSERT_EQUAL(count2, 5, "second bind wrong count");

    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_bind_reset(void) {
    Vector *vec1 = create_test_vector(5);
    Vector *vec2 = create_test_vector(10);

    Chainer c = Chain((Container *)vec1);
    chain_skip(&c, 2);
    chain_take(&c, 2);
    size_t count1 = chain_count(&c);
    ASSERT_EQUAL(count1, 2, "first bind wrong count");

    chain_bind(&c, (Container *)vec2);
    size_t count2 = chain_count(&c);
    ASSERT_EQUAL(count2, 2, "bind should reset skip/take counters");
    ASSERT_EQUAL(count2, 2, "second bind wrong count");

    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

/* ============================================================================
 * Chain Reuse Tests
 * ============================================================================ */

int test_chainer_reuse_same_container(void) {
    Vector *vec = create_test_vector(20);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    size_t count1 = chain_count(&c);
    size_t count2 = chain_count(&c);
    ASSERT_EQUAL(count1, 10, "first count wrong");
    ASSERT_EQUAL(count2, 10, "second count wrong (should reset)");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_reuse_different_pipelines(void) {
    Vector *vec = create_test_vector(20);
    Chainer c = Chain((Container *)vec);

    chain_filter(&c, is_even);
    size_t count1 = chain_count(&c);
    ASSERT_EQUAL(count1, 10, "first pipeline wrong");

    chain_filter(&c, is_positive);
    chain_take(&c, 5);
    size_t count2 = chain_count(&c);
    ASSERT_EQUAL(count2, 5, "second pipeline wrong");

    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests
 * ============================================================================ */

int test_chainer_large_filter(void) {
    const size_t N = 1000000;
    Vector *vec = create_test_vector(N);

    Timer t = timer_start();
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    double time = timer_elapsed(&t);
    printf("    Filter %zu elements: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, N / 2, "wrong filtered count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_large_filter_map_take(void) {
    const size_t N = 1000000;
    Vector *vec = create_test_vector(N);

    Timer t = timer_start();
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    chain_take(&c, 1000);
    size_t count = chain_count(&c);
    double time = timer_elapsed(&t);
    printf("    Filter+Map+Take %zu → 1000: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, 1000, "wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_large_full_pipeline(void) {
    const size_t N = 1000000;
    Vector *vec = create_test_vector(N);

    Timer t = timer_start();
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 10);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    chain_take(&c, 1000);
    size_t count = chain_count(&c);
    double time = timer_elapsed(&t);
    printf("    Full pipeline %zu → 1000: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, 1000, "wrong count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_large_multiple_filters(void) {
    const size_t N = 1000000;
    Vector *vec = create_test_vector(N);

    Timer t = timer_start();
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_filter(&c, is_positive);
    chain_filter(&c, is_even);
    size_t count = chain_count(&c);
    double time = timer_elapsed(&t);
    printf("    Three filters %zu elements: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, 500000, "wrong filtered count");
    chain_destroy(&c);
    vector_destroy(vec);
    return 1;
}

int test_chainer_large_flatten(void) {
    const size_t num_vectors = 1000;
    const size_t elements_per_vector = 1000;
    const size_t total = num_vectors * elements_per_vector;
    
    Vector *outer = create_vector_of_vectors(num_vectors, elements_per_vector);
    
    Timer t = timer_start();
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    size_t count = chain_count(&c);
    double time = timer_elapsed(&t);
    printf("    Flatten %zu vectors × %zu elements = %zu total: %.3f ms\n", 
           num_vectors, elements_per_vector, total, time * 1000);
    ASSERT_EQUAL(count, total, "flatten large wrong count");
    
    chain_destroy(&c);
    vector_destroy(outer);
    return 1;
}

int test_chainer_large_zip(void) {
    const size_t N = 1000000;
    Vector *vec1 = create_test_vector(N);
    Vector *vec2 = create_test_vector(N);
    
    Timer t = timer_start();
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, merge_ints, sizeof(int));
    size_t count = chain_count(&c);
    double time = timer_elapsed(&t);
    printf("    Zip %zu elements: %.3f ms\n", N, time * 1000);
    ASSERT_EQUAL(count, N, "zip large wrong count");
    
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
    return 1;
}

int test_chainer_large_collect_into_int_to_double(void) {
    const size_t N = 1000000;
    Vector *src = create_test_vector(N);
    Vector *dst = vector_create(sizeof(double));
    
    Timer t = timer_start();
    Chainer c = Chain((Container *)src);
    chain_filter(&c, is_even);
    chain_map(&c, int_to_double, sizeof(double));
    size_t n = chain_collect_into(&c, (Container *)dst);
    double time = timer_elapsed(&t);
    printf("    Collect int→double %zu elements (filtered to %zu): %.3f ms\n", 
           N, n, time * 1000);
    ASSERT_EQUAL(n, N / 2, "wrong collected count");
    
    chain_destroy(&c);
    vector_destroy(src);
    vector_destroy(dst);
    return 1;
}

/* ============================================================================
 * Null & Error Handling
 * ============================================================================ */

int test_chainer_null_operations(void) {
    ASSERT_EQUAL(chain_count(NULL), 0, "chain_count(NULL) should be 0");
    ASSERT_FALSE(chain_any(NULL, NULL), "chain_any(NULL) should be false");
    ASSERT_TRUE(chain_all(NULL, NULL), "chain_all(NULL) should be true");
    chain_for_each(NULL, NULL);
    ASSERT_NULL(chain_fold(NULL, NULL, NULL), "chain_fold(NULL) should return NULL");
    ASSERT_NULL(chain_find(NULL, NULL), "chain_find(NULL) should return NULL");
    ASSERT_NULL(chain_first(NULL), "chain_first(NULL) should return NULL");
    ASSERT_NULL(chain_collect(NULL), "chain_collect(NULL) should return NULL");
    ASSERT_EQUAL(chain_collect_into(NULL, NULL), 0, "chain_collect_into(NULL) should be 0");
    chain_bind(NULL, NULL);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  Chainer v2 Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Creation & Destruction */
    TEST(test_chainer_create);
    TEST(test_chainer_create_empty);
    TEST(test_chainer_destroy_null);

    /* Filter Stage */
    TEST(test_chainer_filter_only);
    TEST(test_chainer_filter_all_match);
    TEST(test_chainer_filter_no_match);
    TEST(test_chainer_multiple_filters);

    /* Map Stage */
    TEST(test_chainer_map_only);
    TEST(test_chainer_filter_map);

    /* Skip Stage */
    TEST(test_chainer_skip_only);
    TEST(test_chainer_skip_all);
    TEST(test_chainer_skip_zero);

    /* Take Stage */
    TEST(test_chainer_take_only);
    TEST(test_chainer_take_more_than_length);
    TEST(test_chainer_take_zero);

    /* Skip + Take */
    TEST(test_chainer_skip_take);

    /* Filter + Map + Take */
    TEST(test_chainer_filter_map_take);

    /* Full Pipeline */
    TEST(test_chainer_full_pipeline);

    /* Flatten Stage Tests */
    TEST(test_chainer_flatten_basic);
    TEST(test_chainer_flatten_empty_outer);
    TEST(test_chainer_flatten_empty_inner);
    TEST(test_chainer_flatten_with_filter);
    TEST(test_chainer_flatten_nested_with_map);

    /* Zip Stage Tests */
    TEST(test_chainer_zip_basic);
    TEST(test_chainer_zip_different_lengths);
    TEST(test_chainer_zip_with_filter);
    TEST(test_chainer_zip_with_map);
    TEST(test_chainer_zip_empty_left);
    TEST(test_chainer_zip_empty_right);

    /* Combined Tests */
    TEST(test_chainer_flatten_then_zip);

    /* Terminals */
    TEST(test_chainer_count_basic);
    TEST(test_chainer_count_empty);
    TEST(test_chainer_count_reset);
    TEST(test_chainer_any_true);
    TEST(test_chainer_any_false);
    TEST(test_chainer_any_with_predicate);
    TEST(test_chainer_all_true);
    TEST(test_chainer_all_false);
    TEST(test_chainer_all_empty);
    TEST(test_chainer_for_each_basic);
    TEST(test_chainer_fold_basic);
    TEST(test_chainer_fold_empty);
    TEST(test_chainer_fold_double);
    TEST(test_chainer_find_basic);
    TEST(test_chainer_find_not_found);
    TEST(test_chainer_find_with_predicate);
    TEST(test_chainer_first_basic);
    TEST(test_chainer_first_empty);
    TEST(test_chainer_collect_basic);
    TEST(test_chainer_collect_empty);
    TEST(test_chainer_collect_strings);
    TEST(test_chainer_collect_double);
    TEST(test_chainer_collect_into_basic);
    TEST(test_chainer_collect_into_existing);

    /* Cross-Type collect_into Tests */
    TEST(test_chainer_collect_into_int_to_double);
    TEST(test_chainer_collect_into_int_to_float);
    TEST(test_chainer_collect_into_filter_map_collect);
    TEST(test_chainer_collect_into_existing_double_vector);
    TEST(test_chainer_collect_into_chain_reuse_different_types);

    /* Chain Bind */
    TEST(test_chainer_bind);
    TEST(test_chainer_bind_reset);

    /* Chain Reuse */
    TEST(test_chainer_reuse_same_container);
    TEST(test_chainer_reuse_different_pipelines);

    /* Large Dataset Tests */
    printf("\n--- Large Dataset Tests ---\n");
    TEST(test_chainer_large_filter);
    TEST(test_chainer_large_filter_map_take);
    TEST(test_chainer_large_full_pipeline);
    TEST(test_chainer_large_multiple_filters);
    TEST(test_chainer_large_flatten);
    TEST(test_chainer_large_zip);
    TEST(test_chainer_large_collect_into_int_to_double);

    /* Null Handling */
    TEST(test_chainer_null_operations);

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