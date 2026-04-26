/**
 * @file bench_hashmap.c
 * @brief libcontain HashMap benchmarks using generic API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/hashmap.h>

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_HASH 100000
#define N_STRING 100000
#define N_MAP_OPS 50000
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

static void run_hashmap_insert_int_int(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) {
        hashmap_insert(map, &i, &i);
    }
    sink += hashmap_len(map);
    hashmap_destroy(map);
}

static void run_hashmap_insert_int_int_reserved(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    hashmap_reserve(map, n);
    for (int i = 0; i < n; i++) {
        hashmap_insert(map, &i, &i);
    }
    sink += hashmap_len(map);
    hashmap_destroy(map);
}

static void run_hashmap_insert_str_int(int n) {
    HashMap *map = hashmap_create(0, sizeof(int));
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        hashmap_insert(map, key, &i);
    }
    sink += hashmap_len(map);
    hashmap_destroy(map);
}

static void run_hashmap_insert_str_str(int n) {
    HashMap *map = hashmap_create(0, 0);
    char key[64];
    char val[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(val, sizeof(val), "val_%d", i);
        hashmap_insert(map, key, val);
    }
    sink += hashmap_len(map);
    hashmap_destroy(map);
}

static void run_hashmap_get_existing(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    int key = n / 2;
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get(map, &key);
        sink += val ? *val : 0;
    }
    hashmap_destroy(map);
}

static void run_hashmap_get_non_existing(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    int key = -1;
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get(map, &key);
        sink += val ? *val : 0;
    }
    hashmap_destroy(map);
}

static void run_hashmap_get_or_default_existing(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    int key = n / 2;
    int default_val = 0;
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get_or_default(map, &key, &default_val);
        sink += val ? *val : 0;
    }
    hashmap_destroy(map);
}

static void run_hashmap_get_or_default_non_existing(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    int key = -1;
    int default_val = 0;
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get_or_default(map, &key, &default_val);
        sink += val ? *val : 0;
    }
    hashmap_destroy(map);
}

static void run_hashmap_get_mut_existing(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    int key = n / 2;
    for (int i = 0; i < n; i++) {
        int *val = hashmap_get_mut(map, &key);
        if (val) (*val)++;
    }
    hashmap_destroy(map);
}

static void run_hashmap_get_mut_non_existing(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    int key = -1;
    for (int i = 0; i < n; i++) {
        int *val = hashmap_get_mut(map, &key);
        if (val) (*val)++;
    }
    hashmap_destroy(map);
}

static void run_hashmap_remove(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    for (int i = 0; i < n; i++) {
        hashmap_remove(map, &i);
    }
    hashmap_destroy(map);
}

static void run_hashmap_update(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    for (int i = 0; i < n; i++) {
        int new_val = i * 2;
        hashmap_insert(map, &i, &new_val);
    }
    hashmap_destroy(map);
}

static void run_hashmap_iterate(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    Iterator it = hashmap_iter(map);
    const void *entry;
    while ((entry = iter_next(&it))) {
        const int *key = hashmap_entry_key(map, entry);
        const int *val = hashmap_entry_value(map, entry);
        sink += *key + *val;
    }
    hashmap_destroy(map);
}

static void run_hashmap_merge(int n) {
    HashMap *dst = hashmap_create(sizeof(int), sizeof(int));
    HashMap *src = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) {
        hashmap_insert(dst, &i, &i);
        hashmap_insert(src, &(int){i + n}, &(int){i + n});
    }
    hashmap_merge(dst, src);
    sink += hashmap_len(dst);
    hashmap_destroy(dst);
    hashmap_destroy(src);
}

static void run_hashmap_clone(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    HashMap *clone = hashmap_clone(map);
    sink += hashmap_len(clone);
    hashmap_destroy(map);
    hashmap_destroy(clone);
}

static void run_hashmap_equals(int n) {
    HashMap *a = hashmap_create(sizeof(int), sizeof(int));
    HashMap *b = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) {
        hashmap_insert(a, &i, &i);
        hashmap_insert(b, &i, &i);
    }
    hashmap_equals(a, b);
    hashmap_destroy(a);
    hashmap_destroy(b);
}

static void run_hashmap_keys(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    Array *keys = hashmap_keys(map);
    sink += keys->len;
    free(keys);
    hashmap_destroy(map);
}

static void run_hashmap_values(int n) {
    HashMap *map = hashmap_create(sizeof(int), sizeof(int));
    for (int i = 0; i < n; i++) hashmap_insert(map, &i, &i);
    Array *values = hashmap_values(map);
    sink += values->len;
    free(values);
    hashmap_destroy(map);
}

static void run_hashmap_str_int_get_last(int n) {
    HashMap *map = hashmap_create(0, sizeof(int));
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        hashmap_insert(map, key, &i);
    }
    const char *target = "key_99999";
    for (int i = 0; i < n; i++) {
        const int *val = hashmap_get(map, target);
        sink += val ? *val : 0;
    }
    hashmap_destroy(map);
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
        {"insert 100k int->int", N_HASH, 0, run_hashmap_insert_int_int},
        {"insert 100k int->int (reserved)", N_HASH, 0, run_hashmap_insert_int_int_reserved},
        {"insert 100k str->int", N_STRING, 0, run_hashmap_insert_str_int},
        {"insert 100k str->str", N_STRING, 0, run_hashmap_insert_str_str},
        {"get existing (50k)", N_HASH, 0, run_hashmap_get_existing},
        {"get non-existing", N_HASH, 0, run_hashmap_get_non_existing},
        {"get_or_default existing", N_HASH, 0, run_hashmap_get_or_default_existing},
        {"get_or_default non-existing", N_HASH, 0, run_hashmap_get_or_default_non_existing},
        {"get_mut existing", N_HASH, 0, run_hashmap_get_mut_existing},
        {"get_mut non-existing", N_HASH, 0, run_hashmap_get_mut_non_existing},
        {"remove 100k int keys", N_HASH, 0, run_hashmap_remove},
        {"update existing 100k times", N_HASH, 0, run_hashmap_update},
        {"iterate 100k entries", N_HASH, 0, run_hashmap_iterate},
        {"merge two 50k maps", N_MAP_OPS, 0, run_hashmap_merge},
        {"clone 100k map", N_HASH, 0, run_hashmap_clone},
        {"equals on 100k maps", N_HASH, 0, run_hashmap_equals},
        {"keys array from 100k map", N_HASH, 0, run_hashmap_keys},
        {"values array from 100k map", N_HASH, 0, run_hashmap_values},
        {"get str->int (last)", N_STRING, 0, run_hashmap_str_int_get_last},
        {NULL, 0, 0, NULL}
    };
    
    printf("\n");
    printf("libcontain HashMap Benchmarks\n");
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
    printf("+---------------------------------+----------+-----------------+\n");
    printf("| Operation                       | N        | Time (ms)       |\n");
    printf("+---------------------------------+----------+-----------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        printf("| %-31s | %-8d | %15.3f |\n",
               benches[i].name, benches[i].n,
               benches[i].time * 1000);
    }
    
    printf("+---------------------------------+----------+-----------------+\n");
    printf("\n");
    
    return 0;
}