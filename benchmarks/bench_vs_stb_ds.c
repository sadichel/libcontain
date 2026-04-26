/**
 * @file bench_vs_stb_ds.c
 * @brief libcontain vs stb_ds comparison using typed API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/vector.h>
#include <contain/hashmap.h>
#include <contain/typed.h>

/* Declare typed containers */
DECL_HASHMAP_TYPE(int, int, sizeof(int), sizeof(int), IntIntMap)
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_VECTOR_TYPE(const char*, 0, StringVector)

/* stb_ds */
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_HASH 100000
#define N_VECTOR 1000000
#define N_STRING 100000
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

static void run_stb_ds_int_insert(int n) {
    struct { int key; int value; } *map = NULL;
    for (int i = 0; i < n; i++) {
        hmput(map, i, i);
    }
    sink += hmlen(map);
    hmfree(map);
}

static void run_libcontain_int_insert(int n) {
    IntIntMap *map = IntIntMap_create();
    for (int i = 0; i < n; i++) {
        IntIntMap_insert(map, i, i);
    }
    sink += IntIntMap_len(map);
    IntIntMap_destroy(map);
}

static void run_stb_ds_str_insert(int n) {
    struct { char *key; int value; } *map = NULL;
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        hmput(map, strdup(key), i);
    }
    for (int i = 0; i < hmlen(map); i++) {
        sink += map[i].value;
        free(map[i].key);
    }
    hmfree(map);
}

static void run_libcontain_str_insert(int n) {
    StrIntMap *map = StrIntMap_create();
    char key[64];
    for (int i = 0; i < n; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        StrIntMap_insert(map, key, i);
    }
    sink += StrIntMap_len(map);
    StrIntMap_destroy(map);
}

static void run_stb_ds_vector_push(int n) {
    int *arr = NULL;
    for (int i = 0; i < n; i++) {
        arrput(arr, i);
    }
    sink += arrlen(arr);
    arrfree(arr);
}

static void run_libcontain_vector_push(int n) {
    IntVector *vec = IntVector_create();
    for (int i = 0; i < n; i++) {
        IntVector_push(vec, i);
    }
    sink += IntVector_len(vec);
    IntVector_destroy(vec);
}

static void run_stb_ds_vector_push_string(int n) {
    char **arr = NULL;
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        arrput(arr, strdup(buf));
    }
    for (size_t i = 0; i < (size_t)arrlen(arr); i++) {
        free(arr[i]);
    }
    arrfree(arr);
}

static void run_libcontain_vector_push_string(int n) {
    StringVector *vec = StringVector_create();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        StringVector_push(vec, buf);
    }
    sink += StringVector_len(vec);
    StringVector_destroy(vec);
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
        double stb;
        double lc;
        void (*stb_fn)(int);
        void (*lc_fn)(int);
    } Bench;
    
    Bench benches[] = {
        {"int insert", N_HASH, 0, 0, run_stb_ds_int_insert, run_libcontain_int_insert},
        {"string insert", N_HASH, 0, 0, run_stb_ds_str_insert, run_libcontain_str_insert},
        {"vector push (int)", N_VECTOR, 0, 0, run_stb_ds_vector_push, run_libcontain_vector_push},
        {"vector push (string)", N_STRING, 0, 0, run_stb_ds_vector_push_string, run_libcontain_vector_push_string},
        {NULL, 0, 0, 0, NULL, NULL}
    };
    
    printf("\n");
    printf("libcontain vs stb_ds\n");
    printf("\n");
    
    /* Warmup */
    for (int i = 0; benches[i].name; i++) {
        for (int j = 0; j < WARMUP; j++) {
            benches[i].stb_fn(benches[i].n);
            benches[i].lc_fn(benches[i].n);
        }
    }
    
    /* Measure */
    for (int i = 0; benches[i].name; i++) {
        double stb_total = 0, lc_total = 0;
        for (int j = 0; j < RUNS; j++) {
            stb_total += measure_time(benches[i].stb_fn, benches[i].n);
            lc_total += measure_time(benches[i].lc_fn, benches[i].n);
        }
        benches[i].stb = stb_total / RUNS;
        benches[i].lc = lc_total / RUNS;
    }
    
    /* Print results */
    printf("+--------------------------+----------+-----------------+-----------------+------------+------------+\n");
    printf("| Operation                | N        | stb_ds (ms)     | libcontain (ms) | Winner     | Speedup (x)|\n");
    printf("+--------------------------+----------+-----------------+-----------------+------------+------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        double speedup;
        const char *winner;
        
        if (benches[i].lc < benches[i].stb) {
            /* libcontain is faster: positive speedup */
            speedup = benches[i].stb / benches[i].lc;
            winner = "libcontain";
        } else {
            /* stb_ds is faster: negative speedup */
            speedup = -(benches[i].lc / benches[i].stb);
            winner = "stb_ds";
        }
        
        printf("| %-24s | %-8d | %-15.3f | %-15.3f | %-10s | %+10.2f |\n",
               benches[i].name, benches[i].n,
               benches[i].stb * 1000, benches[i].lc * 1000,
               winner, speedup);
    }
    
    printf("+--------------------------+----------+-----------------+-----------------+------------+------------+\n");
    
    /* Summary */
    printf("\n");
    printf("Summary\n");
    printf("  String management: Manual (strdup) vs Automatic\n");
    printf("  Container suite:   Array/Hash vs Full suite\n");
    printf("  Iterator pipelines: No vs Yes\n");
    printf("  Chainer:           No vs Yes\n");
    printf("  Type-safe wrappers: No vs Yes\n");
    printf("\n");
    
    return 0;
}