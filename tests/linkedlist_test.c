/**
 * @file linkedlist_test.c
 * @brief LinkedList unit tests
 *
 * Tests cover:
 * - Creation and destruction (with/without comparator, alignment, allocator)
 * - Push/pop operations (back and front)
 * - Insert and remove at arbitrary positions
 * - Element access (at, front, back, get_ptr, at_or_default)
 * - Range operations (insert_range, append, remove_range)
 * - Sublist extraction
 * - Splicing between lists (with same/different allocators)
 * - Cloning, hash, equals
 * - Find, contains, rfind
 * - In-place operations (reverse, sort, unique)
 * - Instance creation (new empty list of same type)
 * - String mode (automatic strdup/free)
 * - Edge cases (empty list, out of bounds, NULL)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdint.h>
#include "assertion.h"
#include "timer.h"

#define LINKEDLIST_IMPLEMENTATION
#include <contain/linkedlist.h>

/* ============================================================================
 * Test Helpers
 * ============================================================================ */

typedef struct {
    int   id;
    char *name;
} Person;

static int cmp_person(const void *a, const void *b) {
    return ((const Person *)a)->id - ((const Person *)b)->id;
}

static int cmp_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int cmp_str_ptr(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* Alignment test types */
typedef struct {
    alignas(32) uint8_t data[32];
} Aligned32;

typedef struct {
    alignas(64) uint8_t data[64];
} Aligned64;

static bool aligned32_eq(const Aligned32 *a, const Aligned32 *b) {
    return memcmp(a->data, b->data, 32) == 0;
}

static bool aligned64_eq(const Aligned64 *a, const Aligned64 *b) {
    return memcmp(a->data, b->data, 64) == 0;
}

/* Fill list with ints [0, n) */
static void linkedlist_fill(LinkedList *list, int n) {
    for (int i = 0; i < n; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }
}

/* ============================================================================
 * Creation & Destruction
 * ============================================================================ */

int test_linkedlist_create(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    ASSERT_NOT_NULL(list, "linkedlist_create failed");
    ASSERT_EQUAL(linkedlist_len(list), 0, "new list not empty");
    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_str_create(void) {
    LinkedList *list = linkedlist_str();
    ASSERT_NOT_NULL(list, "linkedlist_str failed");
    ASSERT_TRUE(linkedlist_is_empty(list), "string list not empty");
    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_destroy_null(void) {
    linkedlist_destroy(NULL);
    return 1;
}

/* ============================================================================
 * Alignment Tests
 * ============================================================================ */

int test_linkedlist_alignment_32(void) {
    LinkedList *list = linkedlist_create_aligned(sizeof(Aligned32), 32);
    ASSERT_NOT_NULL(list, "create_aligned failed");

    Aligned32 item = {0};
    for (int i = 0; i < 8; i++) {
        item.data[0] = (uint8_t)i;
        ASSERT_EQUAL(linkedlist_push_back(list, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 8; i++) {
        const void *ptr = linkedlist_at(list, i);
        ASSERT_TRUE(((uintptr_t)ptr % 32) == 0, "element not 32-byte aligned");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_alignment_64(void) {
    LinkedList *list = linkedlist_create_aligned(sizeof(Aligned64), 64);
    ASSERT_NOT_NULL(list, "create_aligned failed");

    Aligned64 item = {0};
    for (int i = 0; i < 8; i++) {
        item.data[0] = (uint8_t)i;
        ASSERT_EQUAL(linkedlist_push_back(list, &item), LC_OK, "push failed");
    }

    for (size_t i = 0; i < 8; i++) {
        const void *ptr = linkedlist_at(list, i);
        ASSERT_TRUE(((uintptr_t)ptr % 64) == 0, "element not 64-byte aligned");
    }

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Push Tests
 * ============================================================================ */

int test_linkedlist_push_back(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int vals[] = {10, 20, 30, 40, 50};

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(linkedlist_push_back(list, &vals[i]), LC_OK, "push_back failed");
    }
    ASSERT_EQUAL(linkedlist_len(list), 5, "wrong length");

    for (int i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), vals[i], "wrong value");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_push_front(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int vals[] = {10, 20, 30};

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(linkedlist_push_front(list, &vals[i]), LC_OK, "push_front failed");
    }

    ASSERT_EQUAL(*(int *)linkedlist_front(list), 30, "wrong front");
    ASSERT_EQUAL(*(int *)linkedlist_back(list), 10, "wrong back");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_push_strings(void) {
    LinkedList *list = linkedlist_str();
    const char *strings[] = {"hello", "world", "test"};

    for (int i = 0; i < 3; i++) {
        ASSERT_EQUAL(linkedlist_push_back(list, strings[i]), LC_OK, "push string failed");
    }

    for (int i = 0; i < 3; i++) {
        ASSERT_STR_EQUAL((char *)linkedlist_at(list, i), strings[i], "wrong string");
    }

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Pop Tests
 * ============================================================================ */

int test_linkedlist_pop_back(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    for (int i = 9; i >= 0; i--) {
        ASSERT_EQUAL(linkedlist_pop_back(list), LC_OK, "pop_back failed");
        ASSERT_EQUAL(linkedlist_len(list), (size_t)i, "wrong length after pop");
    }

    ASSERT_EQUAL(linkedlist_pop_back(list), LC_EBOUNDS, "pop on empty should fail");
    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_pop_front(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    for (int i = 0; i < 10; i++) {
        ASSERT_EQUAL(linkedlist_pop_front(list), LC_OK, "pop_front failed");
        ASSERT_EQUAL(linkedlist_len(list), (size_t)(9 - i), "wrong length after pop");
    }

    ASSERT_EQUAL(linkedlist_pop_front(list), LC_EBOUNDS, "pop on empty should fail");
    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_pop_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "first");
    linkedlist_push_back(list, "second");
    linkedlist_push_back(list, "third");

    ASSERT_EQUAL(linkedlist_pop_back(list), LC_OK, "pop_back string failed");
    ASSERT_EQUAL(linkedlist_len(list), 2, "wrong length");
    ASSERT_STR_EQUAL((char *)linkedlist_back(list), "second", "wrong back");

    ASSERT_EQUAL(linkedlist_pop_front(list), LC_OK, "pop_front string failed");
    ASSERT_EQUAL(linkedlist_len(list), 1, "wrong length");
    ASSERT_STR_EQUAL((char *)linkedlist_front(list), "second", "wrong front");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Insert Tests
 * ============================================================================ */

int test_linkedlist_insert_middle(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int a = 10, b = 30, mid = 20;
    linkedlist_push_back(list, &a);
    linkedlist_push_back(list, &b);

    ASSERT_EQUAL(linkedlist_insert(list, 1, &mid), LC_OK, "insert failed");
    ASSERT_EQUAL(linkedlist_len(list), 3, "wrong length");
    ASSERT_EQUAL(*(int *)linkedlist_at(list, 1), 20, "wrong value at inserted position");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_insert_front(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int a = 20, b = 10;
    linkedlist_push_back(list, &a);

    ASSERT_EQUAL(linkedlist_insert(list, 0, &b), LC_OK, "insert at front failed");
    ASSERT_EQUAL(*(int *)linkedlist_front(list), 10, "wrong front");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_insert_back(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int a = 10, b = 20;
    linkedlist_push_back(list, &a);

    ASSERT_EQUAL(linkedlist_insert(list, 1, &b), LC_OK, "insert at back failed");
    ASSERT_EQUAL(*(int *)linkedlist_back(list), 20, "wrong back");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_insert_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "first");
    linkedlist_push_back(list, "third");

    ASSERT_EQUAL(linkedlist_insert(list, 1, "second"), LC_OK, "insert string failed");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 1), "second", "wrong inserted string");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 2), "third", "wrong shifted string");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Remove Tests
 * ============================================================================ */

int test_linkedlist_remove(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    ASSERT_EQUAL(linkedlist_remove(list, 5), LC_OK, "remove failed");
    ASSERT_EQUAL(linkedlist_len(list), 9, "wrong length");
    ASSERT_EQUAL(*(int *)linkedlist_at(list, 5), 6, "wrong value after removal");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_remove_range(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    ASSERT_EQUAL(linkedlist_remove_range(list, 3, 7), LC_OK, "remove_range failed");
    ASSERT_EQUAL(linkedlist_len(list), 6, "wrong length");

    int expected[] = {0, 1, 2, 7, 8, 9};
    for (size_t i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), expected[i], "wrong value after range remove");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_remove_strings(void) {
    LinkedList *list = linkedlist_str();
    const char *strings[] = {"a", "b", "c", "d", "e"};
    for (int i = 0; i < 5; i++) linkedlist_push_back(list, strings[i]);

    ASSERT_EQUAL(linkedlist_remove(list, 2), LC_OK, "remove string failed");
    ASSERT_EQUAL(linkedlist_len(list), 4, "wrong length");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 2), "d", "wrong shifted string");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Access Tests
 * ============================================================================ */

int test_linkedlist_at(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), (int)i, "wrong value at index");
    }

    ASSERT_NULL(linkedlist_at(list, 999), "out of bounds should return NULL");
    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_front_back(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int first = 10, last = 20;
    linkedlist_push_back(list, &first);
    linkedlist_push_back(list, &last);

    ASSERT_EQUAL(*(int *)linkedlist_front(list), 10, "wrong front");
    ASSERT_EQUAL(*(int *)linkedlist_back(list), 20, "wrong back");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Clear Tests
 * ============================================================================ */

int test_linkedlist_clear(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 100; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    ASSERT_EQUAL(linkedlist_clear(list), LC_OK, "clear failed");
    ASSERT_TRUE(linkedlist_is_empty(list), "not empty after clear");

    int val = 42;
    linkedlist_push_back(list, &val);
    ASSERT_EQUAL(linkedlist_len(list), 1, "cannot push after clear");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_clear_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "hello");
    linkedlist_push_back(list, "world");

    ASSERT_EQUAL(linkedlist_clear(list), LC_OK, "clear strings failed");
    ASSERT_TRUE(linkedlist_is_empty(list), "not empty after clear");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Reverse Tests
 * ============================================================================ */

int test_linkedlist_reverse_inplace(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    ASSERT_EQUAL(linkedlist_reverse_inplace(list), LC_OK, "reverse_inplace failed");
    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), (int)(9 - i), "wrong reverse order");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_reverse(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    LinkedList *rev = linkedlist_reverse(list);
    ASSERT_NOT_NULL(rev, "reverse failed");
    ASSERT_EQUAL(linkedlist_len(rev), 10, "wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(rev, i), (int)(9 - i), "wrong reverse order");
    }

    linkedlist_destroy(list);
    linkedlist_destroy(rev);
    return 1;
}

int test_linkedlist_reverse_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "a");
    linkedlist_push_back(list, "b");
    linkedlist_push_back(list, "c");

    LinkedList *rev = linkedlist_reverse(list);
    ASSERT_NOT_NULL(rev, "reverse strings failed");
    ASSERT_STR_EQUAL((char *)linkedlist_at(rev, 0), "c", "wrong first");
    ASSERT_STR_EQUAL((char *)linkedlist_at(rev, 2), "a", "wrong last");

    linkedlist_destroy(list);
    linkedlist_destroy(rev);
    return 1;
}

/* ============================================================================
 * Sort & Unique Tests
 * ============================================================================ */

int test_linkedlist_sort(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int vals[] = {50, 10, 40, 20, 30};
    for (int i = 0; i < 5; i++) linkedlist_push_back(list, &vals[i]);

    ASSERT_EQUAL(linkedlist_sort(list, cmp_int), LC_OK, "sort failed");
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), (int)(10 + i * 10), "wrong sort order");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_sort_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "zebra");
    linkedlist_push_back(list, "apple");
    linkedlist_push_back(list, "mango");
    linkedlist_push_back(list, "banana");

    ASSERT_EQUAL(linkedlist_sort(list, cmp_str_ptr), LC_OK, "sort strings failed");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 0), "apple", "wrong first");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 3), "zebra", "wrong last");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_unique(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int vals[] = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
    for (int i = 0; i < 10; i++) linkedlist_push_back(list, &vals[i]);

    ASSERT_EQUAL(linkedlist_unique(list), LC_OK, "unique failed");
    ASSERT_EQUAL(linkedlist_len(list), 5, "wrong length after unique");

    int expected[] = {1, 2, 3, 4, 5};
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), expected[i], "wrong unique value");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_unique_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "apple");
    linkedlist_push_back(list, "apple");
    linkedlist_push_back(list, "banana");
    linkedlist_push_back(list, "cherry");
    linkedlist_push_back(list, "cherry");

    ASSERT_EQUAL(linkedlist_unique(list), LC_OK, "unique strings failed");
    ASSERT_EQUAL(linkedlist_len(list), 3, "wrong length after unique");

    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 0), "apple", "wrong first");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 1), "banana", "wrong second");
    ASSERT_STR_EQUAL((char *)linkedlist_at(list, 2), "cherry", "wrong third");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Find & Search Tests
 * ============================================================================ */

int test_linkedlist_find(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    int needle = 5;
    ASSERT_EQUAL(linkedlist_find(list, &needle), 5, "find wrong position");

    needle = 99;
    ASSERT_EQUAL(linkedlist_find(list, &needle), LIST_NPOS, "find should return NPOS");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_rfind(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    int vals[] = {1, 2, 3, 2, 1, 2, 3};
    for (int i = 0; i < 7; i++) linkedlist_push_back(list, &vals[i]);

    int needle = 2;
    ASSERT_EQUAL(linkedlist_rfind(list, &needle), 5, "rfind wrong position");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_contains(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    int present = 5, absent = 99;
    ASSERT_TRUE(linkedlist_contains(list, &present), "contains false for existing");
    ASSERT_FALSE(linkedlist_contains(list, &absent), "contains true for missing");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_contains_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "hello");
    linkedlist_push_back(list, "world");

    ASSERT_TRUE(linkedlist_contains(list, "hello"), "contains false for existing");
    ASSERT_FALSE(linkedlist_contains(list, "missing"), "contains true for missing");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Clone & Sublist Tests
 * ============================================================================ */

int test_linkedlist_clone(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    LinkedList *clone = linkedlist_clone(list);
    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(linkedlist_len(clone), 10, "clone wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(clone, i), (int)i, "clone wrong value");
    }

    int newval = 999;
    linkedlist_set(clone, 0, &newval);
    ASSERT_EQUAL(*(int *)linkedlist_at(list, 0), 0, "original changed after clone modification");

    linkedlist_destroy(list);
    linkedlist_destroy(clone);
    return 1;
}

int test_linkedlist_clone_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "hello");
    linkedlist_push_back(list, "world");

    LinkedList *clone = linkedlist_clone(list);
    ASSERT_NOT_NULL(clone, "clone strings failed");
    ASSERT_STR_EQUAL((char *)linkedlist_at(clone, 0), "hello", "clone wrong string");

    linkedlist_destroy(list);
    linkedlist_destroy(clone);
    return 1;
}

int test_linkedlist_sublist(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    LinkedList *sub = linkedlist_sublist(list, 3, 7);
    ASSERT_NOT_NULL(sub, "sublist failed");
    ASSERT_EQUAL(linkedlist_len(sub), 4, "sublist wrong length");

    int expected[] = {3, 4, 5, 6};
    for (size_t i = 0; i < 4; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(sub, i), expected[i], "sublist wrong value");
    }

    linkedlist_destroy(list);
    linkedlist_destroy(sub);
    return 1;
}

int test_linkedlist_sublist_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "a");
    linkedlist_push_back(list, "b");
    linkedlist_push_back(list, "c");
    linkedlist_push_back(list, "d");

    LinkedList *sub = linkedlist_sublist(list, 1, 3);
    ASSERT_NOT_NULL(sub, "sublist strings failed");
    ASSERT_STR_EQUAL((char *)linkedlist_at(sub, 0), "b", "wrong first");
    ASSERT_STR_EQUAL((char *)linkedlist_at(sub, 1), "c", "wrong second");

    linkedlist_destroy(list);
    linkedlist_destroy(sub);
    return 1;
}

/* ============================================================================
 * Append Tests
 * ============================================================================ */

int test_linkedlist_append(void) {
    LinkedList *a = linkedlist_create(sizeof(int));
    LinkedList *b = linkedlist_create(sizeof(int));
    for (int i = 0; i < 3; i++) {
        int val = i;
        linkedlist_push_back(a, &val);
        linkedlist_push_back(b, &(int){i + 3});
    }

    ASSERT_EQUAL(linkedlist_append(a, b), LC_OK, "append failed");
    ASSERT_EQUAL(linkedlist_len(a), 6, "wrong length after append");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(a, i), i, "wrong appended value");
    }

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

int test_linkedlist_self_append(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 3; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    ASSERT_EQUAL(linkedlist_append(list, list), LC_OK, "self-append failed");
    ASSERT_EQUAL(linkedlist_len(list), 6, "wrong length after self-append");

    for (int i = 0; i < 6; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), i % 3, "wrong self-appended value");
    }

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_append_strings(void) {
    LinkedList *a = linkedlist_str();
    LinkedList *b = linkedlist_str();
    linkedlist_push_back(a, "hello");
    linkedlist_push_back(a, "world");
    linkedlist_push_back(b, "test");
    linkedlist_push_back(b, "append");

    ASSERT_EQUAL(linkedlist_append(a, b), LC_OK, "append strings failed");
    ASSERT_EQUAL(linkedlist_len(a), 4, "wrong length");
    ASSERT_STR_EQUAL((char *)linkedlist_at(a, 2), "test", "wrong appended string");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

/* ============================================================================
 * Splice Tests
 * ============================================================================ */

int test_linkedlist_splice_same_allocator(void) {
    LinkedList *dst = linkedlist_create(sizeof(int));
    LinkedList *src = linkedlist_create_from_impl(dst, true);

    for (int i = 0; i < 4; i++) {
        int val = i;
        linkedlist_push_back(dst, &val);
    }
    for (int i = 0; i < 3; i++) {
        int val = i + 10;
        linkedlist_push_back(src, &val);
    }

    ASSERT_EQUAL(linkedlist_splice(dst, 2, src, 0, 2), LC_OK, "splice failed");
    ASSERT_EQUAL(linkedlist_len(dst), 6, "wrong length after splice");

    linkedlist_destroy(dst);
    linkedlist_destroy(src);
    return 1;
}

/* ============================================================================
 * Hash Tests
 * ============================================================================ */

int test_linkedlist_hash(void) {
    LinkedList *a = linkedlist_create(sizeof(int));
    LinkedList *b = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(a, &val);
        linkedlist_push_back(b, &val);
    }

    size_t ha = linkedlist_hash(a);
    size_t hb = linkedlist_hash(b);
    ASSERT_EQUAL(ha, hb, "identical lists different hash");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

int test_linkedlist_hash_order_sensitive(void) {
    LinkedList *a = linkedlist_create(sizeof(int));
    LinkedList *b = linkedlist_create(sizeof(int));
    int x = 1, y = 2;
    linkedlist_push_back(a, &x);
    linkedlist_push_back(a, &y);
    linkedlist_push_back(b, &y);
    linkedlist_push_back(b, &x);

    ASSERT_NOT_EQUAL(linkedlist_hash(a), linkedlist_hash(b), "order should affect hash");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

/* ============================================================================
 * Equals Tests
 * ============================================================================ */

int test_linkedlist_equals_identical(void) {
    LinkedList *a = linkedlist_create(sizeof(int));
    LinkedList *b = linkedlist_create(sizeof(int));
    linkedlist_fill(a, 5);
    linkedlist_fill(b, 5);

    ASSERT_TRUE(linkedlist_equals(a, b), "identical lists should be equal");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

int test_linkedlist_equals_different_values(void) {
    LinkedList *a = linkedlist_create(sizeof(int));
    LinkedList *b = linkedlist_create(sizeof(int));
    linkedlist_fill(a, 5);
    linkedlist_fill(b, 4);

    int extra = 99;
    linkedlist_push_back(b, &extra);
    ASSERT_FALSE(linkedlist_equals(a, b), "different values should not be equal");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

int test_linkedlist_equals_strings_same_order(void) {
    LinkedList *a = linkedlist_str();
    LinkedList *b = linkedlist_str();
    linkedlist_push_back(a, "hello");
    linkedlist_push_back(a, "world");
    linkedlist_push_back(b, "hello");
    linkedlist_push_back(b, "world");

    ASSERT_TRUE(linkedlist_equals(a, b), "same order strings should be equal");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

int test_linkedlist_equals_strings_different_order(void) {
    LinkedList *a = linkedlist_str();
    LinkedList *b = linkedlist_str();
    linkedlist_push_back(a, "hello");
    linkedlist_push_back(a, "world");
    linkedlist_push_back(b, "world");
    linkedlist_push_back(b, "hello");

    ASSERT_FALSE(linkedlist_equals(a, b), "different order should NOT be equal");

    linkedlist_destroy(a);
    linkedlist_destroy(b);
    return 1;
}

/* ============================================================================
 * Iterator Tests
 * ============================================================================ */

int test_linkedlist_iterator(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    Iterator it = linkedlist_iter(list);
    int count = 0;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "iterator wrong value");
        count++;
    }
    ASSERT_EQUAL(count, 10, "iterator wrong count");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_iterator_reversed(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    Iterator it = linkedlist_iter_reversed(list);
    int count = 9;
    int *p;
    while ((p = (int *)iter_next(&it)) != NULL) {
        ASSERT_EQUAL(*p, count, "reverse iterator wrong value");
        count--;
    }
    ASSERT_EQUAL(count, -1, "reverse iterator wrong count");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_iterator_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "hello");
    linkedlist_push_back(list, "world");

    Iterator it = linkedlist_iter(list);
    char *p;
    p = (char *)iter_next(&it);
    ASSERT_STR_EQUAL(p, "hello", "iterator wrong string");
    p = (char *)iter_next(&it);
    ASSERT_STR_EQUAL(p, "world", "iterator wrong string");
    ASSERT_NULL(iter_next(&it), "iterator not exhausted");

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * To Array Tests
 * ============================================================================ */

int test_linkedlist_to_array(void) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < 10; i++) {
        int val = i;
        linkedlist_push_back(list, &val);
    }

    Array *arr = linkedlist_to_array(list);
    ASSERT_NOT_NULL(arr, "to_array failed");
    ASSERT_EQUAL(arr->len, 10, "array wrong length");

    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQUAL(*(int *)((uint8_t *)arr->items + i * arr->stride), (int)i, "array wrong value");
    }

    free(arr);
    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_to_array_strings(void) {
    LinkedList *list = linkedlist_str();
    linkedlist_push_back(list, "hello");
    linkedlist_push_back(list, "world");

    Array *arr = linkedlist_to_array(list);
    ASSERT_NOT_NULL(arr, "to_array strings failed");
    ASSERT_EQUAL(arr->len, 2, "array wrong length");

    char **table = (char **)arr->items;
    ASSERT_STR_EQUAL(table[0], "hello", "array wrong string");
    ASSERT_STR_EQUAL(table[1], "world", "array wrong string");

    free(arr);
    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Large Dataset Tests
 * ============================================================================ */

int test_linkedlist_large_push(void) {
    const size_t N = 100000;
    LinkedList *list = linkedlist_create(sizeof(int));
    ASSERT_NOT_NULL(list, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        ASSERT_EQUAL(linkedlist_push_back(list, &val), LC_OK, "push failed at %zu", i);
    }
    double push_time = timer_elapsed(&t);
    printf("    Push %zu elements: %.3f ms\n", N, push_time * 1000);

    ASSERT_EQUAL(linkedlist_len(list), N, "wrong length after pushes");

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(list, i), (int)i, "wrong value at %zu", i);
    }
    double access_time = timer_elapsed(&t);
    printf("    Access %zu elements: %.3f ms\n", N, access_time * 1000);

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_large_alternating_push_pop(void) {
    const size_t N = 100000;
    LinkedList *list = linkedlist_create(sizeof(int));
    ASSERT_NOT_NULL(list, "create failed");

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        linkedlist_push_back(list, &val);
        linkedlist_pop_front(list);
    }
    double time = timer_elapsed(&t);
    printf("    Alternating push/pop %zu times: %.3f ms\n", N, time * 1000);

    ASSERT_TRUE(linkedlist_is_empty(list), "list not empty after alternating ops");

    linkedlist_destroy(list);
    return 1;
}

int test_linkedlist_large_clone(void) {
    const size_t N = 100000;
    LinkedList *list = linkedlist_create(sizeof(int));
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        linkedlist_push_back(list, &val);
    }

    Timer t = timer_start();
    LinkedList *clone = linkedlist_clone(list);
    double clone_time = timer_elapsed(&t);
    printf("    Clone %zu elements: %.3f ms\n", N, clone_time * 1000);

    ASSERT_NOT_NULL(clone, "clone failed");
    ASSERT_EQUAL(linkedlist_len(clone), N, "clone wrong length");

    for (size_t i = 0; i < N; i++) {
        ASSERT_EQUAL(*(int *)linkedlist_at(clone, i), (int)i, "clone wrong value at %zu", i);
    }

    linkedlist_destroy(list);
    linkedlist_destroy(clone);
    return 1;
}

int test_linkedlist_large_strings(void) {
    const size_t N = 10000;
    LinkedList *list = linkedlist_str();
    char buf[32];

    Timer t = timer_start();
    for (size_t i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_EQUAL(linkedlist_push_back(list, buf), LC_OK, "push string failed at %zu", i);
    }
    double push_time = timer_elapsed(&t);
    printf("    Push %zu strings: %.3f ms\n", N, push_time * 1000);

    t = timer_start();
    for (size_t i = 0; i < N; i++) {
        char *s = (char *)linkedlist_at(list, i);
        snprintf(buf, sizeof(buf), "string_%zu", i);
        ASSERT_STR_EQUAL(s, buf, "wrong string at %zu", i);
    }
    double access_time = timer_elapsed(&t);
    printf("    Access %zu strings: %.3f ms\n", N, access_time * 1000);

    linkedlist_destroy(list);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  LinkedList Unit Tests\n");
    printf("========================================\n\n");

    Timer total = timer_start();

    /* Creation & Destruction */
    TEST(test_linkedlist_create);
    TEST(test_linkedlist_str_create);
    TEST(test_linkedlist_destroy_null);

    /* Alignment */
    TEST(test_linkedlist_alignment_32);
    TEST(test_linkedlist_alignment_64);

    /* Push */
    TEST(test_linkedlist_push_back);
    TEST(test_linkedlist_push_front);
    TEST(test_linkedlist_push_strings);

    /* Pop */
    TEST(test_linkedlist_pop_back);
    TEST(test_linkedlist_pop_front);
    TEST(test_linkedlist_pop_strings);

    /* Insert */
    TEST(test_linkedlist_insert_middle);
    TEST(test_linkedlist_insert_front);
    TEST(test_linkedlist_insert_back);
    TEST(test_linkedlist_insert_strings);

    /* Remove */
    TEST(test_linkedlist_remove);
    TEST(test_linkedlist_remove_range);
    TEST(test_linkedlist_remove_strings);

    /* Access */
    TEST(test_linkedlist_at);
    TEST(test_linkedlist_front_back);

    /* Clear */
    TEST(test_linkedlist_clear);
    TEST(test_linkedlist_clear_strings);

    /* Reverse */
    TEST(test_linkedlist_reverse_inplace);
    TEST(test_linkedlist_reverse);
    TEST(test_linkedlist_reverse_strings);

    /* Sort & Unique */
    TEST(test_linkedlist_sort);
    TEST(test_linkedlist_sort_strings);
    TEST(test_linkedlist_unique);
    TEST(test_linkedlist_unique_strings);

    /* Find & Search */
    TEST(test_linkedlist_find);
    TEST(test_linkedlist_rfind);
    TEST(test_linkedlist_contains);
    TEST(test_linkedlist_contains_strings);

    /* Clone & Sublist */
    TEST(test_linkedlist_clone);
    TEST(test_linkedlist_clone_strings);
    TEST(test_linkedlist_sublist);
    TEST(test_linkedlist_sublist_strings);

    /* Append */
    TEST(test_linkedlist_append);
    TEST(test_linkedlist_self_append);
    TEST(test_linkedlist_append_strings);

    /* Splice */
    TEST(test_linkedlist_splice_same_allocator);

    /* Hash */
    TEST(test_linkedlist_hash);
    TEST(test_linkedlist_hash_order_sensitive);

    /* Equals */
    TEST(test_linkedlist_equals_identical);
    TEST(test_linkedlist_equals_different_values);
    TEST(test_linkedlist_equals_strings_same_order);
    TEST(test_linkedlist_equals_strings_different_order);

    /* Iterator */
    TEST(test_linkedlist_iterator);
    TEST(test_linkedlist_iterator_reversed);
    TEST(test_linkedlist_iterator_strings);

    /* To Array */
    TEST(test_linkedlist_to_array);
    TEST(test_linkedlist_to_array_strings);

    /* Large Dataset Tests */
    printf("\n--- Large Dataset Tests ---\n");
    TEST(test_linkedlist_large_push);
    TEST(test_linkedlist_large_alternating_push_pop);
    TEST(test_linkedlist_large_clone);
    TEST(test_linkedlist_large_strings);

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