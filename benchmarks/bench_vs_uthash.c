/**
 * @file bench_vs_uthash.c
 * @brief libcontain vs uthash comparison using typed API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/hashmap.h>
#include <contain/typed.h>

/* Declare typed containers */
DECL_HASHMAP_TYPE(int, int, sizeof(int), sizeof(int), IntIntMap)
DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), StrIntMap)

/* uthash */
#include "uthash.h"

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_HASH 100000
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
 * uthash structures
 * ============================================================================ */

typedef struct {
    int key;
    int value;
    UT_hash_handle hh;
} UthashIntEntry;

typedef struct {
    char key[64];
    int value;
    UT_hash_handle hh;
} UthashStrEntry;

/* ============================================================================
 * Benchmark functions
 * ============================================================================ */

static void run_uthash_int_insert(int n) {
    UthashIntEntry *map = NULL;
    for (int i = 0; i < n; i++) {
        UthashIntEntry *e = malloc(sizeof(UthashIntEntry));
        e->key = i;
        e->value = i;
        HASH_ADD_INT(map, key, e);
    }
    UthashIntEntry *curr, *tmp;
    HASH_ITER(hh, map, curr, tmp) {
        HASH_DEL(map, curr);
        free(curr);
    }
}

static void run_libcontain_int_insert(int n) {
    IntIntMap *map = IntIntMap_create();
    for (int i = 0; i < n; i++) {
        IntIntMap_insert(map, i, i);
    }
    sink += IntIntMap_len(map);
    IntIntMap_destroy(map);
}

static void run_uthash_str_insert(int n) {
    UthashStrEntry *map = NULL;
    for (int i = 0; i < n; i++) {
        UthashStrEntry *e = malloc(sizeof(UthashStrEntry));
        snprintf(e->key, sizeof(e->key), "key_%d", i);
        e->value = i;
        HASH_ADD_STR(map, key, e);
    }
    UthashStrEntry *curr, *tmp;
    HASH_ITER(hh, map, curr, tmp) {
        HASH_DEL(map, curr);
        free(curr);
    }
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
        double uthash;
        double libcontain;
        void (*uthash_fn)(int);
        void (*libcontain_fn)(int);
    } Bench;
    
    Bench benches[] = {
        {"int insert", N_HASH, 0, 0, run_uthash_int_insert, run_libcontain_int_insert},
        {"string insert", N_HASH, 0, 0, run_uthash_str_insert, run_libcontain_str_insert},
        {NULL, 0, 0, 0, NULL, NULL}
    };
    
    printf("\n");
    printf("libcontain vs uthash\n");
    printf("\n");
    
    /* Warmup */
    for (int i = 0; benches[i].name; i++) {
        for (int j = 0; j < WARMUP; j++) {
            benches[i].uthash_fn(benches[i].n);
            benches[i].libcontain_fn(benches[i].n);
        }
    }
    
    /* Measure */
    for (int i = 0; benches[i].name; i++) {
        double uthash_total = 0, libcontain_total = 0;
        for (int j = 0; j < RUNS; j++) {
            uthash_total += measure_time(benches[i].uthash_fn, benches[i].n);
            libcontain_total += measure_time(benches[i].libcontain_fn, benches[i].n);
        }
        benches[i].uthash = uthash_total / RUNS;
        benches[i].libcontain = libcontain_total / RUNS;
    }
    
    /* Print results */
    printf("+--------------------------+----------+-----------------+-----------------+------------+------------+\n");
    printf("| Operation                | N        | uthash (ms)     | libcontain (ms) | Winner     | Speedup (x)|\n");
    printf("+--------------------------+----------+-----------------+-----------------+------------+------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        double speedup;
        const char *winner;
        
        if (benches[i].libcontain < benches[i].uthash) {
            /* libcontain is faster: positive speedup */
            speedup = benches[i].uthash / benches[i].libcontain;
            winner = "libcontain";
        } else {
            /* uthash is faster: negative speedup */
            speedup = -(benches[i].libcontain / benches[i].uthash);
            winner = "uthash";
        }
        
        printf("| %-24s | %-8d | %-15.3f | %-15.3f | %-10s | %+10.2f |\n",
               benches[i].name, benches[i].n,
               benches[i].uthash * 1000, benches[i].libcontain * 1000,
               winner, speedup);
    }
    
    printf("+--------------------------+----------+-----------------+-----------------+------------+------------+\n");
    
    /* Summary */
    printf("\n");
    printf("Summary\n");
    printf("  String management: Manual (strdup) vs Automatic\n");
    printf("  Container suite:   Hash only vs Full suite\n");
    printf("  Iterator pipelines: No vs Yes\n");
    printf("  Type-safe wrappers: No vs Yes\n");
    printf("\n");
    
    return 0;
}
