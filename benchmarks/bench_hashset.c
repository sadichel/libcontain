/**
 * @file bench_hashset.c
 * @brief libcontain HashSet benchmarks using generic API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HASHSET_IMPLEMENTATION
#include <contain/hashset.h>

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_HASH 100000
#define N_STRING 100000
#define N_SET_OPS 50000
#define RUNS 10
#define WARMUP 3

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
 * Benchmark functions
 * ============================================================================ */

static void run_hashset_insert(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(set, &i);
    }
    sink += hashset_len(set);
    hashset_destroy(set);
}

static void run_hashset_insert_reserved(int n) {
    HashSet *set = hashset_create(sizeof(int));
    hashset_reserve(set, n);
    for (int i = 0; i < n; i++) {
        hashset_insert(set, &i);
    }
    sink += hashset_len(set);
    hashset_destroy(set);
}

static void run_hashset_insert_string(int n) {
    HashSet *set = hashset_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        hashset_insert(set, buf);
    }
    sink += hashset_len(set);
    hashset_destroy(set);
}

static void run_hashset_contains_mid(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    int val = n / 2;
    for (int i = 0; i < n; i++) {
        hashset_contains(set, &val);
    }
    hashset_destroy(set);
}

static void run_hashset_contains_last(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    int val = n - 1;
    for (int i = 0; i < n; i++) {
        hashset_contains(set, &val);
    }
    hashset_destroy(set);
}

static void run_hashset_contains_not_found(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    int val = -1;
    for (int i = 0; i < n; i++) {
        hashset_contains(set, &val);
    }
    hashset_destroy(set);
}

static void run_hashset_remove(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    for (int i = 0; i < n; i++) hashset_remove(set, &i);
    hashset_destroy(set);
}

static void run_hashset_iterate(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    Iterator it = hashset_iter(set);
    const void *item;
    while ((item = iter_next(&it))) {
        sink += *(int *)item;
    }
    hashset_destroy(set);
}

static void run_hashset_union(int n) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(a, &i);
        hashset_insert(b, &(int){i + n});
    }
    HashSet *result = hashset_union(a, b);
    sink += hashset_len(result);
    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(result);
}

static void run_hashset_intersection(int n) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(a, &i);
        if (i < n / 2) {
            hashset_insert(b, &i);
        } else {
            hashset_insert(b, &(int){i + n});
        }
    }
    HashSet *result = hashset_intersection(a, b);
    sink += hashset_len(result);
    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(result);
}

static void run_hashset_difference(int n) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(a, &i);
        if (i < n / 2) {
            hashset_insert(b, &i);
        } else {
            hashset_insert(b, &(int){i + n});
        }
    }
    HashSet *result = hashset_difference(a, b);
    sink += hashset_len(result);
    hashset_destroy(a);
    hashset_destroy(b);
    hashset_destroy(result);
}

static void run_hashset_merge(int n) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(a, &i);
        hashset_insert(b, &(int){i + n});
    }
    hashset_merge(a, b);
    sink += hashset_len(a);
    hashset_destroy(a);
    hashset_destroy(b);
}

static void run_hashset_subset(int n) {
    HashSet *a = hashset_create(sizeof(int));
    HashSet *b = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        hashset_insert(a, &i);
        hashset_insert(b, &i);
    }
    for (int i = n; i < n * 2; i++) {
        hashset_insert(b, &i);
    }
    hashset_subset(a, b);
    hashset_destroy(a);
    hashset_destroy(b);
}

static void run_hashset_clone(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    HashSet *clone = hashset_clone(set);
    sink += hashset_len(clone);
    hashset_destroy(set);
    hashset_destroy(clone);
}

static void run_hashset_to_array(int n) {
    HashSet *set = hashset_create(sizeof(int));
    for (int i = 0; i < n; i++) hashset_insert(set, &i);
    Array *arr = hashset_to_array(set);
    sink += arr->len;
    free(arr);
    hashset_destroy(set);
}

static void run_hashset_string_contains_last(int n) {
    HashSet *set = hashset_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        hashset_insert(set, buf);
    }
    const char *target = "str_99999";
    for (int i = 0; i < n; i++) {
        hashset_contains(set, target);
    }
    hashset_destroy(set);
}

/* ============================================================================
 * Benchmark runner
 * ============================================================================ */

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
    
    typedef struct {
        const char *name;
        int n;
        double time;
        void (*fn)(int);
    } Bench;
    
    Bench benches[] = {
        {"insert 100k ints", N_HASH, 0, run_hashset_insert},
        {"insert 100k ints (reserved)", N_HASH, 0, run_hashset_insert_reserved},
        {"insert 100k strings", N_STRING, 0, run_hashset_insert_string},
        {"contains (50k in 100k)", N_HASH, 0, run_hashset_contains_mid},
        {"contains last element", N_HASH, 0, run_hashset_contains_last},
        {"contains not found", N_HASH, 0, run_hashset_contains_not_found},
        {"remove 100k ints", N_HASH, 0, run_hashset_remove},
        {"iterate 100k entries", N_HASH, 0, run_hashset_iterate},
        {"union of two 50k sets", N_SET_OPS, 0, run_hashset_union},
        {"intersection (25k overlap)", N_SET_OPS, 0, run_hashset_intersection},
        {"difference (25k overlap)", N_SET_OPS, 0, run_hashset_difference},
        {"merge two 50k sets", N_SET_OPS, 0, run_hashset_merge},
        {"subset check (50k vs 100k)", N_SET_OPS, 0, run_hashset_subset},
        {"clone 100k set", N_HASH, 0, run_hashset_clone},
        {"to_array 100k set", N_HASH, 0, run_hashset_to_array},
        {"contains string (last)", N_STRING, 0, run_hashset_string_contains_last},
        {NULL, 0, 0, NULL}
    };
    
    printf("\n");
    printf("libcontain HashSet Benchmarks\n");
    printf("\n");
    
    /* Warmup */
    for (int i = 0; benches[i].name; i++) {
        for (int j = 0; j < WARMUP; j++) {
            benches[i].fn(benches[i].n);
        }
    }
    
    /* Measure */
    for (int i = 0; benches[i].name; i++) {
        double total = 0;
        for (int j = 0; j < RUNS; j++) {
            total += measure_time(benches[i].fn, benches[i].n);
        }
        benches[i].time = total / RUNS;
    }
    
    /* Print results */
    printf("+--------------------------------+----------+-----------------+\n");
    printf("| Operation                      | N        | Time (ms)       |\n");
    printf("+--------------------------------+----------+-----------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        printf("| %-30s | %-8d | %15.3f |\n",
               benches[i].name, benches[i].n,
               benches[i].time * 1000);
    }
    
    printf("+--------------------------------+----------+-----------------+\n");
    printf("\n");
    
    return 0;
}