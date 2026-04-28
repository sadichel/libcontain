/**
 * @file bench_typed_vs_generic.c
 * @brief Benchmark comparing typed wrappers vs direct generic API
 *
 * Measures performance overhead of typed wrapper layer.
 * Typed wrappers should have near-zero overhead.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/vector.h>
#include <contain/deque.h>
#include <contain/linkedlist.h>
#include <contain/hashset.h>
#include <contain/hashmap.h>
#include <contain/typed.h>

/* Declare typed containers */
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_VECTOR_TYPE(const char*, 0, StringVector)
DECL_DEQUE_TYPE(int, sizeof(int), IntDeque)
DECL_LINKEDLIST_TYPE(int, sizeof(int), IntList)
DECL_HASHSET_TYPE(int, sizeof(int), IntSet)
DECL_HASHSET_TYPE(const char*, 0, StringSet)
DECL_HASHMAP_TYPE(int, int, sizeof(int), sizeof(int), IntIntMap)
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

#define N_VECTOR 500000
#define N_DEQUE 500000
#define N_LIST 500000
#define N_HASH 100000
#define N_STRING 100000
#define RUNS 10
#define WARMUP 3

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

/* ============================================================================
 * Timer
 * ============================================================================ */

#ifdef _WIN32
#include <windows.h>
static double get_time(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}
#else
#include <sys/time.h>
static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
#endif

typedef struct { double start; } Timer;
static inline Timer timer_start(void) { Timer t = { get_time() }; return t; }
static inline double timer_elapsed(Timer *t) { return get_time() - t->start; }

/* ============================================================================
 * Generic API benchmarks
 * ============================================================================ */

static void bench_generic_vector_push(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_push(vec, &i);
    }
    sink += vector_len(vec);
    vector_destroy(vec);
}

static void bench_generic_vector_at(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += *(int *)vector_at(vec, i);
    }
    (void)sum;
    vector_destroy(vec);
}

static void bench_generic_vector_push_string(int n) {
    Vector *vec = vector_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        vector_push(vec, buf);
    }
    sink += vector_len(vec);
    vector_destroy(vec);
}

static void bench_generic_vector_at_string(int n) {
    Vector *vec = vector_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        vector_push(vec, buf);
    }
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        const char *s = vector_at(vec, i);
        sum += strlen(s);
    }
    (void)sum;
    vector_destroy(vec);
}

static void bench_generic_deque_push_back(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_push_back(deq, &i);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void bench_generic_deque_push_front(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_push_front(deq, &i);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void bench_generic_list_push_back(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_push_back(list, &i);
    }
    sink += linkedlist_len(list);
    linkedlist_destroy(list);
}

static void bench_generic_hashset_insert(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(set, &i);
    }
    sink += hashset_len(set);
    hashset_destroy(set);
}

static void bench_generic_hashset_contains(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    volatile int found = 0;
    for (int i = 0; i < n; i++) {
        found += hashset_contains(set, &i) ? 1 : 0;
    }
    (void)found;
    hashset_destroy(set);
}

static void bench_generic_hashset_insert_string(int n) {
    HashSet *set = hashset_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        hashset_insert(set, buf);
    }
    sink += hashset_len(set);
    hashset_destroy(set);
}

static void bench_generic_hashset_contains_string(int n) {
    HashSet *set = hashset_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        hashset_insert(set, buf);
    }
    const char *target = "str_50000";
    volatile int found = 0;
    for (int i = 0; i < n; i++) {
        found += hashset_contains(set, target) ? 1 : 0;
    }
    (void)found;
    hashset_destroy(set);
}

static void bench_generic_hashmap_insert(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) {
        hashmap_insert(map, &i, &i);
    }
    sink += hashmap_len(map);
    hashmap_destroy(map);
}

static void bench_generic_hashmap_get(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get(map, &i);
        sum += val ? *val : 0;
    }
    (void)sum;
    hashmap_destroy(map);
}

static void bench_generic_hashmap_insert_string(int n) {
    HashMap *map = hashmap_create(0, sizeof(int));
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        hashmap_insert(map, key, &i);
    }
    sink += hashmap_len(map);
    hashmap_destroy(map);
}

static void bench_generic_hashmap_get_string(int n) {
    HashMap *map = hashmap_create(0, sizeof(int));
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        hashmap_insert(map, key, &i);
    }
    const char *target = "key_50000";
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get(map, target);
        sum += val ? *val : 0;
    }
    (void)sum;
    hashmap_destroy(map);
}

/* ============================================================================
 * Typed API benchmarks
 * ============================================================================ */

static void bench_typed_vector_push(int n) {
    IntVector *vec = IntVector_create();
    for (int i = 0; i < n; i++) {
        IntVector_push(vec, i);
    }
    sink += IntVector_len(vec);
    IntVector_destroy(vec);
}

static void bench_typed_vector_at(int n) {
    IntVector *vec = IntVector_create();
    for (int i = 0; i < n; i++) IntVector_push(vec, i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += IntVector_at(vec, i);
    }
    (void)sum;
    IntVector_destroy(vec);
}

static void bench_typed_vector_push_string(int n) {
    StringVector *vec = StringVector_create();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        StringVector_push(vec, buf);
    }
    sink += StringVector_len(vec);
    StringVector_destroy(vec);
}

static void bench_typed_vector_at_string(int n) {
    StringVector *vec = StringVector_create();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        StringVector_push(vec, buf);
    }
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        const char *s = StringVector_at(vec, i);
        sum += strlen(s);
    }
    (void)sum;
    StringVector_destroy(vec);
}

static void bench_typed_deque_push_back(int n) {
    IntDeque *deq = IntDeque_create();
    for (int i = 0; i < n; i++) {
        IntDeque_push_back(deq, i);
    }
    sink += IntDeque_len(deq);
    IntDeque_destroy(deq);
}

static void bench_typed_deque_push_front(int n) {
    IntDeque *deq = IntDeque_create();
    for (int i = 0; i < n; i++) {
        IntDeque_push_front(deq, i);
    }
    sink += IntDeque_len(deq);
    IntDeque_destroy(deq);
}

static void bench_typed_list_push_back(int n) {
    IntList *list = IntList_create();
    for (int i = 0; i < n; i++) {
        IntList_push_back(list, i);
    }
    sink += IntList_len(list);
    IntList_destroy(list);
}

static void bench_typed_hashset_insert(int n) {
    IntSet *set = IntSet_create();
    for (int i = 0; i < n; i++) {
        IntSet_insert(set, i);
    }
    sink += IntSet_len(set);
    IntSet_destroy(set);
}

static void bench_typed_hashset_contains(int n) {
    IntSet *set = IntSet_create();
    for (int i = 0; i < n; i++) IntSet_insert(set, i);
    volatile int found = 0;
    for (int i = 0; i < n; i++) {
        found += IntSet_contains(set, i) ? 1 : 0;
    }
    (void)found;
    IntSet_destroy(set);
}

static void bench_typed_hashset_insert_string(int n) {
    StringSet *set = StringSet_create();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        StringSet_insert(set, buf);
    }
    sink += StringSet_len(set);
    StringSet_destroy(set);
}

static void bench_typed_hashset_contains_string(int n) {
    StringSet *set = StringSet_create();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        StringSet_insert(set, buf);
    }
    const char *target = "str_50000";
    volatile int found = 0;
    for (int i = 0; i < n; i++) {
        found += StringSet_contains(set, target) ? 1 : 0;
    }
    (void)found;
    StringSet_destroy(set);
}

static void bench_typed_hashmap_insert(int n) {
    IntIntMap *map = IntIntMap_create();
    for (int i = 0; i < n; i++) {
        IntIntMap_insert(map, i, i);
    }
    sink += IntIntMap_len(map);
    IntIntMap_destroy(map);
}

static void bench_typed_hashmap_get(int n) {
    IntIntMap *map = IntIntMap_create();
    for (int i = 0; i < n; i++) IntIntMap_insert(map, i, i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += IntIntMap_get(map, i);
    }
    (void)sum;
    IntIntMap_destroy(map);
}

static void bench_typed_hashmap_insert_string(int n) {
    StrIntMap *map = StrIntMap_create();
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        StrIntMap_insert(map, key, i);
    }
    sink += StrIntMap_len(map);
    StrIntMap_destroy(map);
}

static void bench_typed_hashmap_get_string(int n) {
    StrIntMap *map = StrIntMap_create();
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        StrIntMap_insert(map, key, i);
    }
    const char *target = "key_50000";
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += StrIntMap_get(map, target);
    }
    (void)sum;
    StrIntMap_destroy(map);
}

/* ============================================================================
 * Benchmark runner
 * ============================================================================ */

typedef struct {
    const char *name;
    int n;
    double generic_time;
    double typed_time;
    void (*generic_fn)(int);
    void (*typed_fn)(int);
} Bench;

static double measure_time(void (*fn)(int), int n) {
    Timer t = timer_start();
    fn(n);
    return timer_elapsed(&t);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    srand(time(NULL));
    
    Bench benches[] = {
        {"vector_push", N_VECTOR, 0, 0, bench_generic_vector_push, bench_typed_vector_push},
        {"vector_at", N_VECTOR, 0, 0, bench_generic_vector_at, bench_typed_vector_at},
        {"vector_push (string)", N_STRING, 0, 0, bench_generic_vector_push_string, bench_typed_vector_push_string},
        {"vector_at (string)", N_STRING, 0, 0, bench_generic_vector_at_string, bench_typed_vector_at_string},
        {"deque_push_back", N_DEQUE, 0, 0, bench_generic_deque_push_back, bench_typed_deque_push_back},
        {"deque_push_front", N_DEQUE, 0, 0, bench_generic_deque_push_front, bench_typed_deque_push_front},
        {"list_push_back", N_LIST, 0, 0, bench_generic_list_push_back, bench_typed_list_push_back},
        {"hashset_insert", N_HASH, 0, 0, bench_generic_hashset_insert, bench_typed_hashset_insert},
        {"hashset_contains", N_HASH, 0, 0, bench_generic_hashset_contains, bench_typed_hashset_contains},
        {"hashset_insert (string)", N_STRING, 0, 0, bench_generic_hashset_insert_string, bench_typed_hashset_insert_string},
        {"hashset_contains (string)", N_STRING, 0, 0, bench_generic_hashset_contains_string, bench_typed_hashset_contains_string},
        {"hashmap_insert", N_HASH, 0, 0, bench_generic_hashmap_insert, bench_typed_hashmap_insert},
        {"hashmap_get", N_HASH, 0, 0, bench_generic_hashmap_get, bench_typed_hashmap_get},
        {"hashmap_insert (string)", N_STRING, 0, 0, bench_generic_hashmap_insert_string, bench_typed_hashmap_insert_string},
        {"hashmap_get (string)", N_STRING, 0, 0, bench_generic_hashmap_get_string, bench_typed_hashmap_get_string},
        {NULL, 0, 0, 0, NULL, NULL}
    };
    
    printf("\n");
    printf("Typed vs Generic Performance Benchmark\n");
    printf("\n");
    
    /* Warmup */
    for (int i = 0; benches[i].name; i++) {
        for (int j = 0; j < WARMUP; j++) {
            benches[i].generic_fn(benches[i].n);
            benches[i].typed_fn(benches[i].n);
        }
    }
    
    /* Measure */
    for (int i = 0; benches[i].name; i++) {
        double generic_total = 0, typed_total = 0;
        for (int j = 0; j < RUNS; j++) {
            generic_total += measure_time(benches[i].generic_fn, benches[i].n);
            typed_total += measure_time(benches[i].typed_fn, benches[i].n);
        }
        benches[i].generic_time = generic_total / RUNS;
        benches[i].typed_time = typed_total / RUNS;
    }
    
    /* Print results with Winner and Speedup columns */
    printf("+--------------------------------+----------+-----------------+-----------------+------------+------------+\n");
    printf("| Operation                      | N        | Generic (ms)    | Typed (ms)      | Winner     | Speedup (x)|\n");
    printf("+--------------------------------+----------+-----------------+-----------------+------------+------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        double speedup;
        const char *winner;
        
        if (benches[i].typed_time < benches[i].generic_time) {
            /* Typed is faster: positive speedup */
            speedup = benches[i].generic_time / benches[i].typed_time;
            winner = "Typed";
        } else {
            /* Generic is faster: negative speedup */
            speedup = -(benches[i].typed_time / benches[i].generic_time);
            winner = "Generic";
        }
        
        printf("| %-30s | %-8d | %15.3f | %15.3f | %-10s | %+10.2f |\n",
               benches[i].name, benches[i].n,
               benches[i].generic_time * 1000,
               benches[i].typed_time * 1000,
               winner, speedup);
    }
    
    printf("+--------------------------------+----------+-----------------+-----------------+------------+------------+\n");
    
    printf("\n");
    
    return 0;
}
