/**
 * @file allocator_test.c
 * @brief Test allocator sharing across libcontain containers
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assertion.h"

#include <contain/linkedlist.h>
#include <contain/hashset.h>
#include <contain/hashmap.h>
#include <contain/allocator.h>

/* ============================================================================
 * Test: Verify allocator usage via stats
 * ============================================================================ */

int test_allocator_stats_linkedlist(void) {
    Allocator *pool = pool_allocator_create(64, 100, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    
    /* After creation: no allocations, all blocks free */
    ASSERT_EQUAL(stats.capacity, 100, "capacity should be 100 blocks");
    ASSERT_EQUAL(stats.used, 0, "used should be 0 (no allocations)");
    ASSERT_EQUAL(stats.freelist, 100, "freelist should be 100");
    ASSERT_EQUAL(stats.segments, 1, "segments should be 1");
    
    LinkedList *list = linkedlist_create_with_allocator(sizeof(int), pool);
    ASSERT_NOT_NULL(list, "list creation failed");
    
    /* Push 50 elements - each allocates a node */
    for (int i = 0; i < 50; i++) {
        linkedlist_push_back(list, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 50, "should have 50 active allocations");
    ASSERT_EQUAL(stats.freelist, 50, "freelist should have 50 blocks");
    
    /* Pop 20 elements - frees nodes back to pool */
    for (int i = 0; i < 20; i++) {
        linkedlist_pop_front(list);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 30, "should have 30 active allocations");
    ASSERT_EQUAL(stats.freelist, 70, "freelist should have 70 blocks");
    
    /* Destroy list - frees remaining nodes */
    linkedlist_destroy(list);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 0, "all allocations should be freed");
    ASSERT_EQUAL(stats.freelist, 100, "freelist should be 100");
    
    allocator_destroy(pool);
    return 1;
}

int test_allocator_stats_hashset(void) {
    Allocator *pool = pool_allocator_create(64, 200, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    
    ASSERT_EQUAL(stats.capacity, 200, "capacity should be 200 blocks");
    ASSERT_EQUAL(stats.used, 0, "used should be 0");
    ASSERT_EQUAL(stats.freelist, 200, "freelist should be 200");
    
    HashSet *set = hashset_create_with_allocator(sizeof(int), pool);
    ASSERT_NOT_NULL(set, "set creation failed");
    
    for (int i = 0; i < 100; i++) {
        hashset_insert(set, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 100, "should have 100 allocations");
    ASSERT_EQUAL(stats.freelist, 100, "freelist should be 100");
    
    for (int i = 0; i < 30; i++) {
        hashset_remove(set, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 70, "should have 70 allocations");
    ASSERT_EQUAL(stats.freelist, 130, "freelist should be 130");
    
    hashset_destroy(set);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 0, "all allocations should be freed");
    ASSERT_EQUAL(stats.freelist, 200, "freelist should be 200");
    
    allocator_destroy(pool);
    return 1;
}

int test_allocator_stats_hashmap(void) {
    Allocator *pool = pool_allocator_create(64, 200, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    
    ASSERT_EQUAL(stats.capacity, 200, "capacity should be 200 blocks");
    ASSERT_EQUAL(stats.used, 0, "used should be 0");
    ASSERT_EQUAL(stats.freelist, 200, "freelist should be 200");
    
    HashMap *map = hashmap_create_with_allocator(sizeof(int), sizeof(int), pool);
    ASSERT_NOT_NULL(map, "map creation failed");
    
    for (int i = 0; i < 100; i++) {
        hashmap_insert(map, &i, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 100, "should have 100 allocations");
    ASSERT_EQUAL(stats.freelist, 100, "freelist should be 100");
    
    for (int i = 0; i < 30; i++) {
        hashmap_remove(map, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 70, "should have 70 allocations");
    ASSERT_EQUAL(stats.freelist, 130, "freelist should be 130");
    
    hashmap_destroy(map);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 0, "all allocations should be freed");
    ASSERT_EQUAL(stats.freelist, 200, "freelist should be 200");
    
    allocator_destroy(pool);
    return 1;
}

int test_allocator_stats_shared(void) {
    Allocator *pool = pool_allocator_create(64, 300, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    
    ASSERT_EQUAL(stats.capacity, 300, "capacity should be 300 blocks");
    ASSERT_EQUAL(stats.used, 0, "used should be 0");
    ASSERT_EQUAL(stats.freelist, 300, "freelist should be 300");
    
    LinkedList *list = linkedlist_create_with_allocator(sizeof(int), pool);
    HashSet *set = hashset_create_with_allocator(sizeof(int), pool);
    HashMap *map = hashmap_create_with_allocator(sizeof(int), sizeof(int), pool);
    
    ASSERT_NOT_NULL(list, "list creation failed");
    ASSERT_NOT_NULL(set, "set creation failed");
    ASSERT_NOT_NULL(map, "map creation failed");
    
    for (int i = 0; i < 30; i++) {
        linkedlist_push_back(list, &i);
        hashset_insert(set, &i);
        hashmap_insert(map, &i, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 90, "total allocations should be 90 (30×3)");
    ASSERT_EQUAL(stats.freelist, 210, "freelist should be 210");
    
    linkedlist_destroy(list);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 60, "should have 60 allocations left");
    ASSERT_EQUAL(stats.freelist, 240, "freelist should be 240");
    
    hashset_destroy(set);
    hashmap_destroy(map);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 0, "all allocations should be freed");
    ASSERT_EQUAL(stats.freelist, 300, "freelist should be 300");
    
    allocator_destroy(pool);
    return 1;
}

int test_allocator_stats_reset(void) {
    Allocator *pool = pool_allocator_create(64, 100, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    LinkedList *list = linkedlist_create_with_allocator(sizeof(int), pool);
    
    for (int i = 0; i < 50; i++) {
        linkedlist_push_back(list, &i);
    }
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 50, "should have 50 allocations");
    ASSERT_EQUAL(stats.freelist, 50, "freelist should be 50");
    
    linkedlist_destroy(list);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 0, "allocations should be 0");
    ASSERT_EQUAL(stats.freelist, 100, "freelist should be 100");
    
    allocator_reset(pool);
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.freelist, 100, "freelist unchanged after reset");
    
    allocator_destroy(pool);
    return 1;
}

int test_allocator_stats_growth(void) {
    Allocator *pool = pool_allocator_create(64, 10, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.segments, 1, "initial segments should be 1");
    ASSERT_EQUAL(stats.capacity, 10, "initial capacity should be 10 blocks");
    ASSERT_EQUAL(stats.used, 0, "used should be 0");
    ASSERT_EQUAL(stats.freelist, 10, "freelist should be 10");
    
    LinkedList *list = linkedlist_create_with_allocator(sizeof(int), pool);
    
    for (int i = 0; i < 30; i++) {
        linkedlist_push_back(list, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_GREATER(stats.segments, 1, "segments should have grown");
    ASSERT_GREATER(stats.capacity, 10, "capacity should have increased");
    ASSERT_EQUAL(stats.used, 30, "should have 30 allocations");
    ASSERT_EQUAL(stats.freelist, stats.capacity - 30, "freelist should be capacity - used");
    
    linkedlist_destroy(list);
    allocator_destroy(pool);
    return 1;
}

int test_allocator_reference_counting(void) {
    Allocator *pool = pool_allocator_create(64, 100, 8, GROW_DOUBLE);
    ASSERT_NOT_NULL(pool, "pool_allocator_create failed");
    
    AllocatorStats stats;
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.capacity, 100, "capacity should be 100");
    ASSERT_EQUAL(stats.used, 0, "used should be 0");
    
    LinkedList *list = linkedlist_create_with_allocator(sizeof(int), pool);
    ASSERT_NOT_NULL(list, "list creation failed");
    
    for (int i = 0; i < 20; i++) {
        linkedlist_push_back(list, &i);
    }
    
    allocator_stats(pool, &stats);
    ASSERT_EQUAL(stats.used, 20, "should have 20 allocations");
    
    allocator_destroy(pool);
    
    for (int i = 20; i < 40; i++) {
        linkedlist_push_back(list, &i);
    }
    
    ASSERT_EQUAL(linkedlist_len(list), 40, "list should still work after allocator_destroy");
    
    linkedlist_destroy(list);
    
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("  Container Allocator Tests\n");
    printf("========================================\n\n");
    
    TEST(test_allocator_stats_linkedlist);
    TEST(test_allocator_stats_hashset);
    TEST(test_allocator_stats_hashmap);
    TEST(test_allocator_stats_shared);
    TEST(test_allocator_stats_reset);
    TEST(test_allocator_stats_growth);
    TEST(test_allocator_reference_counting);
    
    printf("\n========================================\n");
    printf("  Results\n");
    printf("========================================\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Passed:       %d\n", tests_passed);
    printf("  Failed:       %d\n", tests_run - tests_passed);
    printf("========================================\n");
    
    return tests_passed == tests_run ? 0 : 1;
}
