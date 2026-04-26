/**
 * @file typed_test.c
 * @brief Comprehensive test suite for type-safe container wrappers
 *
 * Tests all typed container variants (Vector, Deque, LinkedList, HashSet, HashMap)
 * with different data types (int, double, string, struct).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "assertion.h"
#include "timer.h"

#include <contain/typed.h>

/* Define all typed containers before including typed.h */
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_VECTOR_TYPE(double, sizeof(double), DoubleVector)
DECL_VECTOR_TYPE(const char*, 0, StringVector)

DECL_DEQUE_TYPE(int, sizeof(int), IntDeque)
DECL_DEQUE_TYPE(double, sizeof(double), DoubleDeque)
DECL_DEQUE_TYPE(const char*, 0, StringDeque)

DECL_LINKEDLIST_TYPE(int, sizeof(int), IntList)
DECL_LINKEDLIST_TYPE(double, sizeof(double), DoubleList)
DECL_LINKEDLIST_TYPE(const char*, 0, StringList)

DECL_HASHSET_TYPE(int, sizeof(int), IntSet)
DECL_HASHSET_TYPE(double, sizeof(double), DoubleSet)
DECL_HASHSET_TYPE(const char*, 0, StringSet)

DECL_HASHMAP_TYPE(int, int, sizeof(int), sizeof(int), IntIntMap)
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)
DECL_HASHMAP_TYPE(const char*, const char*, 0, 0, StrStrMap)
DECL_HASHMAP_TYPE(int, double, sizeof(int), sizeof(double), IntDoubleMap)

/* Struct tests - define custom struct and its typed containers */
typedef struct {
    int id;
    char name[32];
    double value;
} Record;

/* Custom hash function for Record */
static size_t record_hash(const void *key, size_t len) {
    (void)len;
    const Record *r = (const Record *)key;
    size_t h = 5381;
    h = ((h << 5) + h) ^ r->id;
    for (const char *p = r->name; *p; p++) {
        h = ((h << 5) + h) ^ *p;
    }
    return h;
}

/* Custom comparator for Record */
static int record_compare(const void *a, const void *b) {
    const Record *ra = (const Record *)a;
    const Record *rb = (const Record *)b;
    if (ra->id != rb->id) return ra->id - rb->id;
    return strcmp(ra->name, rb->name);
}

/* Declare typed containers for struct */
DECL_VECTOR_TYPE(Record, sizeof(Record), RecordVector)
DECL_DEQUE_TYPE(Record, sizeof(Record), RecordDeque)
DECL_LINKEDLIST_TYPE(Record, sizeof(Record), RecordList)
DECL_HASHSET_TYPE(Record, sizeof(Record), RecordSet)
DECL_HASHMAP_TYPE(int, Record, sizeof(int), sizeof(Record), IntRecordMap)


/* ============================================================================
 * Test Helpers
 * ============================================================================ */

static Record make_record(int id, const char *name, double value) {
    Record r;
    r.id = id;
    strncpy(r.name, name, sizeof(r.name) - 1);
    r.name[sizeof(r.name) - 1] = '\0';
    r.value = value;
    return r;
}

static bool records_equal(const Record *a, const Record *b) {
    return a->id == b->id && strcmp(a->name, b->name) == 0 && fabs(a->value - b->value) < 0.0001;
}

/* ============================================================================
 * Vector Tests
 * ============================================================================ */

static int test_vector_int_basic(void) {
    IntVector *vec = IntVector_create();
    ASSERT_NOT_NULL(vec, "IntVector_create");

    IntVector_push(vec, 10);
    IntVector_push(vec, 20);
    IntVector_push(vec, 30);
    ASSERT_EQUAL(IntVector_len(vec), 3, "push three elements");

    int val = IntVector_at(vec, 0);
    ASSERT_EQUAL(val, 10, "at index 0 returns 10");

    val = IntVector_back(vec);
    ASSERT_EQUAL(val, 30, "back returns 30");

    val = IntVector_front(vec);
    ASSERT_EQUAL(val, 10, "front returns 10");

    IntVector_set(vec, 1, 99);
    val = IntVector_at(vec, 1);
    ASSERT_EQUAL(val, 99, "set at index 1 to 99");

    IntVector_destroy(vec);
    return 1;
}

static int test_vector_int_pop(void) {
    IntVector *vec = IntVector_create();
    ASSERT_NOT_NULL(vec, "IntVector_create");

    IntVector_push(vec, 10);
    IntVector_push(vec, 20);
    IntVector_push(vec, 30);
    ASSERT_EQUAL(IntVector_len(vec), 3, "initial len 3");

    IntVector_pop(vec);
    ASSERT_EQUAL(IntVector_len(vec), 2, "len after pop is 2");
    ASSERT_EQUAL(IntVector_back(vec), 20, "back after pop is 20");

    IntVector_remove(vec, 0);
    ASSERT_EQUAL(IntVector_len(vec), 1, "len after remove is 1");
    ASSERT_EQUAL(IntVector_at(vec, 0), 20, "first element after remove is 20");

    IntVector_clear(vec);
    ASSERT_EQUAL(IntVector_len(vec), 0, "clear sets len to 0");
    ASSERT_TRUE(IntVector_is_empty(vec), "is_empty returns true");

    IntVector_destroy(vec);
    return 1;
}

static int test_vector_int_find(void) {
    IntVector *vec = IntVector_create();
    IntVector_push(vec, 10);
    IntVector_push(vec, 20);
    IntVector_push(vec, 30);
    IntVector_push(vec, 20);
    IntVector_push(vec, 40);

    size_t idx = IntVector_find(vec, 20);
    ASSERT_EQUAL(idx, 1, "find returns first occurrence");

    idx = IntVector_rfind(vec, 20);
    ASSERT_EQUAL(idx, 3, "rfind returns last occurrence");

    idx = IntVector_find(vec, 99);
    ASSERT_EQUAL(idx, VEC_NPOS, "find returns NPOS for missing");

    bool found = IntVector_contains(vec, 30);
    ASSERT_TRUE(found, "contains returns true for existing");

    found = IntVector_contains(vec, 100);
    ASSERT_FALSE(found, "contains returns false for missing");

    IntVector_destroy(vec);
    return 1;
}

static int test_vector_int_sort_reverse(void) {
    IntVector *vec = IntVector_create();
    IntVector_push(vec, 30);
    IntVector_push(vec, 10);
    IntVector_push(vec, 50);
    IntVector_push(vec, 20);
    IntVector_push(vec, 40);

    IntVector_sort(vec, lc_compare_int);
    ASSERT_EQUAL(IntVector_at(vec, 0), 10, "sort ascending index 0");
    ASSERT_EQUAL(IntVector_at(vec, 1), 20, "sort ascending index 1");
    ASSERT_EQUAL(IntVector_at(vec, 2), 30, "sort ascending index 2");
    ASSERT_EQUAL(IntVector_at(vec, 3), 40, "sort ascending index 3");
    ASSERT_EQUAL(IntVector_at(vec, 4), 50, "sort ascending index 4");

    IntVector_reverse_inplace(vec);
    ASSERT_EQUAL(IntVector_at(vec, 0), 50, "reverse index 0");
    ASSERT_EQUAL(IntVector_at(vec, 4), 10, "reverse index 4");

    IntVector_destroy(vec);
    return 1;
}

static int test_vector_string(void) {
    StringVector *vec = StringVector_create();
    ASSERT_NOT_NULL(vec, "StringVector_create");

    StringVector_push(vec, "hello");
    StringVector_push(vec, "world");
    StringVector_push(vec, "test");
    ASSERT_EQUAL(StringVector_len(vec), 3, "push three strings");

    const char *s = StringVector_at(vec, 0);
    ASSERT_STR_EQUAL(s, "hello", "at index 0 returns 'hello'");

    s = StringVector_back(vec);
    ASSERT_STR_EQUAL(s, "test", "back returns 'test'");

    StringVector_set(vec, 1, "modified");
    s = StringVector_at(vec, 1);
    ASSERT_STR_EQUAL(s, "modified", "set at index 1 to 'modified'");

    StringVector_insert(vec, 1, "inserted");
    s = StringVector_at(vec, 1);
    ASSERT_STR_EQUAL(s, "inserted", "insert at index 1");

    ASSERT_EQUAL(StringVector_len(vec), 4, "len after insert is 4");

    StringVector_remove(vec, 1);
    s = StringVector_at(vec, 1);
    ASSERT_STR_EQUAL(s, "modified", "after remove at index 1");

    StringVector_clear(vec);
    ASSERT_TRUE(StringVector_is_empty(vec), "clear empties vector");

    StringVector_destroy(vec);
    return 1;
}

static int test_vector_sort_strings(void) {
    StringVector *vec = StringVector_create();
    StringVector_push(vec, "zebra");
    StringVector_push(vec, "apple");
    StringVector_push(vec, "mango");
    StringVector_push(vec, "banana");

    StringVector_sort(vec, lc_compare_str);
    ASSERT_STR_EQUAL(StringVector_at(vec, 0), "apple", "sort ascending index 0");
    ASSERT_STR_EQUAL(StringVector_at(vec, 1), "banana", "sort ascending index 1");
    ASSERT_STR_EQUAL(StringVector_at(vec, 2), "mango", "sort ascending index 2");
    ASSERT_STR_EQUAL(StringVector_at(vec, 3), "zebra", "sort ascending index 3");

    StringVector_reverse_inplace(vec);
    ASSERT_STR_EQUAL(StringVector_at(vec, 0), "zebra", "reverse index 0");
    ASSERT_STR_EQUAL(StringVector_at(vec, 3), "apple", "reverse index 3");

    StringVector_destroy(vec);
    return 1;
}

static int test_vector_double(void) {
    DoubleVector *vec = DoubleVector_create();
    DoubleVector_push(vec, 3.14);
    DoubleVector_push(vec, 2.718);
    DoubleVector_push(vec, 1.414);

    double val = DoubleVector_at(vec, 0);
    ASSERT_TRUE(fabs(val - 3.14) < 0.0001, "double at index 0");

    val = DoubleVector_back(vec);
    ASSERT_TRUE(fabs(val - 1.414) < 0.0001, "double back");

    DoubleVector_set(vec, 1, 1.618);
    val = DoubleVector_at(vec, 1);
    ASSERT_TRUE(fabs(val - 1.618) < 0.0001, "double set");

    DoubleVector_clear(vec);
    ASSERT_TRUE(DoubleVector_is_empty(vec), "clear empties vector");

    DoubleVector_destroy(vec);
    return 1;
}

static int test_vector_struct(void) {
    RecordVector *vec = RecordVector_create();
    ASSERT_NOT_NULL(vec, "RecordVector_create");

    Record r1 = make_record(1, "Alice", 100.5);
    Record r2 = make_record(2, "Bob", 200.3);
    Record r3 = make_record(3, "Charlie", 300.1);

    RecordVector_push(vec, r1);
    RecordVector_push(vec, r2);
    RecordVector_push(vec, r3);
    ASSERT_EQUAL(RecordVector_len(vec), 3, "push three records");

    Record val = RecordVector_at(vec, 0);
    ASSERT_TRUE(records_equal(&val, &r1), "at index 0 returns correct record");

    val = RecordVector_back(vec);
    ASSERT_TRUE(records_equal(&val, &r3), "back returns correct record");

    Record r2_modified = make_record(2, "Robert", 250.0);
    RecordVector_set(vec, 1, r2_modified);
    val = RecordVector_at(vec, 1);
    ASSERT_TRUE(records_equal(&val, &r2_modified), "set modifies record");

    RecordVector_clear(vec);
    ASSERT_TRUE(RecordVector_is_empty(vec), "clear empties vector");

    RecordVector_destroy(vec);
    return 1;
}

static int test_vector_slice_clone(void) {
    IntVector *vec = IntVector_create();
    for (int i = 0; i < 10; i++) {
        IntVector_push(vec, i);
    }

    IntVector *slice = IntVector_slice(vec, 3, 7);
    ASSERT_EQUAL(IntVector_len(slice), 4, "slice length is 4");
    ASSERT_EQUAL(IntVector_at(slice, 0), 3, "slice index 0");
    ASSERT_EQUAL(IntVector_at(slice, 3), 6, "slice index 3");

    IntVector *clone = IntVector_clone(vec);
    ASSERT_EQUAL(IntVector_len(clone), 10, "clone length matches");
    ASSERT_EQUAL(IntVector_at(clone, 5), 5, "clone index 5 matches");

    IntVector_destroy(vec);
    IntVector_destroy(slice);
    IntVector_destroy(clone);
    return 1;
}

/* ============================================================================
 * Deque Tests 
 * ============================================================================ */

static int test_deque_int_basic(void) {
    IntDeque *deq = IntDeque_create();
    ASSERT_NOT_NULL(deq, "IntDeque_create");

    IntDeque_push_back(deq, 10);
    IntDeque_push_back(deq, 20);
    IntDeque_push_front(deq, 5);
    IntDeque_push_front(deq, 1);
    ASSERT_EQUAL(IntDeque_len(deq), 4, "push four elements");

    int val = IntDeque_front(deq);
    ASSERT_EQUAL(val, 1, "front returns 1");

    val = IntDeque_back(deq);
    ASSERT_EQUAL(val, 20, "back returns 20");

    val = IntDeque_at(deq, 1);
    ASSERT_EQUAL(val, 5, "at index 1 returns 5");

    IntDeque_set(deq, 2, 99);
    val = IntDeque_at(deq, 2);
    ASSERT_EQUAL(val, 99, "set at index 2 to 99");

    IntDeque_destroy(deq);
    return 1;
}

static int test_deque_int_pop_insert_remove(void) {
    IntDeque *deq = IntDeque_create();
    IntDeque_push_back(deq, 10);
    IntDeque_push_back(deq, 20);
    IntDeque_push_back(deq, 30);
    ASSERT_EQUAL(IntDeque_len(deq), 3, "initial len 3");

    IntDeque_pop_back(deq);
    ASSERT_EQUAL(IntDeque_len(deq), 2, "len after pop back is 2");
    ASSERT_EQUAL(IntDeque_back(deq), 20, "back after pop back is 20");

    IntDeque_pop_front(deq);
    ASSERT_EQUAL(IntDeque_len(deq), 1, "len after pop front is 1");
    ASSERT_EQUAL(IntDeque_front(deq), 20, "front after pop front is 20");

    IntDeque_insert(deq, 1, 15);
    ASSERT_EQUAL(IntDeque_len(deq), 2, "len after insert is 2");
    ASSERT_EQUAL(IntDeque_at(deq, 1), 15, "insert at index 1");

    IntDeque_remove(deq, 1);
    ASSERT_EQUAL(IntDeque_len(deq), 1, "len after remove is 1");
    ASSERT_EQUAL(IntDeque_at(deq, 0), 20, "after remove at index 1");

    IntDeque_clear(deq);
    ASSERT_EQUAL(IntDeque_len(deq), 0, "clear sets len to 0");
    ASSERT_TRUE(IntDeque_is_empty(deq), "is_empty returns true");

    IntDeque_destroy(deq);
    return 1;
}

static int test_deque_int_find(void) {
    IntDeque *deq = IntDeque_create();
    IntDeque_push_back(deq, 10);
    IntDeque_push_back(deq, 20);
    IntDeque_push_back(deq, 30);
    IntDeque_push_back(deq, 20);
    IntDeque_push_back(deq, 40);

    size_t idx = IntDeque_find(deq, 20);
    ASSERT_EQUAL(idx, 1, "find returns first occurrence");

    idx = IntDeque_find(deq, 99);
    ASSERT_EQUAL(idx, DEQ_NPOS, "find returns NPOS for missing");

    bool found = IntDeque_contains(deq, 30);
    ASSERT_TRUE(found, "contains returns true for existing");

    found = IntDeque_contains(deq, 100);
    ASSERT_FALSE(found, "contains returns false for missing");

    IntDeque_destroy(deq);
    return 1;
}

static int test_deque_int_sort_reverse(void) {
    IntDeque *deq = IntDeque_create();
    IntDeque_push_back(deq, 30);
    IntDeque_push_back(deq, 10);
    IntDeque_push_back(deq, 50);
    IntDeque_push_back(deq, 20);
    IntDeque_push_back(deq, 40);

    IntDeque_sort(deq, lc_compare_int);
    ASSERT_EQUAL(IntDeque_at(deq, 0), 10, "sort ascending index 0");
    ASSERT_EQUAL(IntDeque_at(deq, 1), 20, "sort ascending index 1");
    ASSERT_EQUAL(IntDeque_at(deq, 2), 30, "sort ascending index 2");
    ASSERT_EQUAL(IntDeque_at(deq, 3), 40, "sort ascending index 3");
    ASSERT_EQUAL(IntDeque_at(deq, 4), 50, "sort ascending index 4");

    IntDeque_reverse_inplace(deq);
    ASSERT_EQUAL(IntDeque_at(deq, 0), 50, "reverse index 0");
    ASSERT_EQUAL(IntDeque_at(deq, 4), 10, "reverse index 4");

    IntDeque_destroy(deq);
    return 1;
}

static int test_deque_string(void) {
    StringDeque *deq = StringDeque_create();
    StringDeque_push_back(deq, "world");
    StringDeque_push_front(deq, "hello");
    StringDeque_push_back(deq, "!");

    const char *s = StringDeque_front(deq);
    ASSERT_STR_EQUAL(s, "hello", "front returns 'hello'");

    s = StringDeque_back(deq);
    ASSERT_STR_EQUAL(s, "!", "back returns '!'");

    s = StringDeque_at(deq, 1);
    ASSERT_STR_EQUAL(s, "world", "at index 1 returns 'world'");

    StringDeque_set(deq, 1, "WORLD");
    s = StringDeque_at(deq, 1);
    ASSERT_STR_EQUAL(s, "WORLD", "set at index 1");

    StringDeque_insert(deq, 1, "MID");
    s = StringDeque_at(deq, 1);
    ASSERT_STR_EQUAL(s, "MID", "insert at index 1");

    StringDeque_pop_front(deq);
    s = StringDeque_front(deq);
    ASSERT_STR_EQUAL(s, "MID", "pop front");

    StringDeque_pop_back(deq);
    s = StringDeque_back(deq);
    ASSERT_STR_EQUAL(s, "WORLD", "pop back");

    StringDeque_clear(deq);
    ASSERT_TRUE(StringDeque_is_empty(deq), "clear empties deque");

    StringDeque_destroy(deq);
    return 1;
}

static int test_deque_sort_strings(void) {
    StringDeque *deq = StringDeque_create();
    StringDeque_push_back(deq, "zebra");
    StringDeque_push_back(deq, "apple");
    StringDeque_push_back(deq, "mango");
    StringDeque_push_back(deq, "banana");

    StringDeque_sort(deq, lc_compare_str);
    ASSERT_STR_EQUAL(StringDeque_at(deq, 0), "apple", "sort ascending index 0");
    ASSERT_STR_EQUAL(StringDeque_at(deq, 1), "banana", "sort ascending index 1");
    ASSERT_STR_EQUAL(StringDeque_at(deq, 2), "mango", "sort ascending index 2");
    ASSERT_STR_EQUAL(StringDeque_at(deq, 3), "zebra", "sort ascending index 3");

    StringDeque_reverse_inplace(deq);
    ASSERT_STR_EQUAL(StringDeque_at(deq, 0), "zebra", "reverse index 0");
    ASSERT_STR_EQUAL(StringDeque_at(deq, 3), "apple", "reverse index 3");

    StringDeque_destroy(deq);
    return 1;
}

static int test_deque_double(void) {
    DoubleDeque *deq = DoubleDeque_create();
    DoubleDeque_push_back(deq, 3.14);
    DoubleDeque_push_back(deq, 2.718);
    DoubleDeque_push_front(deq, 1.414);

    double val = DoubleDeque_front(deq);
    ASSERT_TRUE(fabs(val - 1.414) < 0.0001, "front returns 1.414");

    val = DoubleDeque_back(deq);
    ASSERT_TRUE(fabs(val - 2.718) < 0.0001, "back returns 2.718");

    DoubleDeque_set(deq, 1, 1.618);
    val = DoubleDeque_at(deq, 1);
    ASSERT_TRUE(fabs(val - 1.618) < 0.0001, "set at index 1");

    DoubleDeque_destroy(deq);
    return 1;
}

static int test_deque_struct(void) {
    RecordDeque *deq = RecordDeque_create();
    ASSERT_NOT_NULL(deq, "RecordDeque_create");

    Record r1 = make_record(1, "Alice", 100.5);
    Record r2 = make_record(2, "Bob", 200.3);
    Record r3 = make_record(3, "Charlie", 300.1);

    RecordDeque_push_back(deq, r2);
    RecordDeque_push_front(deq, r1);
    RecordDeque_push_back(deq, r3);

    Record val = RecordDeque_front(deq);
    ASSERT_TRUE(records_equal(&val, &r1), "front returns correct record");

    val = RecordDeque_back(deq);
    ASSERT_TRUE(records_equal(&val, &r3), "back returns correct record");

    Record r2_modified = make_record(2, "Robert", 250.0);
    RecordDeque_set(deq, 1, r2_modified);
    val = RecordDeque_at(deq, 1);
    ASSERT_TRUE(records_equal(&val, &r2_modified), "set modifies record");

    RecordDeque_clear(deq);
    ASSERT_TRUE(RecordDeque_is_empty(deq), "clear empties deque");

    RecordDeque_destroy(deq);
    return 1;
}

static int test_deque_slice_clone(void) {
    IntDeque *deq = IntDeque_create();
    for (int i = 0; i < 10; i++) {
        IntDeque_push_back(deq, i);
    }

    IntDeque *slice = IntDeque_slice(deq, 3, 7);
    ASSERT_EQUAL(IntDeque_len(slice), 4, "slice length is 4");
    ASSERT_EQUAL(IntDeque_at(slice, 0), 3, "slice index 0");
    ASSERT_EQUAL(IntDeque_at(slice, 3), 6, "slice index 3");

    IntDeque *clone = IntDeque_clone(deq);
    ASSERT_EQUAL(IntDeque_len(clone), 10, "clone length matches");
    ASSERT_EQUAL(IntDeque_at(clone, 5), 5, "clone index 5 matches");

    IntDeque_destroy(deq);
    IntDeque_destroy(slice);
    IntDeque_destroy(clone);
    return 1;
}

/* ============================================================================
 * LinkedList Tests 
 * ============================================================================ */

static int test_list_int_basic(void) {
    IntList *list = IntList_create();
    ASSERT_NOT_NULL(list, "IntList_create");

    IntList_push_back(list, 10);
    IntList_push_back(list, 20);
    IntList_push_front(list, 5);
    IntList_push_front(list, 1);
    ASSERT_EQUAL(IntList_len(list), 4, "push four elements");

    int val = IntList_front(list);
    ASSERT_EQUAL(val, 1, "front returns 1");

    val = IntList_back(list);
    ASSERT_EQUAL(val, 20, "back returns 20");

    val = IntList_at(list, 1);
    ASSERT_EQUAL(val, 5, "at index 1 returns 5");

    IntList_set(list, 2, 99);
    val = IntList_at(list, 2);
    ASSERT_EQUAL(val, 99, "set at index 2 to 99");

    IntList_destroy(list);
    return 1;
}

static int test_list_int_pop_insert_remove(void) {
    IntList *list = IntList_create();
    IntList_push_back(list, 10);
    IntList_push_back(list, 20);
    IntList_push_back(list, 30);
    ASSERT_EQUAL(IntList_len(list), 3, "initial len 3");

    IntList_pop_back(list);
    ASSERT_EQUAL(IntList_len(list), 2, "len after pop back is 2");
    ASSERT_EQUAL(IntList_back(list), 20, "back after pop back is 20");

    IntList_pop_front(list);
    ASSERT_EQUAL(IntList_len(list), 1, "len after pop front is 1");
    ASSERT_EQUAL(IntList_front(list), 20, "front after pop front is 20");

    IntList_insert(list, 1, 15);
    ASSERT_EQUAL(IntList_len(list), 2, "len after insert is 2");
    ASSERT_EQUAL(IntList_at(list, 1), 15, "insert at index 1");

    IntList_remove(list, 1);
    ASSERT_EQUAL(IntList_len(list), 1, "len after remove is 1");
    ASSERT_EQUAL(IntList_at(list, 0), 20, "after remove at index 1");

    IntList_clear(list);
    ASSERT_EQUAL(IntList_len(list), 0, "clear sets len to 0");
    ASSERT_TRUE(IntList_is_empty(list), "is_empty returns true");

    IntList_destroy(list);
    return 1;
}

static int test_list_int_find(void) {
    IntList *list = IntList_create();
    IntList_push_back(list, 10);
    IntList_push_back(list, 20);
    IntList_push_back(list, 30);
    IntList_push_back(list, 20);
    IntList_push_back(list, 40);

    size_t idx = IntList_find(list, 20);
    ASSERT_EQUAL(idx, 1, "find returns first occurrence");

    idx = IntList_find(list, 99);
    ASSERT_EQUAL(idx, LIST_NPOS, "find returns NPOS for missing");

    bool found = IntList_contains(list, 30);
    ASSERT_TRUE(found, "contains returns true for existing");

    found = IntList_contains(list, 100);
    ASSERT_FALSE(found, "contains returns false for missing");

    IntList_destroy(list);
    return 1;
}

static int test_list_int_sort_reverse(void) {
    IntList *list = IntList_create();
    IntList_push_back(list, 30);
    IntList_push_back(list, 10);
    IntList_push_back(list, 50);
    IntList_push_back(list, 20);
    IntList_push_back(list, 40);

    IntList_sort(list, lc_compare_int);
    ASSERT_EQUAL(IntList_at(list, 0), 10, "sort ascending index 0");
    ASSERT_EQUAL(IntList_at(list, 1), 20, "sort ascending index 1");
    ASSERT_EQUAL(IntList_at(list, 2), 30, "sort ascending index 2");
    ASSERT_EQUAL(IntList_at(list, 3), 40, "sort ascending index 3");
    ASSERT_EQUAL(IntList_at(list, 4), 50, "sort ascending index 4");

    IntList_reverse_inplace(list);
    ASSERT_EQUAL(IntList_at(list, 0), 50, "reverse index 0");
    ASSERT_EQUAL(IntList_at(list, 4), 10, "reverse index 4");

    IntList_destroy(list);
    return 1;
}

static int test_list_string(void) {
    StringList *list = StringList_create();
    StringList_push_back(list, "world");
    StringList_push_front(list, "hello");
    StringList_push_back(list, "!");

    const char *s = StringList_front(list);
    ASSERT_STR_EQUAL(s, "hello", "front returns 'hello'");

    s = StringList_back(list);
    ASSERT_STR_EQUAL(s, "!", "back returns '!'");

    s = StringList_at(list, 1);
    ASSERT_STR_EQUAL(s, "world", "at index 1 returns 'world'");

    StringList_set(list, 1, "WORLD");
    s = StringList_at(list, 1);
    ASSERT_STR_EQUAL(s, "WORLD", "set at index 1");

    StringList_insert(list, 1, "MID");
    s = StringList_at(list, 1);
    ASSERT_STR_EQUAL(s, "MID", "insert at index 1");

    StringList_pop_front(list);
    s = StringList_front(list);
    ASSERT_STR_EQUAL(s, "MID", "pop front");

    StringList_pop_back(list);
    s = StringList_back(list);
    ASSERT_STR_EQUAL(s, "WORLD", "pop back");

    StringList_clear(list);
    ASSERT_TRUE(StringList_is_empty(list), "clear empties list");

    StringList_destroy(list);
    return 1;
}

static int test_list_sort_strings(void) {
    StringList *list = StringList_create();
    StringList_push_back(list, "zebra");
    StringList_push_back(list, "apple");
    StringList_push_back(list, "mango");
    StringList_push_back(list, "banana");

    StringList_sort(list, lc_compare_str);
    ASSERT_STR_EQUAL(StringList_at(list, 0), "apple", "sort ascending index 0");
    ASSERT_STR_EQUAL(StringList_at(list, 1), "banana", "sort ascending index 1");
    ASSERT_STR_EQUAL(StringList_at(list, 2), "mango", "sort ascending index 2");
    ASSERT_STR_EQUAL(StringList_at(list, 3), "zebra", "sort ascending index 3");

    StringList_reverse_inplace(list);
    ASSERT_STR_EQUAL(StringList_at(list, 0), "zebra", "reverse index 0");
    ASSERT_STR_EQUAL(StringList_at(list, 3), "apple", "reverse index 3");

    StringList_destroy(list);
    return 1;
}

static int test_list_double(void) {
    DoubleList *list = DoubleList_create();
    DoubleList_push_back(list, 3.14);
    DoubleList_push_back(list, 2.718);
    DoubleList_push_front(list, 1.414);

    double val = DoubleList_front(list);
    ASSERT_TRUE(fabs(val - 1.414) < 0.0001, "front returns 1.414");

    val = DoubleList_back(list);
    ASSERT_TRUE(fabs(val - 2.718) < 0.0001, "back returns 2.718");

    DoubleList_set(list, 1, 1.618);
    val = DoubleList_at(list, 1);
    ASSERT_TRUE(fabs(val - 1.618) < 0.0001, "set at index 1");

    DoubleList_destroy(list);
    return 1;
}

static int test_list_struct(void) {
    RecordList *list = RecordList_create();
    ASSERT_NOT_NULL(list, "RecordList_create");

    Record r1 = make_record(1, "Alice", 100.5);
    Record r2 = make_record(2, "Bob", 200.3);
    Record r3 = make_record(3, "Charlie", 300.1);

    RecordList_push_back(list, r2);
    RecordList_push_front(list, r1);
    RecordList_push_back(list, r3);

    Record val = RecordList_front(list);
    ASSERT_TRUE(records_equal(&val, &r1), "front returns correct record");

    val = RecordList_back(list);
    ASSERT_TRUE(records_equal(&val, &r3), "back returns correct record");

    Record r2_modified = make_record(2, "Robert", 250.0);
    RecordList_set(list, 1, r2_modified);
    val = RecordList_at(list, 1);
    ASSERT_TRUE(records_equal(&val, &r2_modified), "set modifies record");

    RecordList_clear(list);
    ASSERT_TRUE(RecordList_is_empty(list), "clear empties list");

    RecordList_destroy(list);
    return 1;
}

static int test_list_clone(void) {
    IntList *list = IntList_create();
    for (int i = 0; i < 10; i++) {
        IntList_push_back(list, i);
    }

    IntList *clone = IntList_clone(list);
    ASSERT_EQUAL(IntList_len(clone), 10, "clone length matches");
    ASSERT_EQUAL(IntList_at(clone, 5), 5, "clone index 5 matches");

    IntList_destroy(list);
    IntList_destroy(clone);
    return 1;
}

/* ============================================================================
 * HashSet Tests 
 * ============================================================================ */

static int test_set_int_basic(void) {
    IntSet *set = IntSet_create();
    ASSERT_NOT_NULL(set, "IntSet_create");

    IntSet_insert(set, 10);
    IntSet_insert(set, 20);
    IntSet_insert(set, 10);
    ASSERT_EQUAL(IntSet_len(set), 2, "insert ignores duplicates");

    bool found = IntSet_contains(set, 20);
    ASSERT_TRUE(found, "contains returns true for existing");

    found = IntSet_contains(set, 99);
    ASSERT_FALSE(found, "contains returns false for missing");

    IntSet_remove(set, 10);
    found = IntSet_contains(set, 10);
    ASSERT_FALSE(found, "remove deletes element");

    IntSet_destroy(set);
    return 1;
}

static int test_set_int_operations(void) {
    IntSet *set = IntSet_create();
    IntSet_insert(set, 10);
    IntSet_insert(set, 20);
    IntSet_insert(set, 30);

    IntSet_clear(set);
    ASSERT_TRUE(IntSet_is_empty(set), "clear empties set");

    IntSet_insert(set, 40);
    IntSet_insert(set, 50);
    ASSERT_EQUAL(IntSet_len(set), 2, "insert after clear");

    IntSet_destroy(set);
    return 1;
}

static int test_set_int_reserve_shrink(void) {
    IntSet *set = IntSet_create();
    for (int i = 0; i < 100; i++) {
        IntSet_insert(set, i);
    }

    size_t cap_before = IntSet_capacity(set);
    IntSet_reserve(set, 200);
    ASSERT_GREATER(IntSet_capacity(set), cap_before, "reserve increased capacity");

    IntSet_shrink_to_fit(set);
    ASSERT_LESS(IntSet_capacity(set), cap_before * 2, "shrink reduced capacity");

    IntSet_destroy(set);
    return 1;
}

static int test_set_set_operations(void) {
    IntSet *a = IntSet_create();
    IntSet *b = IntSet_create();

    for (int i = 1; i <= 5; i++) IntSet_insert(a, i);
    for (int i = 3; i <= 7; i++) IntSet_insert(b, i);

    IntSet *union_set = IntSet_union(a, b);
    ASSERT_EQUAL(IntSet_len(union_set), 7, "union size 7");

    IntSet *intersection = IntSet_intersection(a, b);
    ASSERT_EQUAL(IntSet_len(intersection), 3, "intersection size 3");

    IntSet *diff = IntSet_difference(a, b);
    ASSERT_EQUAL(IntSet_len(diff), 2, "difference size 2");

    IntSet_merge(a, b);
    ASSERT_EQUAL(IntSet_len(a), 7, "merge into a");

    IntSet_destroy(a);
    IntSet_destroy(b);
    IntSet_destroy(union_set);
    IntSet_destroy(intersection);
    IntSet_destroy(diff);
    return 1;
}

static int test_set_string(void) {
    StringSet *set = StringSet_create();
    StringSet_insert(set, "apple");
    StringSet_insert(set, "banana");
    StringSet_insert(set, "apple");

    ASSERT_EQUAL(StringSet_len(set), 2, "insert ignores duplicates");

    bool found = StringSet_contains(set, "banana");
    ASSERT_TRUE(found, "contains 'banana'");

    StringSet_remove(set, "apple");
    found = StringSet_contains(set, "apple");
    ASSERT_FALSE(found, "remove deletes element");

    StringSet_clear(set);
    ASSERT_TRUE(StringSet_is_empty(set), "clear empties set");

    StringSet_destroy(set);
    return 1;
}

static int test_set_double(void) {
    DoubleSet *set = DoubleSet_create();
    DoubleSet_insert(set, 3.14);
    DoubleSet_insert(set, 2.718);
    DoubleSet_insert(set, 3.14);

    ASSERT_EQUAL(DoubleSet_len(set), 2, "insert ignores duplicates");

    bool found = DoubleSet_contains(set, 2.718);
    ASSERT_TRUE(found, "contains 2.718");

    DoubleSet_destroy(set);
    return 1;
}

static int test_set_struct(void) {
    RecordSet *set = RecordSet_create_with_hasher(record_hash);
    RecordSet_set_comparator(set, record_compare);

    Record r1 = make_record(1, "Alice", 100.5);
    Record r2 = make_record(2, "Bob", 200.3);
    Record r3 = make_record(1, "Alice", 100.5);

    RecordSet_insert(set, r1);
    RecordSet_insert(set, r2);
    RecordSet_insert(set, r3);
    ASSERT_EQUAL(RecordSet_len(set), 2, "insert ignores duplicates");

    bool found = RecordSet_contains(set, r1);
    ASSERT_TRUE(found, "contains returns true for existing");

    RecordSet_remove(set, r1);
    found = RecordSet_contains(set, r1);
    ASSERT_FALSE(found, "remove deletes element");

    RecordSet_destroy(set);
    return 1;
}

static int test_set_clone(void) {
    IntSet *set = IntSet_create();
    for (int i = 0; i < 100; i++) {
        IntSet_insert(set, i);
    }

    IntSet *clone = IntSet_clone(set);
    ASSERT_EQUAL(IntSet_len(clone), 100, "clone length matches");

    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(IntSet_contains(clone, i), "clone contains element");
    }

    IntSet_destroy(set);
    IntSet_destroy(clone);
    return 1;
}

/* ============================================================================
 * HashMap Tests
 * ============================================================================ */

static int test_map_int_int_basic(void) {
    IntIntMap *map = IntIntMap_create();
    ASSERT_NOT_NULL(map, "IntIntMap_create");

    IntIntMap_insert(map, 1, 100);
    IntIntMap_insert(map, 2, 200);
    IntIntMap_insert(map, 3, 300);
    ASSERT_EQUAL(IntIntMap_len(map), 3, "insert three pairs");

    int val = IntIntMap_get(map, 2);
    ASSERT_EQUAL(val, 200, "get returns 200 for key 2");

    bool found = IntIntMap_contains(map, 2);
    ASSERT_TRUE(found, "contains key 2");

    IntIntMap_insert(map, 2, 250);
    val = IntIntMap_get(map, 2);
    ASSERT_EQUAL(val, 250, "update changes value");

    IntIntMap_remove(map, 2);
    found = IntIntMap_contains(map, 2);
    ASSERT_FALSE(found, "remove deletes key");

    IntIntMap_destroy(map);
    return 1;
}

static int test_map_int_int_operations(void) {
    IntIntMap *map = IntIntMap_create();
    IntIntMap_insert(map, 1, 100);
    IntIntMap_insert(map, 2, 200);

    IntIntMap_clear(map);
    ASSERT_TRUE(IntIntMap_is_empty(map), "clear empties map");

    IntIntMap_insert(map, 3, 300);
    ASSERT_EQUAL(IntIntMap_len(map), 1, "insert after clear");

    IntIntMap_destroy(map);
    return 1;
}

static int test_map_int_int_remove_entry(void) {
    IntIntMap *map = IntIntMap_create();
    IntIntMap_insert(map, 1, 100);
    IntIntMap_insert(map, 2, 200);

    IntIntMap_remove_entry(map, 1, 100);
    ASSERT_FALSE(IntIntMap_contains(map, 1), "remove entry deletes key");

    IntIntMap_remove_entry(map, 2, 999);  // wrong value
    ASSERT_TRUE(IntIntMap_contains(map, 2), "remove entry with wrong value does nothing");

    IntIntMap_destroy(map);
    return 1;
}

static int test_map_str_int(void) {
    StrIntMap *map = StrIntMap_create();
    StrIntMap_insert(map, "apple", 100);
    StrIntMap_insert(map, "banana", 200);
    StrIntMap_insert(map, "apple", 150);

    int val = StrIntMap_get(map, "apple");
    ASSERT_EQUAL(val, 150, "get returns updated value");

    bool found = StrIntMap_contains(map, "banana");
    ASSERT_TRUE(found, "contains 'banana'");

    StrIntMap_remove(map, "banana");
    found = StrIntMap_contains(map, "banana");
    ASSERT_FALSE(found, "remove deletes key");

    StrIntMap_destroy(map);
    return 1;
}

static int test_map_str_str(void) {
    StrStrMap *map = StrStrMap_create();
    StrStrMap_insert(map, "hello", "world");
    StrStrMap_insert(map, "foo", "bar");
    StrStrMap_insert(map, "hello", "WORLD");

    const char *val = StrStrMap_get(map, "hello");
    ASSERT_STR_EQUAL(val, "WORLD", "get returns updated value");

    StrStrMap_remove(map, "foo");
    bool found = StrStrMap_contains(map, "foo");
    ASSERT_FALSE(found, "remove deletes key");

    StrStrMap_destroy(map);
    return 1;
}

static int test_map_int_double(void) {
    IntDoubleMap *map = IntDoubleMap_create();
    IntDoubleMap_insert(map, 1, 3.14);
    IntDoubleMap_insert(map, 2, 2.718);
    IntDoubleMap_insert(map, 1, 1.618);

    double val = IntDoubleMap_get(map, 1);
    ASSERT_TRUE(fabs(val - 1.618) < 0.0001, "get returns updated value");

    IntDoubleMap_destroy(map);
    return 1;
}

static int test_map_int_record(void) {
    IntRecordMap *map = IntRecordMap_create();
    Record r1 = make_record(100, "Alice", 100.5);
    Record r2 = make_record(200, "Bob", 200.3);
    Record r3 = make_record(100, "Alicia", 150.0);

    IntRecordMap_insert(map, 1, r1);
    IntRecordMap_insert(map, 2, r2);
    IntRecordMap_insert(map, 1, r3);

    Record val = IntRecordMap_get(map, 1);
    ASSERT_TRUE(records_equal(&val, &r3), "get returns updated record");

    IntRecordMap_remove(map, 2);
    bool found = IntRecordMap_contains(map, 2);
    ASSERT_FALSE(found, "remove deletes key");

    IntRecordMap_destroy(map);
    return 1;
}

static int test_map_clone(void) {
    IntIntMap *map = IntIntMap_create();
    for (int i = 0; i < 100; i++) {
        IntIntMap_insert(map, i, i * 100);
    }

    IntIntMap *clone = IntIntMap_clone(map);
    ASSERT_EQUAL(IntIntMap_len(clone), 100, "clone length matches");

    for (int i = 0; i < 100; i++) {
        ASSERT_EQUAL(IntIntMap_get(clone, i), i * 100, "clone value matches");
    }

    IntIntMap_destroy(map);
    IntIntMap_destroy(clone);
    return 1;
}

static int test_map_keys_values(void) {
    StrIntMap *map = StrIntMap_create();
    StrIntMap_insert(map, "apple", 100);
    StrIntMap_insert(map, "banana", 200);
    StrIntMap_insert(map, "cherry", 300);

    Array *keys = StrIntMap_keys(map);
    ASSERT_EQUAL(keys->len, 3, "keys array length 3");

    Array *values = StrIntMap_values(map);
    ASSERT_EQUAL(values->len, 3, "values array length 3");

    free(keys);
    free(values);
    StrIntMap_destroy(map);
    return 1;
}

/* ============================================================================
 * Iterator Tests
 * ============================================================================ */

static int test_vector_iterators(void) {
    IntVector *vec = IntVector_create();
    for (int i = 0; i < 10; i++) {
        IntVector_push(vec, i);
    }

    /* Forward iteration */
    Iterator it = IntVector_iter(vec);
    int expected = 0;
    const int *val;
    while ((val = iter_next(&it))) {
        ASSERT_EQUAL(*val, expected, "vector forward iteration");
        expected++;
    }
    ASSERT_EQUAL(expected, 10, "vector forward iteration count");

    /* Reverse iteration */
    it = IntVector_iter_reversed(vec);
    expected = 9;
    while ((val = iter_next(&it))) {
        ASSERT_EQUAL(*val, expected, "vector reverse iteration");
        expected--;
    }
    ASSERT_EQUAL(expected, -1, "vector reverse iteration count");

    IntVector_destroy(vec);
    return 1;
}

static int test_deque_iterators(void) {
    IntDeque *deq = IntDeque_create();
    for (int i = 0; i < 10; i++) {
        IntDeque_push_back(deq, i);
    }

    /* Forward iteration */
    Iterator it = IntDeque_iter(deq);
    int expected = 0;
    const int *val;
    while ((val = iter_next(&it))) {
        ASSERT_EQUAL(*val, expected, "deque forward iteration");
        expected++;
    }
    ASSERT_EQUAL(expected, 10, "deque forward iteration count");

    /* Reverse iteration */
    it = IntDeque_iter_reversed(deq);
    expected = 9;
    while ((val = iter_next(&it))) {
        ASSERT_EQUAL(*val, expected, "deque reverse iteration");
        expected--;
    }
    ASSERT_EQUAL(expected, -1, "deque reverse iteration count");

    IntDeque_destroy(deq);
    return 1;
}

static int test_list_iterators(void) {
    IntList *list = IntList_create();
    for (int i = 0; i < 10; i++) {
        IntList_push_back(list, i);
    }

    /* Forward iteration */
    Iterator it = IntList_iter(list);
    int expected = 0;
    const int *val;
    while ((val = iter_next(&it))) {
        ASSERT_EQUAL(*val, expected, "list forward iteration");
        expected++;
    }
    ASSERT_EQUAL(expected, 10, "list forward iteration count");

    /* Reverse iteration */
    it = IntList_iter_reversed(list);
    expected = 9;
    while ((val = iter_next(&it))) {
        ASSERT_EQUAL(*val, expected, "list reverse iteration");
        expected--;
    }
    ASSERT_EQUAL(expected, -1, "list reverse iteration count");

    IntList_destroy(list);
    return 1;
}

static int test_set_iterators(void) {
    IntSet *set = IntSet_create();
    for (int i = 0; i < 100; i++) {
        IntSet_insert(set, i);
    }

    /* Forward iteration (order unspecified, but should visit all elements) */
    Iterator it = IntSet_iter(set);
    int count = 0;
    const int *val;
    while ((val = iter_next(&it))) {
        count++;
        ASSERT_TRUE(*val >= 0 && *val < 100, "set iteration value in range");
    }
    ASSERT_EQUAL(count, 100, "set iteration count");

    IntSet_destroy(set);
    return 1;
}

static int test_map_iterators(void) {
    StrIntMap *map = StrIntMap_create();
    StrIntMap_insert(map, "apple", 100);
    StrIntMap_insert(map, "banana", 200);
    StrIntMap_insert(map, "cherry", 300);

    Iterator it = StrIntMap_iter(map);
    int count = 0;
    int found_100 = 0, found_200 = 0, found_300 = 0;
    const void *entry;
    while ((entry = iter_next(&it))) {
        const char *key = StrIntMap_entry_key(map, entry);
        int val = StrIntMap_entry_value(map, entry);
        count++;
        ASSERT_NOT_NULL(key, "map entry key not null");
        
        if (val == 100) found_100 = 1;
        else if (val == 200) found_200 = 1;
        else if (val == 300) found_300 = 1;
    }
    ASSERT_EQUAL(count, 3, "map iteration count");
    ASSERT_TRUE(found_100 && found_200 && found_300, "all values found");

    StrIntMap_destroy(map);
    return 1;
}

/* ============================================================================
 * String Iterator Tests
 * ============================================================================ */

static int test_vector_string_iterators(void) {
    StringVector *vec = StringVector_create();
    StringVector_push(vec, "hello");
    StringVector_push(vec, "world");
    StringVector_push(vec, "test");

    /* Forward iteration */
    Iterator it = StringVector_iter(vec);
    const char *s;
    int count = 0;
    while ((s = iter_next(&it))) {
        if (count == 0) ASSERT_STR_EQUAL(s, "hello", "string forward iteration 0");
        if (count == 1) ASSERT_STR_EQUAL(s, "world", "string forward iteration 1");
        if (count == 2) ASSERT_STR_EQUAL(s, "test", "string forward iteration 2");
        count++;
    }
    ASSERT_EQUAL(count, 3, "string forward iteration count");

    /* Reverse iteration */
    it = StringVector_iter_reversed(vec);
    count = 0;
    while ((s = iter_next(&it))) {
        if (count == 0) ASSERT_STR_EQUAL(s, "test", "string reverse iteration 0");
        if (count == 1) ASSERT_STR_EQUAL(s, "world", "string reverse iteration 1");
        if (count == 2) ASSERT_STR_EQUAL(s, "hello", "string reverse iteration 2");
        count++;
    }
    ASSERT_EQUAL(count, 3, "string reverse iteration count");

    StringVector_destroy(vec);
    return 1;
}

static int test_deque_string_iterators(void) {
    StringDeque *deq = StringDeque_create();
    StringDeque_push_back(deq, "hello");
    StringDeque_push_back(deq, "world");
    StringDeque_push_back(deq, "test");

    /* Forward iteration */
    Iterator it = StringDeque_iter(deq);
    const char *s;
    int count = 0;
    while ((s = iter_next(&it))) {
        if (count == 0) ASSERT_STR_EQUAL(s, "hello", "deque string forward 0");
        if (count == 1) ASSERT_STR_EQUAL(s, "world", "deque string forward 1");
        if (count == 2) ASSERT_STR_EQUAL(s, "test", "deque string forward 2");
        count++;
    }
    ASSERT_EQUAL(count, 3, "deque string forward count");

    /* Reverse iteration */
    it = StringDeque_iter_reversed(deq);
    count = 0;
    while ((s = iter_next(&it))) {
        if (count == 0) ASSERT_STR_EQUAL(s, "test", "deque string reverse 0");
        if (count == 1) ASSERT_STR_EQUAL(s, "world", "deque string reverse 1");
        if (count == 2) ASSERT_STR_EQUAL(s, "hello", "deque string reverse 2");
        count++;
    }
    ASSERT_EQUAL(count, 3, "deque string reverse count");

    StringDeque_destroy(deq);
    return 1;
}

static int test_list_string_iterators(void) {
    StringList *list = StringList_create();
    StringList_push_back(list, "hello");
    StringList_push_back(list, "world");
    StringList_push_back(list, "test");

    /* Forward iteration */
    Iterator it = StringList_iter(list);
    const char *s;
    int count = 0;
    while ((s = iter_next(&it))) {
        if (count == 0) ASSERT_STR_EQUAL(s, "hello", "list string forward 0");
        if (count == 1) ASSERT_STR_EQUAL(s, "world", "list string forward 1");
        if (count == 2) ASSERT_STR_EQUAL(s, "test", "list string forward 2");
        count++;
    }
    ASSERT_EQUAL(count, 3, "list string forward count");

    /* Reverse iteration */
    it = StringList_iter_reversed(list);
    count = 0;
    while ((s = iter_next(&it))) {
        if (count == 0) ASSERT_STR_EQUAL(s, "test", "list string reverse 0");
        if (count == 1) ASSERT_STR_EQUAL(s, "world", "list string reverse 1");
        if (count == 2) ASSERT_STR_EQUAL(s, "hello", "list string reverse 2");
        count++;
    }
    ASSERT_EQUAL(count, 3, "list string reverse count");

    StringList_destroy(list);
    return 1;
}

static int test_set_string_iterators(void) {
    StringSet *set = StringSet_create();
    StringSet_insert(set, "apple");
    StringSet_insert(set, "banana");
    StringSet_insert(set, "cherry");

    /* Forward iteration (order unspecified, but should visit all elements) */
    Iterator it = StringSet_iter(set);
    int count = 0;
    const char *s;
    while ((s = iter_next(&it))) {
        count++;
        ASSERT_NOT_NULL(s, "string set iteration value not null");
    }
    ASSERT_EQUAL(count, 3, "string set iteration count");

    StringSet_destroy(set);
    return 1;
}

static int test_map_string_iterators(void) {
    StrStrMap *map = StrStrMap_create();
    StrStrMap_insert(map, "hello", "world");
    StrStrMap_insert(map, "foo", "bar");
    StrStrMap_insert(map, "c", "programming");

    /* Forward iteration */
    Iterator it = StrStrMap_iter(map);
    int count = 0;
    const void *entry;
    while ((entry = iter_next(&it))) {
        const char *key = StrStrMap_entry_key(map, entry);
        const char *val = StrStrMap_entry_value(map, entry);
        count++;
        ASSERT_NOT_NULL(key, "map string entry key not null");
        ASSERT_NOT_NULL(val, "map string entry value not null");
    }
    ASSERT_EQUAL(count, 3, "map string iteration count");

    StrStrMap_destroy(map);
    return 1;
}

/* ============================================================================
 * Cast Tests
 * ============================================================================ */

static int test_vector_wrap_unwrap(void) {
    /* 1. Create a generic vector and put some data in it */
    Vector *raw_vec = vector_create(sizeof(int));
    int a = 100, b = 200;
    vector_push(raw_vec, &a);
    vector_push(raw_vec, &b);

    /* 2. Wrap it into the typed interface */
    IntVector *typed_vec = IntVector_wrap((Container *)raw_vec);
    ASSERT_NOT_NULL(typed_vec, "IntVector_wrap");
    ASSERT_EQUAL(IntVector_len(typed_vec), 2, "Wrapped vector length");
    ASSERT_EQUAL(IntVector_at(typed_vec, 0), 100, "Wrapped element 0");

    /* 3. Unwrap it to get the raw pointer back */
    Vector *unwrapped = IntVector_unwrap(typed_vec);
    ASSERT_PTR_EQUAL((void*)unwrapped, (void*)raw_vec, "Unwrapped pointer matches original");
    
    int c = 300;
    vector_push(unwrapped, &c);
    ASSERT_EQUAL(IntVector_len(typed_vec), 3, "Typed vector reflects changes to unwrapped raw vector");

    /* 4. Destroying the typed wrapper should destroy the underlying vector */
    IntVector_destroy(typed_vec);
    return 1;
}

static int test_generic_to_typed_cast(void) {
    /* 1. Create a generic vector and put some data in it */
    Vector *raw_vec = vector_create(sizeof(int));
    int a = 100, b = 200;
    vector_push(raw_vec, &a);
    vector_push(raw_vec, &b);
    
    /* 2. Cast it to a typed vector */
    IntVector *typed_vec = (IntVector *)raw_vec;
    ASSERT_NOT_NULL(typed_vec, "IntVector cast");
    ASSERT_EQUAL(IntVector_len(typed_vec), 2, "Cast vector length");
    ASSERT_EQUAL(IntVector_at(typed_vec, 0), 100, "Cast element 0");
    
    /* 3. Cast it back to a generic vector */
    Vector *unwrapped = (Vector *)typed_vec;
    ASSERT_PTR_EQUAL((void*)unwrapped, (void*)raw_vec, "Cast back pointer matches original");
    
    int c = 300;
    vector_push(unwrapped, &c);
    ASSERT_EQUAL(IntVector_len(typed_vec), 3, "Typed vector reflects changes to unwrapped raw vector");
    
    /* 4. Destroying the typed wrapper should destroy the underlying vector */
    IntVector_destroy(typed_vec);
    return 1;
}

static int test_typed_to_generic_cast(void) {
    /* 1. Create a typed vector and put some data in it */
    IntVector *typed_vec = IntVector_create();
    IntVector_push(typed_vec, 100);
    IntVector_push(typed_vec, 200);
    
    /* 2. Cast it to a generic vector */
    Vector *raw_vec = (Vector *)typed_vec;
    ASSERT_NOT_NULL(raw_vec, "Vector cast");
    ASSERT_EQUAL(vector_len(raw_vec), 2, "Cast vector length");
    ASSERT_EQUAL(*(int *)vector_at(raw_vec, 0), 100, "Cast element 0");
    
    /* 3. Cast it back to a typed vector */
    IntVector *unwrapped = (IntVector *)raw_vec;
    ASSERT_PTR_EQUAL((void*)unwrapped, (void*)typed_vec, "Cast back pointer matches original");
    
    int c = 300;
    vector_push((Vector *)unwrapped, &c);
    ASSERT_EQUAL(IntVector_len(typed_vec), 3, "Typed vector reflects changes to unwrapped raw vector");
    
    /* 4. Destroying the typed wrapper should destroy the underlying vector */
    IntVector_destroy(typed_vec);
    return 1;
}

static int test_generic_to_typed_cast_invalid(void) {
    /* 1. Create a generic vector of a different type */
    Vector *raw_vec = vector_create(sizeof(double));
    double x = 3.14, y = 2.718;
    vector_push(raw_vec, &x);
    vector_push(raw_vec, &y);
    
    /* 2. Cast it to a typed vector of a different type */
    IntVector *typed_vec = (IntVector *)raw_vec;
    ASSERT_NOT_NULL(typed_vec, "IntVector cast");
    
    /* Accessing elements through the wrong type should not crash but will give incorrect results */
    int val0 = IntVector_at(typed_vec, 0);
    int val1 = IntVector_at(typed_vec, 1);
    ASSERT_NOT_EQUAL(val0, 3.14, "Accessing double as int should not equal original double");
    ASSERT_NOT_EQUAL(val1, 2.718, "Accessing double as int should not equal original double");
    
    /* Clean up */
    vector_destroy(raw_vec);
    return 1;
}

/* ============================================================================
 * Performance Tests
 * ============================================================================ */

static int perf_vector_push(void) {
    const int N = 1000000;
    IntVector *vec = IntVector_create();

    Timer t = timer_start();
    for (int i = 0; i < N; i++) {
        IntVector_push(vec, i);
    }
    double seconds = timer_elapsed(&t);

    printf("    IntVector_push %d times: %.3f seconds\n", N, seconds);
    ASSERT_EQUAL(IntVector_len(vec), N, "all elements pushed");

    IntVector_destroy(vec);
    return 1;
}

static int perf_deque_push_back(void) {
    const int N = 1000000;
    IntDeque *deq = IntDeque_create();

    Timer t = timer_start();
    for (int i = 0; i < N; i++) {
        IntDeque_push_back(deq, i);
    }
    double seconds = timer_elapsed(&t);

    printf("    IntDeque_push_back %d times: %.3f seconds\n", N, seconds);
    ASSERT_EQUAL(IntDeque_len(deq), N, "all elements pushed");

    IntDeque_destroy(deq);
    return 1;
}

static int perf_list_push_back(void) {
    const int N = 1000000;
    IntList *list = IntList_create();

    Timer t = timer_start();
    for (int i = 0; i < N; i++) {
        IntList_push_back(list, i);
    }
    double seconds = timer_elapsed(&t);

    printf("    IntList_push_back %d times: %.3f seconds\n", N, seconds);
    ASSERT_EQUAL(IntList_len(list), N, "all elements pushed");

    IntList_destroy(list);
    return 1;
}

static int perf_hashset_insert(void) {
    const int N = 500000;
    IntSet *set = IntSet_create();

    Timer t = timer_start();
    for (int i = 0; i < N; i++) {
        IntSet_insert(set, i);
    }
    double seconds = timer_elapsed(&t);

    printf("    IntSet_insert %d times: %.3f seconds\n", N, seconds);
    ASSERT_EQUAL(IntSet_len(set), N, "all elements inserted");

    IntSet_destroy(set);
    return 1;
}

static int perf_hashmap_insert(void) {
    const int N = 500000;
    char key_buf[32];
    StrIntMap *map = StrIntMap_create();

    Timer t = timer_start();
    for (int i = 0; i < N; i++) {
        snprintf(key_buf, sizeof(key_buf), "key_%d", i);
        StrIntMap_insert(map, key_buf, i);
    }
    double seconds = timer_elapsed(&t);

    printf("    StrIntMap_insert %d times: %.3f seconds\n", N, seconds);
    ASSERT_EQUAL(StrIntMap_len(map), N, "all pairs inserted");

    StrIntMap_destroy(map);
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  Type-Safe Unit Tests\n");
    printf("========================================\n\n");
    Timer total = timer_start();

    /* Vector Tests */
    TEST(test_vector_int_basic);
    TEST(test_vector_int_pop);
    TEST(test_vector_int_find);
    TEST(test_vector_int_sort_reverse);
    TEST(test_vector_string);
    TEST(test_vector_sort_strings);
    TEST(test_vector_double);
    TEST(test_vector_struct);
    TEST(test_vector_slice_clone);

    /* Deque Tests */
    TEST(test_deque_int_basic);
    TEST(test_deque_int_pop_insert_remove);
    TEST(test_deque_int_find);
    TEST(test_deque_int_sort_reverse);
    TEST(test_deque_string);
    TEST(test_deque_sort_strings);
    TEST(test_deque_double);
    TEST(test_deque_struct);
    TEST(test_deque_slice_clone);

    /* LinkedList Tests */
    TEST(test_list_int_basic);
    TEST(test_list_int_pop_insert_remove);
    TEST(test_list_int_find);
    TEST(test_list_int_sort_reverse);
    TEST(test_list_string);
    TEST(test_list_sort_strings);   
    TEST(test_list_double);
    TEST(test_list_struct);
    TEST(test_list_clone);

    /* HashSet Tests */
    TEST(test_set_int_basic);
    TEST(test_set_int_operations);
    TEST(test_set_int_reserve_shrink);
    TEST(test_set_set_operations);
    TEST(test_set_string);
    TEST(test_set_double);
    TEST(test_set_struct);
    TEST(test_set_clone);

    /* HashMap Tests */
    TEST(test_map_int_int_basic);
    TEST(test_map_int_int_operations);
    TEST(test_map_int_int_remove_entry);
    TEST(test_map_str_int);
    TEST(test_map_str_str);
    TEST(test_map_int_double);
    TEST(test_map_int_record);
    TEST(test_map_clone);
    TEST(test_map_keys_values);

    /* Iterator Tests */
    TEST(test_vector_iterators);
    TEST(test_deque_iterators);
    TEST(test_list_iterators);
    TEST(test_set_iterators);
    TEST(test_map_iterators);
    TEST(test_vector_string_iterators);
    TEST(test_deque_string_iterators);
    TEST(test_list_string_iterators);
    TEST(test_set_string_iterators);
    TEST(test_map_string_iterators);
    
    /* Cast Tests */
    TEST(test_vector_wrap_unwrap);
    TEST(test_generic_to_typed_cast);
    TEST(test_typed_to_generic_cast);
    TEST(test_generic_to_typed_cast_invalid);

    /* Performance Tests */
    TEST(perf_vector_push);
    TEST(perf_deque_push_back);
    TEST(perf_list_push_back);
    TEST(perf_hashset_insert);
    TEST(perf_hashmap_insert);

    double total_time = timer_elapsed(&total);

    printf("\n========================================\n");
    printf("  Results\n");
    printf("========================================\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Passed:       %d\n", tests_passed);
    printf("  Failed:       %d\n", tests_run - tests_passed);
    printf("  Duration:     %.3f seconds\n", total_time);
    printf("========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
