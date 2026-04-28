/**
 * @file tour.c
 * @brief libcontain Tour - A comprehensive introduction to the library
 * 
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <contain/container.h>
#include <contain/iterator.h>
#include <contain/chainer.h>
#include <contain/vector.h>
#include <contain/deque.h>
#include <contain/linkedlist.h>
#include <contain/hashset.h>
#include <contain/hashmap.h>
#include <contain/typed.h>

/* ============================================================================
 * Type-safe declarations (defined once, reused everywhere)
 * ============================================================================ */

/* Fixed-size containers */
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_VECTOR_TYPE(char, sizeof(char), CharVector)
DECL_VECTOR_TYPE(double, sizeof(double), DoubleVector)

/* String containers (item_size == 0) */
DECL_VECTOR_TYPE(const char*, 0, StringVector)
DECL_DEQUE_TYPE(const char*, 0, StringDeque)
DECL_LINKEDLIST_TYPE(const char*, 0, StringList)
DECL_HASHSET_TYPE(const char*, 0, StringSet)

/* HashMap: string -> int */
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), WordCountMap)

/* HashMap: string -> string */
DECL_HASHMAP_TYPE(const char*, const char*, 0, 0, StrStrMap)

/* For linkedlist demo */
DECL_LINKEDLIST_TYPE(double, sizeof(double), DoubleList)

/* ============================================================================
 * Generic factory functions (return Container* for polymorphism)
 * ============================================================================ */

/* Create a string vector from a sentence (generic container) */
static Container *create_string_vector(const char *sentence) {
    Vector *vec = vector_str();
    if (!vec) return NULL;

    char word[64];
    size_t word_len = 0;

    for (const char *p = sentence; *p; p++) {
        if (isalpha((unsigned char)*p)) {
            word[word_len++] = tolower((unsigned char)*p);
        } else if (word_len > 0) {
            word[word_len] = '\0';
            if (vector_push(vec, word) != LC_OK) {
                vector_destroy(vec);
                return NULL;
            }
            word_len = 0;
        }
    }

    if (word_len > 0) {
        word[word_len] = '\0';
        vector_push(vec, word);
    }

    return (Container *)vec;
}

/* ============================================================================
 * Callbacks for iterator pipelines
 * ============================================================================ */

/* Predicate: is character alphabetic? */
static bool is_alpha_char(const Container *ctx, const void *item) {
    (void)ctx;
    char c = *(const char *)item;
    return isalpha((unsigned char)c);
}

/* Transform: convert character to lowercase (in-place in buffer) */
static void *char_to_lower(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    char c = *(const char *)item;
    *(char *)buf = tolower((unsigned char)c);
    return buf;
}

/* Transform: convert string to uppercase (stride must be >= max_len + 1) */
static void *str_to_upper(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    const char *str = (const char *)item;
    char *out = (char *)buf;

    strcpy(out, str);
    for (char *p = out; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
    return buf;
}

/* Transform: remove punctuation from string */
static void *str_remove_punct(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    const char *str = (const char *)item;
    char *out = (char *)buf;

    char *write = out;
    for (const char *read = str; *read; read++) {
        if (isalnum((unsigned char)*read) || *read == ' ') {
            *write++ = *read;
        }
    }
    *write = '\0';
    return buf;
}

/* Predicate: filter out common stop words */
static bool is_not_stop_word(const Container *ctx, const void *item) {
    (void)ctx;
    const char *word = (const char *)item;

    static const char *stop_words[] = {
        "the", "and", "of", "to", "a", "in", "for", "on", "with", "by",
        "is", "are", "was", "were", "be", "been", "being", "have", "has",
        "had", "having", "do", "does", "did", "doing", "but", "or", "so",
        "for", "nor", "yet", "at", "by", "from", "into", "through",
        "during", "before", "after", "above", "below", "between"
    };

    size_t n = sizeof(stop_words) / sizeof(stop_words[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcmp(word, stop_words[i]) == 0)
            return false;
    }
    return true;
}

/* Predicate: is even? */
static bool is_even(const Container *ctx, const void *item) {
    (void)ctx;
    return *(const int *)item % 2 == 0;
}

/* Predicate: greater than 50 */
static bool is_gt_50(const Container *ctx, const void *item) {
    (void)ctx;
    return *(const int *)item > 50;
}

/* Transform: double an integer */
static void *double_int(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    *(int *)buf = *(const int *)item * 2;
    return buf;
}

/* Transform: square an integer */
static void *square_int(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    int val = *(const int *)item;
    *(int *)buf = val * val;
    return buf;
}

/* Fold: sum integers */
static void *sum_ints(const Container *ctx, const void *item, void *acc) {
    (void)ctx;
    *(int *)acc += *(const int *)item;
    return acc;
}

/* Fold: accumulate word counts */
static void *fold_count(const Container *ctx, const void *item, void *acc) {
    (void)ctx;
    WordCountMap *map = (WordCountMap *)acc;
    const char *word = (const char *)item;

    int current = WordCountMap_get_or_default(map, word, 0);
    WordCountMap_insert(map, word, current + 1);
    return acc;
}

/* ============================================================================
 * Demo 1: Vector - Basic operations with slice, append, insert_range, swap
 * ============================================================================ */

static void demo_vector(void) {
    printf("\n========================================\n");
    printf("  Demo 1: Vector - Dynamic Array\n");
    printf("========================================\n\n");

    /* Create and push */
    IntVector *vec = IntVector_create();
    for (int i = 0; i < 10; i++) {
        IntVector_push(vec, i * i);
    }

    printf("Initial: ");
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("%d ", IntVector_at(vec, i));
    }
    printf("\n");

    /* Insert and remove */
    IntVector_insert(vec, 3, 999);
    printf("After insert(3, 999): ");
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("%d ", IntVector_at(vec, i));
    }
    printf("\n");

    IntVector_remove(vec, 3);
    printf("After remove(3): ");
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("%d ", IntVector_at(vec, i));
    }
    printf("\n");

    /* Set and get */
    IntVector_set(vec, 0, 100);
    printf("After set(0, 100): first = %d\n", IntVector_front(vec));

    /* Find */
    size_t pos = IntVector_find(vec, 49);
    printf("Position of 49: %zu\n", pos);
    printf("Contains 25? %s\n", IntVector_contains(vec, 25) ? "yes" : "no");

    /* Pointer access */
    int *ptr = IntVector_get_ptr(vec, 5);
    if (ptr) {
        printf("Element at 5: %d\n", *ptr);
        *ptr = 888;
        printf("After modify via pointer: %d\n", IntVector_at(vec, 5));
    }

    /* Slice */
    IntVector *slice = IntVector_slice(vec, 2, 6);
    printf("Slice [2,6): ");
    for (size_t i = 0; i < IntVector_len(slice); i++) {
        printf("%d ", IntVector_at(slice, i));
    }
    printf("\n");

    /* Insert range */
    IntVector *src = IntVector_create();
    for (int i = 100; i <= 103; i++) {
        IntVector_push(src, i);
    }
    IntVector_insert_range(vec, 4, src, 0, IntVector_len(src));
    printf("After insert_range(4, [100,101,102,103]): ");
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("%d ", IntVector_at(vec, i));
    }
    printf("\n");

    /* Append */
    IntVector *src2 = IntVector_create();
    for (int i = 200; i <= 202; i++) {
        IntVector_push(src2, i);
    }
    IntVector_append(vec, src2);
    printf("After append([200,201,202]): ");
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("%d ", IntVector_at(vec, i));
    }
    printf("\n");

    /* Clone */
    IntVector *clone = IntVector_clone(vec);
    printf("Clone length: %zu\n", IntVector_len(clone));

    /* Swap */
    IntVector_swap(vec, clone);
    printf("After swap - original length: %zu, clone length: %zu\n",
           IntVector_len(vec), IntVector_len(clone));

    /* Instance (empty of same type) */
    IntVector *empty = IntVector_instance(vec);
    printf("Empty instance length: %zu\n", IntVector_len(empty));

    /* Cleanup */
    IntVector_destroy(vec);
    IntVector_destroy(slice);
    IntVector_destroy(src);
    IntVector_destroy(src2);
    IntVector_destroy(clone);
    IntVector_destroy(empty);
}

/* ============================================================================
 * Demo 2: Deque - Double-ended queue with append and insert_range
 * ============================================================================ */

static void demo_deque(void) {
    printf("\n========================================\n");
    printf("  Demo 2: Deque - Double-ended Queue\n");
    printf("========================================\n\n");

    StringDeque *dq = StringDeque_create();

    /* Push from both ends */
    StringDeque_push_back(dq, "world");
    StringDeque_push_front(dq, "Hello");
    StringDeque_push_back(dq, "!");
    StringDeque_push_front(dq, "Say:");

    printf("Deque: ");
    for (size_t i = 0; i < StringDeque_len(dq); i++) {
        printf("%s ", StringDeque_at(dq, i));
    }
    printf("\n");

    /* Pop from both ends */
    printf("Front: '%s', Back: '%s'\n",
           StringDeque_front(dq), StringDeque_back(dq));

    StringDeque_pop_front(dq);
    StringDeque_pop_back(dq);
    printf("After pop front/back: ");
    for (size_t i = 0; i < StringDeque_len(dq); i++) {
        printf("%s ", StringDeque_at(dq, i));
    }
    printf("\n");

    /* At or default */
    const char *val = StringDeque_at_or_default(dq, 10, "(not found)");
    printf("At index 10: %s\n", val);

    /* Insert range */
    StringDeque *src = StringDeque_create();
    StringDeque_push_back(src, "foo");
    StringDeque_push_back(src, "bar");
    StringDeque_insert_range(dq, 1, src, 0, StringDeque_len(src));
    printf("After insert_range(1, [foo,bar]): ");
    for (size_t i = 0; i < StringDeque_len(dq); i++) {
        printf("%s ", StringDeque_at(dq, i));
    }
    printf("\n");

    /* Append */
    StringDeque *src2 = StringDeque_create();
    StringDeque_push_back(src2, "baz");
    StringDeque_push_back(src2, "qux");
    StringDeque_append(dq, src2);
    printf("After append([baz,qux]): ");
    for (size_t i = 0; i < StringDeque_len(dq); i++) {
        printf("%s ", StringDeque_at(dq, i));
    }
    printf("\n");

    /* Clear and reuse */
    StringDeque_clear(dq);
    printf("After clear, empty? %s\n", StringDeque_is_empty(dq) ? "yes" : "no");

    StringDeque_destroy(dq);
    StringDeque_destroy(src);
    StringDeque_destroy(src2);
}

/* ============================================================================
 * Demo 3: LinkedList - Node-based sequence with append and insert_range
 * ============================================================================ */

static void demo_linkedlist(void) {
    printf("\n========================================\n");
    printf("  Demo 3: LinkedList - Doubly-linked list\n");
    printf("========================================\n\n");

    DoubleList *list = DoubleList_create();

    /* Push operations */
    for (int i = 1; i <= 5; i++) {
        DoubleList_push_back(list, i * 1.5);
    }
    DoubleList_push_front(list, 0.0);
    DoubleList_push_back(list, 99.9);

    printf("List: ");
    for (size_t i = 0; i < DoubleList_len(list); i++) {
        printf("%.1f ", DoubleList_at(list, i));
    }
    printf("\n");

    /* Insert and remove */
    DoubleList_insert(list, 3, 55.5);
    printf("After insert(3, 55.5): ");
    for (size_t i = 0; i < DoubleList_len(list); i++) {
        printf("%.1f ", DoubleList_at(list, i));
    }
    printf("\n");

    DoubleList_remove(list, 3);
    printf("After remove(3): ");
    for (size_t i = 0; i < DoubleList_len(list); i++) {
        printf("%.1f ", DoubleList_at(list, i));
    }
    printf("\n");

    /* Insert range */
    DoubleList *src = DoubleList_create();
    for (int i = 10; i <= 12; i++) {
        DoubleList_push_back(src, i * 1.1);
    }
    DoubleList_insert_range(list, 2, src, 0, DoubleList_len(src));
    printf("After insert_range(2, [11.0,12.1,13.2]): ");
    for (size_t i = 0; i < DoubleList_len(list); i++) {
        printf("%.1f ", DoubleList_at(list, i));
    }
    printf("\n");

    /* Append */
    DoubleList *src2 = DoubleList_create();
    for (int i = 20; i <= 21; i++) {
        DoubleList_push_back(src2, i * 1.1);
    }
    DoubleList_append(list, src2);
    printf("After append([22.0,23.1]): ");
    for (size_t i = 0; i < DoubleList_len(list); i++) {
        printf("%.1f ", DoubleList_at(list, i));
    }
    printf("\n");

    /* Reverse in place */
    DoubleList_reverse_inplace(list);
    printf("After reverse: ");
    for (size_t i = 0; i < DoubleList_len(list); i++) {
        printf("%.1f ", DoubleList_at(list, i));
    }
    printf("\n");

    /* Sublist */
    DoubleList *sub = DoubleList_sublist(list, 2, 5);
    printf("Sublist [2,5): ");
    for (size_t i = 0; i < DoubleList_len(sub); i++) {
        printf("%.1f ", DoubleList_at(sub, i));
    }
    printf("\n");

    /* Swap */
    DoubleList *list2 = DoubleList_clone(list);
    DoubleList_swap(list, list2);
    printf("After swap - original length: %zu, clone length: %zu\n",
           DoubleList_len(list), DoubleList_len(list2));

    DoubleList_destroy(list);
    DoubleList_destroy(list2);
    DoubleList_destroy(src);
    DoubleList_destroy(src2);
    DoubleList_destroy(sub);
}

/* ============================================================================
 * Demo 4: HashSet - Unique elements with merge
 * ============================================================================ */

static void demo_hashset(void) {
    printf("\n========================================\n");
    printf("  Demo 4: HashSet - Unique elements\n");
    printf("========================================\n\n");

    StringSet *set = StringSet_create();

    StringSet_insert(set, "apple");
    StringSet_insert(set, "banana");
    StringSet_insert(set, "apple");
    StringSet_insert(set, "cherry");
    StringSet_insert(set, "banana");

    printf("Set contains: ");
    Iterator it = StringSet_iter(set);
    const char *val;
    while ((val = iter_next(&it))) {
        printf("%s ", val);
    }
    printf("\n");

    printf("Contains 'apple'? %s\n", StringSet_contains(set, "apple") ? "yes" : "no");
    printf("Contains 'grape'? %s\n", StringSet_contains(set, "grape") ? "yes" : "no");

    StringSet_remove(set, "banana");
    printf("After remove 'banana': ");
    it = StringSet_iter(set);
    while ((val = iter_next(&it))) {
        printf("%s ", val);
    }
    printf("\n");

    /* Merge (in-place union) */
    StringSet *set2 = StringSet_create();
    StringSet_insert(set2, "cherry");
    StringSet_insert(set2, "date");
    StringSet_insert(set2, "elderberry");
    StringSet_merge(set, set2);
    printf("After merge with {cherry,date,elderberry}: ");
    it = StringSet_iter(set);
    while ((val = iter_next(&it))) {
        printf("%s ", val);
    }
    printf("\n");

    /* Union (returns new set) */
    StringSet *set3 = StringSet_create();
    StringSet_insert(set3, "fig");
    StringSet_insert(set3, "grape");
    StringSet *union_set = StringSet_union(set, set3);
    printf("Union with {fig,grape}: ");
    it = StringSet_iter(union_set);
    while ((val = iter_next(&it))) {
        printf("%s ", val);
    }
    printf("\n");

    /* Intersection */
    StringSet *intersection = StringSet_intersection(set, set2);
    printf("Intersection with {cherry,date,elderberry}: ");
    it = StringSet_iter(intersection);
    while ((val = iter_next(&it))) {
        printf("%s ", val);
    }
    printf("\n");

    /* Difference */
    StringSet *diff = StringSet_difference(set, set2);
    printf("Difference (set \\ set2): ");
    it = StringSet_iter(diff);
    while ((val = iter_next(&it))) {
        printf("%s ", val);
    }
    printf("\n");

    /* Subset */
    printf("Is subset? %s\n", StringSet_subset(set2, set) ? "yes" : "no");

    /* Equals */
    StringSet *set4 = StringSet_clone(set);
    printf("Equals clone? %s\n", StringSet_equals(set, set4) ? "yes" : "no");

    /* Swap */
    StringSet_swap(set, set4);
    printf("After swap - original size: %zu, clone size: %zu\n",
           StringSet_len(set), StringSet_len(set4));

    StringSet_destroy(set);
    StringSet_destroy(set2);
    StringSet_destroy(set3);
    StringSet_destroy(set4);
    StringSet_destroy(union_set);
    StringSet_destroy(intersection);
    StringSet_destroy(diff);
}

/* ============================================================================
 * Demo 5: HashMap - Key-value store (string -> int)
 * ============================================================================ */

static void demo_hashmap(void) {
    printf("\n========================================\n");
    printf("  Demo 5: HashMap - Key-value store (string -> int)\n");
    printf("========================================\n\n");

    WordCountMap *map = WordCountMap_create();

    WordCountMap_insert(map, "apple", 5);
    WordCountMap_insert(map, "banana", 3);
    WordCountMap_insert(map, "apple", 10);
    WordCountMap_insert(map, "cherry", 7);

    const char *keys[] = {"apple", "banana", "cherry", "grape"};
    for (int i = 0; i < 4; i++) {
        if (WordCountMap_contains(map, keys[i])) {
            printf("%s -> %d\n", keys[i], WordCountMap_get(map, keys[i]));
        } else {
            printf("%s -> (not found)\n", keys[i]);
        }
    }

    printf("grape (default): %d\n", WordCountMap_get_or_default(map, "grape", 0));

    int *val = WordCountMap_get_mut(map, "apple");
    if (val) {
        *val = 99;
        printf("After mutate: apple -> %d\n", WordCountMap_get(map, "apple"));
    }

    printf("\nAll entries:\n");
    Iterator it = WordCountMap_iter(map);
    const void *entry;
    while ((entry = iter_next(&it))) {
        const char *key = WordCountMap_entry_key(map, entry);
        int count = WordCountMap_entry_value(map, entry);
        printf("  %s: %d\n", key, count);
    }

    /* Keys and values arrays */
    Array *keys_arr = WordCountMap_keys(map);
    Array *vals_arr = WordCountMap_values(map);
    printf("\nKeys array: ");
    for (size_t i = 0; i < keys_arr->len; i++) {
        printf("%s ", ((const char **)keys_arr->items)[i]);
    }
    printf("\n");

    free(keys_arr);
    free(vals_arr);

    /* Merge */
    WordCountMap *map2 = WordCountMap_create();
    WordCountMap_insert(map2, "date", 4);
    WordCountMap_insert(map2, "elderberry", 6);
    WordCountMap_merge(map, map2);
    printf("\nAfter merge with {date:4, elderberry:6}:\n");
    it = WordCountMap_iter(map);
    while ((entry = iter_next(&it))) {
        const char *key = WordCountMap_entry_key(map, entry);
        int count = WordCountMap_entry_value(map, entry);
        printf("  %s: %d\n", key, count);
    }

    /* Clone */
    WordCountMap *clone = WordCountMap_clone(map);
    printf("\nClone size: %zu\n", WordCountMap_len(clone));

    /* Equals */
    printf("Equals clone? %s\n", WordCountMap_equals(map, clone) ? "yes" : "no");

    /* Swap */
    WordCountMap_swap(map, clone);
    printf("After swap - original size: %zu, clone size: %zu\n",
           WordCountMap_len(map), WordCountMap_len(clone));

    WordCountMap_destroy(map);
    WordCountMap_destroy(map2);
    WordCountMap_destroy(clone);
}

/* ============================================================================
 * Demo 5b: HashMap - String to String (str -> str)
 * ============================================================================ */

static void demo_hashmap_str_str(void) {
    printf("\n========================================\n");
    printf("  Demo 5b: HashMap - String to String (str -> str)\n");
    printf("========================================\n\n");

    StrStrMap *map = StrStrMap_create();

    /* Insert key-value pairs */
    StrStrMap_insert(map, "name", "Alice");
    StrStrMap_insert(map, "city", "New York");
    StrStrMap_insert(map, "country", "USA");
    StrStrMap_insert(map, "name", "Bob");  /* Update existing key */

    printf("After inserts:\n");
    printf("  name -> %s\n", StrStrMap_get(map, "name"));
    printf("  city -> %s\n", StrStrMap_get(map, "city"));
    printf("  country -> %s\n", StrStrMap_get(map, "country"));

    /* Get or default */
    const char *default_val = "unknown";
    const char *occupation = StrStrMap_get_or_default(map, "occupation", default_val);
    printf("  occupation (default): %s\n", occupation);

    /* Check existence */
    printf("\nContains 'city'? %s\n", StrStrMap_contains(map, "city") ? "yes" : "no");
    printf("Contains 'zip'? %s\n", StrStrMap_contains(map, "zip") ? "yes" : "no");

    /* Get mutable pointer */
    const char **val_ptr = StrStrMap_get_mut(map, "city");
    if (val_ptr) {
        printf("  Before mutate: city = %s\n", *val_ptr);
    }

    /* Remove entry */
    StrStrMap_remove(map, "country");
    printf("\nAfter removing 'country':\n");
    printf("  Contains 'country'? %s\n", StrStrMap_contains(map, "country") ? "yes" : "no");

    /* Iterate over all entries */
    printf("\nAll entries:\n");
    Iterator it = StrStrMap_iter(map);
    const void *entry;
    while ((entry = iter_next(&it))) {
        const char *key = StrStrMap_entry_key(map, entry);
        const char *value = StrStrMap_entry_value(map, entry);
        printf("  %s -> %s\n", key, value);
    }

    /* Keys and values arrays */
    Array *keys_arr = StrStrMap_keys(map);
    Array *vals_arr = StrStrMap_values(map);
    printf("\nKeys array: ");
    for (size_t i = 0; i < keys_arr->len; i++) {
        printf("%s ", ((const char **)keys_arr->items)[i]);
    }
    printf("\n");
    printf("Values array: ");
    for (size_t i = 0; i < vals_arr->len; i++) {
        printf("%s ", ((const char **)vals_arr->items)[i]);
    }
    printf("\n");

    free(keys_arr);
    free(vals_arr);

    /* Merge another map */
    StrStrMap *map2 = StrStrMap_create();
    StrStrMap_insert(map2, "occupation", "Engineer");
    StrStrMap_insert(map2, "hobby", "Reading");
    StrStrMap_merge(map, map2);
    printf("\nAfter merge with {occupation:Engineer, hobby:Reading}:\n");
    it = StrStrMap_iter(map);
    while ((entry = iter_next(&it))) {
        const char *key = StrStrMap_entry_key(map, entry);
        const char *value = StrStrMap_entry_value(map, entry);
        printf("  %s -> %s\n", key, value);
    }

    /* Clone and compare */
    StrStrMap *clone = StrStrMap_clone(map);
    printf("\nClone size: %zu\n", StrStrMap_len(clone));
    printf("Equals clone? %s\n", StrStrMap_equals(map, clone) ? "yes" : "no");

    /* Swap */
    StrStrMap_swap(map, clone);
    printf("After swap - original size: %zu, clone size: %zu\n",
           StrStrMap_len(map), StrStrMap_len(clone));

    StrStrMap_destroy(map);
    StrStrMap_destroy(map2);
    StrStrMap_destroy(clone);
}

/* ============================================================================
 * Demo 6: Iterator pipelines - Functional transformations
 * ============================================================================ */

static void demo_iterator_pipelines(void) {
    printf("\n========================================\n");
    printf("  Demo 6: Iterator Pipelines\n");
    printf("========================================\n\n");

    /* Part A: Character pipeline */
    CharVector *chars = CharVector_create();
    const char *text = "  Hello,  World!  ";
    for (const char *p = text; *p; p++) {
        CharVector_push(chars, *p);
    }

    printf("Original: \"%s\"\n", text);

    Iterator *it = HeapIter((Container *)chars);
    it = iter_filter(it, is_alpha_char);
    it = iter_map(it, char_to_lower, sizeof(char));

    Container *result = iter_collect(it);
    CharVector *result_chars = (CharVector *)result;

    printf("Filtered + lowercased: ");
    for (size_t i = 0; i < CharVector_len(result_chars); i++) {
        printf("%c", CharVector_at(result_chars, i));
    }
    printf("\n");

    CharVector_destroy(chars);
    CharVector_destroy(result_chars);

    /* Part B: Integer pipeline with multiple operations */
    IntVector *numbers = IntVector_create();
    for (int i = 1; i <= 20; i++) {
        IntVector_push(numbers, i);
    }

    printf("\nInteger pipeline:\n");
    printf("Original: ");
    for (size_t i = 0; i < IntVector_len(numbers); i++) {
        printf("%d ", IntVector_at(numbers, i));
    }
    printf("\n");

    it = HeapIter((Container *)numbers);
    it = iter_filter(it, is_even);
    it = iter_map(it, double_int, sizeof(int));
    it = iter_take(it, 5);

    result = iter_collect(it);
    IntVector *result_nums = (IntVector *)result;

    printf("Even -> double -> take 5: ");
    for (size_t i = 0; i < IntVector_len(result_nums); i++) {
        printf("%d ", IntVector_at(result_nums, i));
    }
    printf("\n");

    it = HeapIter((Container *)numbers);
    it = iter_filter(it, is_gt_50);
    size_t count = iter_count(it);
    printf("Numbers > 50: %zu\n", count);

    it = HeapIter((Container *)numbers);
    bool has_gt_50 = iter_any(it, is_gt_50);
    printf("Any > 50? %s\n", has_gt_50 ? "yes" : "no");

    it = HeapIter((Container *)numbers);
    bool all_gt_0 = iter_all(it, NULL);
    printf("Any elements? %s\n", all_gt_0 ? "yes" : "no");

    it = HeapIter((Container *)numbers);
    int sum = 0;
    iter_fold(it, &sum, sum_ints);
    printf("Sum of all numbers: %d\n", sum);

    it = HeapIter((Container *)numbers);
    const int *val = iter_find(it, is_even);
    printf("Find even: %d\n", *val);

    it = HeapIter((Container *)numbers);
    it = iter_peekable(it);
    val = iter_peek(it);
    printf("Peek first: %d\n", *(const int *)val);
    val = iter_peek(it);
    printf("Peek again (same): %d\n", *(const int *)val);
    val = iter_next(it);
    printf("Next (consumes): %d\n", *(const int *)val);
    val = iter_peek(it);
    printf("Peek after next: %d\n", *(const int *)val);
    iter_destroy(it);

    IntVector_destroy(numbers);
    IntVector_destroy(result_nums);

    /* Part C: String transformations with iter_map */
    StringVector *strs = StringVector_create();
    StringVector_push(strs, "hello, world!");
    StringVector_push(strs, "C programming is fun!");
    StringVector_push(strs, "libcontain rocks!");

    printf("\nString transformations:\n");
    printf("Original:\n");
    for (size_t i = 0; i < StringVector_len(strs); i++) {
        printf("  %s\n", StringVector_at(strs, i));
    }

    it = HeapIter((Container *)strs);
    it = iter_map(it, str_remove_punct, 256);
    it = iter_map(it, str_to_upper, 256);

    result = iter_collect(it);
    StringVector *result_strs = (StringVector *)result;


    printf("\nAfter remove_punct + to_upper:\n");
    for (size_t i = 0; i < StringVector_len(result_strs); i++) {
        printf("  %s\n", StringVector_at(result_strs, i));
    }

    StringVector_destroy(strs);
    StringVector_destroy(result_strs);
}

/* ============================================================================
 * Demo 7: Chainer - Fused pipeline with reuse
 * ============================================================================ */

static void demo_chainer(void) {
    printf("\n========================================\n");
    printf("  Demo 7: Chainer - Fused pipeline\n");
    printf("========================================\n\n");

    IntVector *numbers = IntVector_create();
    for (int i = 1; i <= 100; i++) {
        IntVector_push(numbers, i);
    }

    Chainer c = Chain((Container *)numbers);
    chain_skip(&c, 10);
    chain_filter(&c, is_even);
    chain_map(&c, square_int, sizeof(int));
    chain_take(&c, 10);

    size_t count = chain_count(&c);
    printf("After skip(10) + filter(even) + map(square) + take(10): %zu elements\n", count);

    Container *result = chain_collect(&c);
    IntVector *result_vec = (IntVector *)result;

    printf("Result: ");
    for (size_t i = 0; i < IntVector_len(result_vec); i++) {
        printf("%d ", IntVector_at(result_vec, i));
    }
    printf("\n");

    IntVector *numbers2 = IntVector_create();
    for (int i = 200; i >= 101; i--) {
        IntVector_push(numbers2, i);
    }

    chain_bind(&c, (Container *)numbers2);
    count = chain_count(&c);
    printf("\nReused on different data: %zu elements\n", count);

    result = chain_collect(&c);
    IntVector *result_vec2 = (IntVector *)result;

    printf("Result from second data: ");
    for (size_t i = 0; i < IntVector_len(result_vec2); i++) {
        printf("%d ", IntVector_at(result_vec2, i));
    }
    printf("\n");

    chain_destroy(&c);
    IntVector_destroy(numbers);
    IntVector_destroy(numbers2);
    IntVector_destroy(result_vec);
    IntVector_destroy(result_vec2);
}

/* ============================================================================
 * Demo 8: Container polymorphism - Generic algorithms
 * ============================================================================ */

static void print_container(Container *c, const char *name) {
    printf("%s: [", name);
    Iterator it = Iter(c);
    const void *val;
    bool first = true;
    while ((val = iter_next(&it))) {
        if (!first) printf(", ");
        printf("%d", *(const int *)val);
        first = false;
    }
    printf("]\n");
}

static void demo_polymorphism(void) {
    printf("\n========================================\n");
    printf("  Demo 8: Container Polymorphism\n");
    printf("========================================\n\n");

    IntVector *vec = IntVector_create();
    StringDeque *deq = StringDeque_create();
    DoubleList *list = DoubleList_create();

    for (int i = 1; i <= 5; i++) {
        IntVector_push(vec, i);
        StringDeque_push_back(deq, "x");
        DoubleList_push_back(list, i * 1.1);
    }

    print_container((Container *)vec, "Vector");

    int total = 0;
    iter_fold(HeapIter((Container *)vec), &total, sum_ints);
    printf("Sum of vector: %d\n", total);

    IntVector *empty = IntVector_instance(vec);
    printf("Empty vector length: %zu\n", IntVector_len(empty));

    IntVector_destroy(vec);
    StringDeque_destroy(deq);
    DoubleList_destroy(list);
    IntVector_destroy(empty);
}

/* ============================================================================
 * Demo 9: Real-world - Word frequency analysis
 * ============================================================================ */

static void demo_word_frequency(void) {
    printf("\n========================================\n");
    printf("  Demo 9: Word Frequency Analysis\n");
    printf("========================================\n\n");

    const char *text =
        "It was the best of times it was the worst of times "
        "it was the age of wisdom it was the age of foolishness "
        "it was the epoch of belief it was the epoch of incredulity "
        "it was the season of Light it was the season of Darkness "
        "it was the spring of hope it was the winter of despair "
        "we had everything before us we had nothing before us "
        "we were all going direct to Heaven we were all going direct "
        "the other way in short the period was so far like the present period "
        "that some of its noisiest authorities insisted on its being received "
        "for good or for evil in the superlative degree of comparison only";

    Container *generic_tokens = create_string_vector(text);
    StringVector *tokens = (StringVector *)generic_tokens;

    printf("Total tokens: %zu\n", StringVector_len(tokens));

    WordCountMap *freq = WordCountMap_create();

    Iterator *it = HeapIter((Container *)tokens);
    it = iter_filter(it, is_not_stop_word);
    iter_fold(it, freq, fold_count);

    printf("Unique words (excluding stop words): %zu\n\n", WordCountMap_len(freq));

    printf("Top 10 words:\n");
    printf("  Rank  Word                      Count\n");
    printf("  -------------------------------------\n");

    size_t shown = 0;
    Iterator it2 = WordCountMap_iter(freq);
    const void *entry;
    while ((entry = iter_next(&it2)) && shown < 10) {
        const char *word = WordCountMap_entry_key(freq, entry);
        int count = WordCountMap_entry_value(freq, entry);
        printf("  %3zu.  %-25s %d\n", ++shown, word, count);
    }

    StringVector_destroy(tokens);
    WordCountMap_destroy(freq);
}

/* ============================================================================
 * Demo 10: Wrap/Unwrap - Seamless generic to typed conversion
 * ============================================================================ */

static Container *create_generic_vector(void) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 1; i <= 10; i++) {
        vector_push(vec, &i);
    }
    return (Container *)vec;
}

static void demo_wrap_unwrap(void) {
    printf("\n========================================\n");
    printf("  Demo 10: Generic to Typed Cast\n");
    printf("========================================\n\n");

    Container *generic = create_generic_vector();
    IntVector *typed = (IntVector *)generic;

    printf("Cast to typed vector: ");
    for (size_t i = 0; i < IntVector_len(typed); i++) {
        printf("%d ", IntVector_at(typed, i));
    }
    printf("\n");

    Vector *raw = (Vector *)typed;
    printf("Cast back to generic - length: %zu, capacity: %zu\n",
           vector_len(raw), vector_capacity(raw));

    IntVector_destroy(typed);
}

/* ============================================================================
 * Demo 11: As slice - Zero-copy array view
 * ============================================================================ */

static void demo_as_slice(void) {
    printf("\n========================================\n");
    printf("  Demo 11: As Slice - Zero-copy array view\n");
    printf("========================================\n\n");

    IntVector *vec = IntVector_create();
    for (int i = 0; i < 10; i++) {
        IntVector_push(vec, i * 2);
    }

    const int *slice = IntVector_as_slice(vec);
    printf("Array slice: ");
    for (size_t i = 0; i < IntVector_len(vec); i++) {
        printf("%d ", slice[i]);
    }
    printf("\n");

    IntVector_destroy(vec);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  libcontain - Complete Tour\n");
    printf("  Generic Containers with Iterator Pipelines\n");
    printf("========================================\n");

    demo_vector();
    demo_deque();
    demo_linkedlist();
    demo_hashset();
    demo_hashmap();
    demo_hashmap_str_str();
    demo_iterator_pipelines();
    demo_chainer();
    demo_polymorphism();
    demo_word_frequency();
    demo_wrap_unwrap();
    demo_as_slice();

    printf("\n========================================\n");
    printf("  Tour Complete!\n");
    printf("========================================\n");
    printf("\nKey features demonstrated:\n");
    printf("  * Vector, Deque, LinkedList, HashSet, HashMap\n");
    printf("  * Type-safe wrappers (DECL_*_TYPE macros)\n");
    printf("  * Direct casting between generic and typed\n");
    printf("  * Iterator pipelines (filter, map, take, fold)\n");
    printf("  * Chainer - fused pipelines with reuse\n");
    printf("  * Container polymorphism\n");
    printf("  * First-class string support (size=0)\n");
    printf("  * get_ptr() for direct memory access\n");
    printf("  * get_mut() / get_or_default() for hash maps\n");
    printf("  * Set operations (union, intersection, difference, merge)\n");
    printf("  * Slice, insert_range, append, swap, clone, instance\n");
    printf("  * as_slice() - zero-copy array view\n");
    printf("  * String -> String hashmap\n");
    printf("========================================\n\n");

    return 0;
}
