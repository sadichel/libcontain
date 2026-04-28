/**
 * @file deque_test.c
 * @brief Deque unit tests
 *
 * Tests cover:
 * - Creation and destruction (with/without capacity, comparator, alignment)
 * - Push/pop operations (back and front)
 * - Insert and remove at arbitrary positions
 * - Element access (at, front, back, at_or_default)
 * - Range operations (insert_range, append, remove_range)
 * - Slicing, cloning, equals
 * - Find, contains, rfind
 * - Capacity management (reserve, shrink_to_fit, resize)
 * - In-place operations (reverse, sort, unique)
 * - Instance creation (new empty deque of same type)
 * - String mode (automatic strdup/free)
 * - Edge cases (empty deque, out of bounds, NULL)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdint.h>
#include "assertion.h"
#include "timer.h"

#include <contain/deque.h>

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

/* Force circular buffer wrap */
static void deque_force_wrap(Deque *deq, int n) {
    for (int i = 0; i < n; i++) {
        int val = i;
        deque_push_front(deq, &val);
    }
    for (int i = 0; i < n; i++) {
        deque_pop_front(deq);
    }
}

/* ============================================================================
 * Creation & Destruction
 * ============================================================================ */

int test_deque_create(void) {
    Deque *deq = deque_create(sizeof(int));
    ASSERT_NOT_NULL(deq, "deque_create failed");
    ASSERT_EQUAL(deque_len(deq), 0, "new deque not empty");
    ASSERT_TRUE(deque_capacity(deq) >= DEQUE_MIN_CAPACITY, "wrong initial capacity");
    deque_destroy(deq);
    return 1;
}

int test_deque_create_with_capacity(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 64);
    ASSERT_NOT_NULL(deq, "create_with_capacity failed");
    ASSERT_TRUE(deque_capacity(deq) >= 64, "capacity too small");
    deque_destroy(deq);
    return 1;
}

int test_deque_str_create(void) {
    Deque *deq = deque_str();
    ASSERT_NOT_NULL(deq, "deque_str failed");
    ASSERT_TRUE(deque_is_empty(deq), "string deque not empty");
    deque_destroy(deq);
    return 1;
}

int test_deque_destroy_null(void) {
    deque_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Alignment Tests
 * ============================================================================ */

int test_deque_alignment_32(void) {
    Deque *deq = deque_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(deq, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 8; i++) {
        item.data[0] = (uint8_t)i;
        ASSERT_EQUAL(deque_push_back(deq, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 8; i++) {
        const void *ptr = deque_at(deq, i);
        ASSERT_TRUE(((uintptr_t)ptr % 32) == 0, "element not 32-byte aligned");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_alignment_64(void) {
    Deque *deq = deque_create_aligned(sizeof(Aligned64), 64);
    ASSERT_NOT_NULL(deq, "create_aligned failed");

    Aligned64 item = {0};
    for (int i = 0; i < 8; i++) {
        item.data[0] = (uint8_t)i;
        ASSERT_EQUAL(deque_push_back(deq, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 8; i++) {
        const void *ptr = deque_at(deq, i);
        ASSERT_TRUE(((uintptr_t)ptr % 64) == 0, "element not 64-byte aligned");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_alignment_after_grow(void) {
    Deque *deq = deque_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(deq, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 200; i++) {
        item.data[0] = (uint8_t)(i & 0xFF);
        ASSERT_EQUAL(deque_push_back(deq, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 200; i++) {
        const void *ptr = deque_at(deq, i);
        ASSERT_TRUE(((uintptr_t)ptr % 32) == 0, "element misaligned after grow");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_alignment_after_wrap(void) {
    Deque *deq = deque_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(deq, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 20; i++) {
        item.data[0] = (uint8_t)i;
        deque_push_front(deq, &item);
    }

    for (size_t i = 0; i < 20; i++) {
        const void *ptr = deque_at(deq, i);
        ASSERT_TRUE(((uintptr_t)ptr % 32) == 0, "element misaligned after wrap");
    }

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Circular Buffer Tests
 * ============================================================================ */

int test_deque_circular_push_front(void) {
    Deque *deq = deque_create(sizeof(int));

    for (int i = 9; i >= 0; i--) {
        ASSERT_EQUAL(deque_push_front(deq, &i), LC_OK, "push_front failed");
    }
    ASSERT_EQUAL(deque_len(deq), 10, "wrong length");

    for (int i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i, "wrong value");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_circular_mixed_push(void) {
    Deque *deq = deque_create(sizeof(int));
    int a = 3, b = 4, c = 2, d = 5, e = 1, f = 6;

    deque_push_back(deq, &a);
    deque_push_back(deq, &b);
    deque_push_front(deq, &c);
    deque_push_back(deq, &d);
    deque_push_front(deq, &e);
    deque_push_back(deq, &f);

    ASSERT_EQUAL(deque_len(deq), 6, "wrong length");
    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i + 1, "wrong value");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_circular_wrap_correctness(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);

    for (int i = 0; i < 4; i++) deque_push_back(deq, &i);
    for (int i = 0; i < 4; i++) deque_pop_front(deq);
    for (int i = 3; i >= 0; i--) deque_push_front(deq, &i);

    ASSERT_EQUAL(deque_len(deq), 4, "wrong length");
    for (int i = 0; i < 4; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i, "wrong value after wrap");
    }

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Push & Pop Tests
 * ============================================================================ */

int test_deque_push_back(void) {
    Deque *deq = deque_create(sizeof(int));
    int vals[] = {10, 20, 30, 40, 50};

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(deque_push_back(deq, &vals[i]), LC_OK, "push_back failed");
    }
    ASSERT_EQUAL(deque_len(deq), 5, "wrong length");

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), vals[i], "wrong value");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_push_front(void) {
    Deque *deq = deque_create(sizeof(int));
    int vals[] = {10, 20, 30};

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(deque_push_front(deq, &vals[i]), LC_OK, "push_front failed");
    }

    ASSERT_EQUAL(*(int *)deque_at(deq, 0), 30, "wrong front");
    ASSERT_EQUAL(*(int *)deque_at(deq, 2), 10, "wrong back");

    deque_destroy(deq);
    return 1;
}

int test_deque_push_strings(void) {
    Deque *deq = deque_str();
    const char *strings[] = {"hello", "world", "test"};

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(deque_push_back(deq, strings[i]), LC_OK, "push string failed");
    }

    for (int i = 0; i < 3; i++) {
        ASSERT_STR_EQUAL((char *)deque_at(deq, i), strings[i], "wrong string");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_pop_back(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    for (int i = 9; i >= 0; i--) {
        ASSERT_EQUAL(deque_pop_back(deq), LC_OK, "pop_back failed");
        ASSERT_EQUAL(deque_len(deq), (size_t)i, "wrong length after pop");
    }

    ASSERT_EQUAL(deque_pop_back(deq), LC_EBOUNDS, "pop on empty should fail");
    deque_destroy(deq);
    return 1;
}

int test_deque_pop_front(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    for (int i = 0; i < 10; i++) {
        ASSERT_EQUAL(deque_pop_front(deq), LC_OK, "pop_front failed");
        ASSERT_EQUAL(deque_len(deq), (size_t)(9 - i), "wrong length after pop");
    }

    ASSERT_EQUAL(deque_pop_front(deq), LC_EBOUNDS, "pop on empty should fail");
    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Insert & Remove Tests
 * ============================================================================ */

int test_deque_insert_middle(void) {
    Deque *deq = deque_create(sizeof(int));
    int a = 10, b = 30, mid = 20;
    deque_push_back(deq, &a);
    deque_push_back(deq, &b);

    ASSERT_EQUAL(deque_insert(deq, 1, &mid), LC_OK, "insert failed");
    ASSERT_EQUAL(deque_len(deq), 3, "wrong length");
    ASSERT_EQUAL(*(int *)deque_at(deq, 1), 20, "wrong value at inserted position");

    deque_destroy(deq);
    return 1;
}

int test_deque_insert_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 5; i++) deque_push_back(deq, &i);

    int mid = 99;
    ASSERT_EQUAL(deque_insert(deq, 2, &mid), LC_OK, "insert in wrapped buffer failed");
    ASSERT_EQUAL(*(int *)deque_at(deq, 2), 99, "wrong inserted value");

    deque_destroy(deq);
    return 1;
}

int test_deque_remove(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_remove(deq, 5), LC_OK, "remove failed");
    ASSERT_EQUAL(deque_len(deq), 9, "wrong length");
    ASSERT_EQUAL(*(int *)deque_at(deq, 5), 6, "wrong value after removal");

    deque_destroy(deq);
    return 1;
}

int test_deque_remove_range(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_remove_range(deq, 3, 7), LC_OK, "remove_range failed");
    ASSERT_EQUAL(deque_len(deq), 6, "wrong length");

    int expected[] = {0, 1, 2, 7, 8, 9};
    for (size_t i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), expected[i], "wrong value after range remove");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_remove_range_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 7; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_remove_range(deq, 1, 4), LC_OK, "remove range on wrapped failed");
    ASSERT_EQUAL(deque_len(deq), 4, "wrong length");
    ASSERT_EQUAL(*(int *)deque_at(deq, 0), 0, "wrong first");
    ASSERT_EQUAL(*(int *)deque_at(deq, 1), 4, "wrong second");

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Access Tests
 * ============================================================================ */

int test_deque_at(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), (int)i, "wrong value at index");
    }

    ASSERT_NULL(deque_at(deq, 999), "out of bounds should return NULL");
    deque_destroy(deq);
    return 1;
}

int test_deque_at_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i, "wrong value at wrapped index");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_front_back(void) {
    Deque *deq = deque_create(sizeof(int));
    int first = 10, last = 20;
    deque_push_back(deq, &first);
    deque_push_back(deq, &last);

    ASSERT_EQUAL(*(int *)deque_front(deq), 10, "wrong front");
    ASSERT_EQUAL(*(int *)deque_back(deq), 20, "wrong back");

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Capacity Tests
 * ============================================================================ */

int test_deque_reserve(void) {
    Deque *deq = deque_create(sizeof(int));
    ASSERT_EQUAL(deque_reserve(deq, 256), LC_OK, "reserve failed");
    ASSERT_TRUE(deque_capacity(deq) >= 256, "capacity not increased");
    deque_destroy(deq);
    return 1;
}

int test_deque_reserve_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_reserve(deq, 256), LC_OK, "reserve on wrapped failed");
    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i, "value lost after reserve");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_shrink_to_fit(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 100);
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    size_t cap_before = deque_capacity(deq);
    ASSERT_EQUAL(deque_shrink_to_fit(deq), LC_OK, "shrink_to_fit failed");
    ASSERT_TRUE(deque_capacity(deq) < cap_before, "capacity not reduced");
    ASSERT_EQUAL(deque_len(deq), 10, "length changed");

    deque_destroy(deq);
    return 1;
}

int test_deque_resize_grow(void) {
    Deque *deq = deque_create(sizeof(int));
    int val = 42;
    deque_push_back(deq, &val);

    ASSERT_EQUAL(deque_resize(deq, 5), LC_OK, "resize grow failed");
    ASSERT_EQUAL(deque_len(deq), 5, "wrong length");
    ASSERT_EQUAL(*(int *)deque_at(deq, 0), 42, "original element lost");

    for (size_t i = 1; i < 5; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), 0, "new slot not zeroed");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_resize_shrink(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_resize(deq, 4), LC_OK, "resize shrink failed");
    ASSERT_EQUAL(deque_len(deq), 4, "wrong length");
    for (int i = 0; i < 4; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i, "wrong value after shrink");
    }

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Clear Tests
 * ============================================================================ */

int test_deque_clear(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 100; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_clear(deq), LC_OK, "clear failed");
    ASSERT_TRUE(deque_is_empty(deq), "not empty after clear");

    int val = 42;
    deque_push_back(deq, &val);
    ASSERT_EQUAL(deque_len(deq), 1, "cannot push after clear");

    deque_destroy(deq);
    return 1;
}

int test_deque_clear_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_clear(deq), LC_OK, "clear on wrapped failed");
    ASSERT_TRUE(deque_is_empty(deq), "not empty after clear");

    deque_destroy(deq);
    return 1;
}

int test_deque_clear_strings(void) {
    Deque *deq = deque_str();
    deque_push_back(deq, "hello");
    deque_push_back(deq, "world");

    ASSERT_EQUAL(deque_clear(deq), LC_OK, "clear strings failed");
    ASSERT_TRUE(deque_is_empty(deq), "not empty after clear");

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Reverse Tests
 * ============================================================================ */

int test_deque_reverse_inplace(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_reverse_inplace(deq), LC_OK, "reverse_inplace failed");
    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), (int)(9 - i), "wrong reverse order");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_reverse_inplace_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 5; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_reverse_inplace(deq), LC_OK, "reverse_inplace on wrapped failed");
    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), 4 - i, "wrong reverse order");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_reverse(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    Deque *rev = deque_reverse(deq);
    ASSERT_NOT_NULL(rev, "reverse failed");
    ASSERT_EQUAL(deque_len(rev), 10, "wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)deque_at(rev, i), (int)(9 - i), "wrong reverse order");
    }

    deque_destroy(deq);
    deque_destroy(rev);
    return 1;
}

/* ============================================================================
 * Sort & Unique Tests
 * ============================================================================ */

int test_deque_sort(void) {
    Deque *deq = deque_create(sizeof(int));
    int vals[] = {50, 10, 40, 20, 30};
    for (int i = 0; i < 5; i++) deque_push_back(deq, &vals[i]);

    ASSERT_EQUAL(deque_sort(deq, cmp_int), LC_OK, "sort failed");
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), (int)(10 + i * 10), "wrong sort order");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_sort_strings(void) {
    Deque *deq = deque_str();
    deque_push_back(deq, "zebra");
    deque_push_back(deq, "apple");
    deque_push_back(deq, "mango");
    deque_push_back(deq, "banana");

    ASSERT_EQUAL(deque_sort(deq, cmp_str_ptr), LC_OK, "sort strings failed");
    ASSERT_STR_EQUAL((char *)deque_at(deq, 0), "apple", "wrong first");
    ASSERT_STR_EQUAL((char *)deque_at(deq, 3), "zebra", "wrong last");

    deque_destroy(deq);
    return 1;
}

int test_deque_unique(void) {
    Deque *deq = deque_create(sizeof(int));
    int vals[] = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
    for (int i = 0; i < 10; i++) deque_push_back(deq, &vals[i]);

    ASSERT_EQUAL(deque_unique(deq), LC_OK, "unique failed");
    ASSERT_EQUAL(deque_len(deq), 5, "wrong length after unique");

    int expected[] = {1, 2, 3, 4, 5};
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), expected[i], "wrong unique value");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_unique_strings(void) {
    Deque *deq = deque_str();
    deque_push_back(deq, "apple");
    deque_push_back(deq, "apple");
    deque_push_back(deq, "banana");
    deque_push_back(deq, "cherry");
    deque_push_back(deq, "cherry");

    ASSERT_EQUAL(deque_unique(deq), LC_OK, "unique strings failed");
    ASSERT_EQUAL(deque_len(deq), 3, "wrong length after unique");

    ASSERT_STR_EQUAL((char *)deque_at(deq, 0), "apple", "wrong first");
    ASSERT_STR_EQUAL((char *)deque_at(deq, 1), "banana", "wrong second");
    ASSERT_STR_EQUAL((char *)deque_at(deq, 2), "cherry", "wrong third");

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Find & Search Tests
 * ============================================================================ */

int test_deque_find(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    int needle = 5;
    ASSERT_EQUAL(deque_find(deq, &needle), 5, "find wrong position");

    needle = 99;
    ASSERT_EQUAL(deque_find(deq, &needle), DEQ_NPOS, "find should return NPOS");

    deque_destroy(deq);
    return 1;
}

int test_deque_find_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    int needle = 4;
    ASSERT_EQUAL(deque_find(deq, &needle), 4, "find in wrapped failed");

    deque_destroy(deq);
    return 1;
}

int test_deque_rfind(void) {
    Deque *deq = deque_create(sizeof(int));
    int vals[] = {1, 2, 3, 2, 1, 2, 3};
    for (int i = 0; i < 7; i++) deque_push_back(deq, &vals[i]);

    int needle = 2;
    ASSERT_EQUAL(deque_rfind(deq, &needle), 5, "rfind wrong position");

    deque_destroy(deq);
    return 1;
}

int test_deque_contains(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    int present = 5, absent = 99;
    ASSERT_TRUE(deque_contains(deq, &present), "contains false for existing");
    ASSERT_FALSE(deque_contains(deq, &absent), "contains true for missing");

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Clone & Slice Tests
 * ============================================================================ */

int test_deque_clone(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    Deque *clone = deque_clone(deq);
    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(deque_len(clone), 10, "clone wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)deque_at(clone, i), (int)i, "clone wrong value");
    }

    int newval = 999;
    deque_set(clone, 0, &newval);
    ASSERT_EQUAL(*(int *)deque_at(deq, 0), 0, "original changed after clone modification");

    deque_destroy(deq);
    deque_destroy(clone);
    return 1;
}

int test_deque_clone_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    Deque *clone = deque_clone(deq);
    ASSERT_NOT_NULL(clone, "clone of wrapped failed");
    ASSERT_EQUAL(deque_len(clone), 6, "clone wrong length");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(clone, i), i, "clone wrong value");
    }

    deque_destroy(deq);
    deque_destroy(clone);
    return 1;
}

int test_deque_clone_strings(void) {
    Deque *deq = deque_str();
    deque_push_back(deq, "hello");
    deque_push_back(deq, "world");

    Deque *clone = deque_clone(deq);
    ASSERT_NOT_NULL(clone, "clone strings failed");
    ASSERT_STR_EQUAL((char *)deque_at(clone, 0), "hello", "clone wrong string");

    deque_destroy(deq);
    deque_destroy(clone);
    return 1;
}

int test_deque_slice(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    Deque *slice = deque_slice(deq, 3, 7);
    ASSERT_NOT_NULL(slice, "slice failed");
    ASSERT_EQUAL(deque_len(slice), 4, "slice wrong length");

    int expected[] = {3, 4, 5, 6};
    for (size_t i = 0; i < 4; i++) {
        ASSERT_EQUAL(*(int *)deque_at(slice, i), expected[i], "slice wrong value");
    }

    deque_destroy(deq);
    deque_destroy(slice);
    return 1;
}

int test_deque_slice_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    Deque *slice = deque_slice(deq, 2, 5);
    ASSERT_NOT_NULL(slice, "slice of wrapped failed");
    ASSERT_EQUAL(deque_len(slice), 3, "slice wrong length");

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(*(int *)deque_at(slice, i), i + 2, "slice wrong value");
    }

    deque_destroy(deq);
    deque_destroy(slice);
    return 1;
}

/* ============================================================================
 * Append Tests
 * ============================================================================ */

int test_deque_append(void) {
    Deque *a = deque_create(sizeof(int));
    Deque *b = deque_create(sizeof(int));
    for (int i = 0; i < 3; i++) {
        deque_push_back(a, &i);
        deque_push_back(b, &(int){i + 3});
    }

    ASSERT_EQUAL(deque_append(a, b), LC_OK, "append failed");
    ASSERT_EQUAL(deque_len(a), 6, "wrong length after append");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(a, i), i, "wrong appended value");
    }

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

int test_deque_self_append(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 3; i++) deque_push_back(deq, &i);

    ASSERT_EQUAL(deque_append(deq, deq), LC_OK, "self-append failed");
    ASSERT_EQUAL(deque_len(deq), 6, "wrong length after self-append");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)deque_at(deq, i), i % 3, "wrong self-appended value");
    }

    deque_destroy(deq);
    return 1;
}

int test_deque_append_strings(void) {
    Deque *a = deque_str();
    Deque *b = deque_str();
    deque_push_back(a, "hello");
    deque_push_back(a, "world");
    deque_push_back(b, "test");
    deque_push_back(b, "append");

    ASSERT_EQUAL(deque_append(a, b), LC_OK, "append strings failed");
    ASSERT_EQUAL(deque_len(a), 4, "wrong length");
    ASSERT_STR_EQUAL((char *)deque_at(a, 2), "test", "wrong appended string");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

/* ============================================================================
 * Hash Tests
 * ============================================================================ */

int test_deque_hash(void) {
    Deque *a = deque_create(sizeof(int));
    Deque *b = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        deque_push_back(a, &i);
        deque_push_back(b, &i);
    }

    size_t ha = deque_hash(a);
    size_t hb = deque_hash(b);
    ASSERT_EQUAL(ha, hb, "identical deques different hash");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

int test_deque_hash_wrapped_vs_linear(void) {
    Deque *a = deque_create(sizeof(int));
    Deque *b = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(b, 4);

    int vals[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        deque_push_back(a, &vals[i]);
        deque_push_back(b, &vals[i]);
    }

    ASSERT_EQUAL(deque_hash(a), deque_hash(b), "wrapped and linear hash mismatch");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

/* ============================================================================
 * Equals Tests     
 * ============================================================================ */

int test_deque_equals_identical(void) {
    Deque *a = deque_create(sizeof(int));
    Deque *b = deque_create(sizeof(int));
    for (int i = 0; i < 5; i++) {
        deque_push_back(a, &i);
        deque_push_back(b, &i);
    }

    ASSERT_TRUE(deque_equals(a, b), "identical deques should be equal");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

int test_deque_equals_different_values(void) {
    Deque *a = deque_create(sizeof(int));
    Deque *b = deque_create(sizeof(int));
    
    for (int i = 0; i < 5; i++) deque_push_back(a, &i);
    for (int i = 0; i < 4; i++) deque_push_back(b, &i);

    int extra = 99;
    deque_push_back(b, &extra);
    ASSERT_FALSE(deque_equals(a, b), "different values should not be equal");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

int test_deque_equals_strings_same_order(void) {
    Deque *a = deque_str();
    Deque *b = deque_str();
    deque_push_back(a, "hello");
    deque_push_back(a, "world");
    deque_push_back(b, "hello");
    deque_push_back(b, "world");

    ASSERT_TRUE(deque_equals(a, b), "same order strings should be equal");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

int test_deque_equals_strings_different_order(void) {
    Deque *a = deque_str();
    Deque *b = deque_str();
    deque_push_back(a, "hello");
    deque_push_back(a, "world");
    deque_push_back(b, "world");
    deque_push_back(b, "hello");

    ASSERT_FALSE(deque_equals(a, b), "different order should NOT be equal");

    deque_destroy(a);
    deque_destroy(b);
    return 1;
}

/* ============================================================================
 * Iterator Tests
 * ============================================================================ */

int test_deque_iterator(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    Iterator it = deque_iter(deq);
    int count = 0;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "iterator wrong value");
        count++;
    }
    ASSERT_EQUAL(count, 10, "iterator wrong count");

    deque_destroy(deq);
    return 1;
}

int test_deque_iterator_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    Iterator it = deque_iter(deq);
    int count = 0;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "iterator wrong value on wrapped");
        count++;
    }
    ASSERT_EQUAL(count, 6, "iterator wrong count");

    deque_destroy(deq);
    return 1;
}

int test_deque_iterator_reversed(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    Iterator it = deque_iter_reversed(deq);
    int count = 9;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "reverse iterator wrong value");
        count--;
    }
    ASSERT_EQUAL(count, -1, "reverse iterator wrong count");

    deque_destroy(deq);
    return 1;
}

int test_deque_iterator_reversed_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    Iterator it = deque_iter_reversed(deq);
    int count = 5;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "reverse iterator wrong value on wrapped");
        count--;
    }
    ASSERT_EQUAL(count, -1, "reverse iterator wrong count");

    deque_destroy(deq);
    return 1;
}

int test_deque_iterator_strings(void) {
    Deque *deq = deque_str();
    deque_push_back(deq, "hello");
    deque_push_back(deq, "world");

    Iterator it = deque_iter(deq);
    char *p;
    p = (char *)iter_next(&it);
    ASSERT_STR_EQUAL(p, "hello", "iterator wrong string");
    p = (char *)iter_next(&it);
    ASSERT_STR_EQUAL(p, "world", "iterator wrong string");
    ASSERT_NULL(iter_next(&it), "iterator not exhausted");

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * To Array Tests
 * ============================================================================ */

int test_deque_to_array(void) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < 10; i++) deque_push_back(deq, &i);

    Array *arr = deque_to_array(deq);
    ASSERT_NOT_NULL(arr, "to_array failed");
    ASSERT_EQUAL(arr->len, 10, "array wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)((uint8_t *)arr->items + i * arr->stride), (int)i, "array wrong value");
    }

    free(arr);
    deque_destroy(deq);
    return 1;
}

int test_deque_to_array_wrapped(void) {
    Deque *deq = deque_create_with_capacity(sizeof(int), 8);
    deque_force_wrap(deq, 4);

    for (int i = 0; i < 6; i++) deque_push_back(deq, &i);

    Array *arr = deque_to_array(deq);
    ASSERT_NOT_NULL(arr, "to_array on wrapped failed");
    ASSERT_EQUAL(arr->len, 6, "array wrong length");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)((uint8_t *)arr->items + i * arr->stride), i, "array wrong value");
    }

    free(arr);
    deque_destroy(deq);
    return 1;
}

int test_deque_to_array_strings(void) {
    Deque *deq = deque_str();
    deque_push_back(deq, "hello");
    deque_push_back(deq, "world");

    Array *arr = deque_to_array(deq);
    ASSERT_NOT_NULL(arr, "to_array strings failed");
    ASSERT_EQUAL(arr->len, 2, "array wrong length");

    char **table = (char **)arr->items;
    ASSERT_STR_EQUAL(table[0], "hello", "array wrong string");
    ASSERT_STR_EQUAL(table[1], "world", "array wrong string");

    free(arr);
    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests
 * ============================================================================ */

int test_deque_large_push(void) {
    const size_t N = 1000000;
    Deque *deq = deque_create(sizeof(int));
    ASSERT_NOT_NULL(deq, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_EQUAL_FMT(deque_push_back(deq, &val), LC_OK, "push failed at %zu", i);
    }
    double push_time = timer_elapsed(&t);
    printf("    Push %zu elements: %.3f ms\n", N, push_time * 1000);

    ASSERT_EQUAL(deque_len(deq), N, "wrong length after pushes");

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL_FMT(*(int *)deque_at(deq, i), (int)i, "wrong value at %zu", i);
    }
    double access_time = timer_elapsed(&t);
    printf("    Access %zu elements: %.3f ms\n", N, access_time * 1000);

    deque_destroy(deq);
    return 1;
}

int test_deque_large_push_pop_alternating(void) {
    const size_t N = 100000;
    Deque *deq = deque_create(sizeof(int));
    ASSERT_NOT_NULL(deq, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        deque_push_back(deq, &val);
        deque_pop_front(deq);
    }
    double time = timer_elapsed(&t);
    printf("    Alternating push/pop %zu times: %.3f ms\n", N, time * 1000);

    ASSERT_TRUE(deque_is_empty(deq), "deque not empty after alternating ops");

    deque_destroy(deq);
    return 1;
}

int test_deque_large_clone(void) {
    const size_t N = 1000000;
    Deque *deq = deque_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        deque_push_back(deq, &val);
    }

    Timer t = timer_start();
    Deque *clone = deque_clone(deq);
    double clone_time = timer_elapsed(&t);
    printf("    Clone %zu elements: %.3f ms\n", N, clone_time * 1000);

    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(deque_len(clone), N, "clone wrong length");

    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL_FMT(*(int *)deque_at(clone, i), (int)i, "clone wrong value at %zu", i);
    }

    deque_destroy(deq);
    deque_destroy(clone);
    return 1;
}

int test_deque_large_strings(void) {
    const size_t N = 100000;
    Deque *deq = deque_str();
    char buf[32];

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_EQUAL_FMT(deque_push_back(deq, buf), LC_OK, "push string failed at %zu", i);
    }
    double push_time = timer_elapsed(&t);
    printf("    Push %zu strings: %.3f ms\n", N, push_time * 1000);

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        char *s = (char *)deque_at(deq, i);
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_STR_EQUAL_FMT(s, buf, "wrong string at %zu", i);
    }
    double access_time = timer_elapsed(&t);
    printf("    Access %zu strings: %.3f ms\n", N, access_time * 1000);

    deque_destroy(deq);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  Deque Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Creation & Destruction */
    TEST(test_deque_create);
    TEST(test_deque_create_with_capacity);
    TEST(test_deque_str_create);
    TEST(test_deque_destroy_null);

    /* Alignment */
    TEST(test_deque_alignment_32);
    TEST(test_deque_alignment_64);
    TEST(test_deque_alignment_after_grow);
    TEST(test_deque_alignment_after_wrap);

    /* Circular Buffer */
    TEST(test_deque_circular_push_front);
    TEST(test_deque_circular_mixed_push);
    TEST(test_deque_circular_wrap_correctness);

    /* Push & Pop */
    TEST(test_deque_push_back);
    TEST(test_deque_push_front);
    TEST(test_deque_push_strings);
    TEST(test_deque_pop_back);
    TEST(test_deque_pop_front);

    /* Insert & Remove */
    TEST(test_deque_insert_middle);
    TEST(test_deque_insert_wrapped);
    TEST(test_deque_remove);
    TEST(test_deque_remove_range);
    TEST(test_deque_remove_range_wrapped);

    /* Access */
    TEST(test_deque_at);
    TEST(test_deque_at_wrapped);
    TEST(test_deque_front_back);

    /* Capacity */
    TEST(test_deque_reserve);
    TEST(test_deque_reserve_wrapped);
    TEST(test_deque_shrink_to_fit);
    TEST(test_deque_resize_grow);
    TEST(test_deque_resize_shrink);

    /* Clear */
    TEST(test_deque_clear);
    TEST(test_deque_clear_wrapped);
    TEST(test_deque_clear_strings);

    /* Reverse */
    TEST(test_deque_reverse_inplace);
    TEST(test_deque_reverse_inplace_wrapped);
    TEST(test_deque_reverse);

    /* Sort & Unique */
    TEST(test_deque_sort);
    TEST(test_deque_sort_strings);
    TEST(test_deque_unique);
    TEST(test_deque_unique_strings);

    /* Find & Search */
    TEST(test_deque_find);
    TEST(test_deque_find_wrapped);
    TEST(test_deque_rfind);
    TEST(test_deque_contains);

    /* Clone & Slice */
    TEST(test_deque_clone);
    TEST(test_deque_clone_wrapped);
    TEST(test_deque_clone_strings);
    TEST(test_deque_slice);
    TEST(test_deque_slice_wrapped);

    /* Append */
    TEST(test_deque_append);
    TEST(test_deque_self_append);
    TEST(test_deque_append_strings);

    /* Hash */
    TEST(test_deque_hash);
    TEST(test_deque_hash_wrapped_vs_linear);

    /* Equals */
    TEST(test_deque_equals_identical);
    TEST(test_deque_equals_different_values);
    TEST(test_deque_equals_strings_same_order);
    TEST(test_deque_equals_strings_different_order);

    /* Iterator */
    TEST(test_deque_iterator);
    TEST(test_deque_iterator_wrapped);
    TEST(test_deque_iterator_reversed);
    TEST(test_deque_iterator_reversed_wrapped);
    TEST(test_deque_iterator_strings);

    /* To Array */
    TEST(test_deque_to_array);
    TEST(test_deque_to_array_wrapped);
    TEST(test_deque_to_array_strings);

    /* Large Dataset Tests */
    TEST(test_deque_large_push);
    TEST(test_deque_large_push_pop_alternating);
    TEST(test_deque_large_clone);
    TEST(test_deque_large_strings);

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
