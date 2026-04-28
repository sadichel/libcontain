/**
 * @file bench_vector.c
 * @brief libcontain Vector benchmarks using generic API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/vector.h>

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_PUSH 1000000
#define N_FIND 100000
#define N_INSERT 10000
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

static void run_vector_push_no_reserve(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_push(vec, &i);
    }
    sink += vector_len(vec);
    vector_destroy(vec);
}

static void run_vector_push_reserved(int n) {
    Vector *vec = vector_create(sizeof(int));
    vector_reserve(vec, n);
    for (int i = 0; i < n; i++) {
        vector_push(vec, &i);
    }
    sink += vector_len(vec);
    vector_destroy(vec);
}

static void run_vector_push_string(int n) {
    Vector *vec = vector_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        vector_push(vec, buf);
    }
    sink += vector_len(vec);
    vector_destroy(vec);
}

static void run_vector_pop_back(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    for (int i = 0; i < n; i++) vector_pop(vec);
    vector_destroy(vec);
}

static void run_vector_sequential_at(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += *(int *)vector_at(vec, i);
    }
    (void)sum;
    vector_destroy(vec);
}

static void run_vector_random_at(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        int idx = rand() % n;
        sum += *(int *)vector_at(vec, idx);
    }
    (void)sum;
    vector_destroy(vec);
}

static void run_vector_find_mid(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    int val = n / 2;
    vector_find(vec, &val);
    vector_destroy(vec);
}

static void run_vector_find_last(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    int val = n - 1;
    vector_find(vec, &val);
    vector_destroy(vec);
}

static void run_vector_find_not_found(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    int val = -1;
    vector_find(vec, &val);
    vector_destroy(vec);
}

static void run_vector_insert_front(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_insert(vec, 0, &i);
    }
    vector_destroy(vec);
}

static void run_vector_insert_middle(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_insert(vec, vector_len(vec) / 2, &i);
    }
    vector_destroy(vec);
}

static void run_vector_insert_back(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_insert(vec, vector_len(vec), &i);
    }
    vector_destroy(vec);
}

static void run_vector_remove_front(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    for (int i = 0; i < n; i++) vector_remove(vec, 0);
    vector_destroy(vec);
}

static void run_vector_remove_middle(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    for (int i = 0; i < n; i++) vector_remove(vec, vector_len(vec) / 2);
    vector_destroy(vec);
}

static void run_vector_remove_back(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    for (int i = 0; i < n; i++) vector_remove(vec, vector_len(vec) - 1);
    vector_destroy(vec);
}

static void run_vector_insert_range(int n) {
    Vector *dst = vector_create(sizeof(int));
    Vector *src = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_push(dst, &i);
        vector_push(src, &i);
    }
    vector_insert_range(dst, n / 2, src, 0, n);
    vector_destroy(dst);
    vector_destroy(src);
}

static void run_vector_append(int n) {
    Vector *dst = vector_create(sizeof(int));
    Vector *src = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_push(dst, &i);
        vector_push(src, &i);
    }
    vector_append(dst, src);
    vector_destroy(dst);
    vector_destroy(src);
}

static void run_vector_remove_range(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n * 2; i++) vector_push(vec, &i);
    vector_remove_range(vec, n / 2, n + n / 2);
    vector_destroy(vec);
}

static void run_vector_reverse(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) vector_push(vec, &i);
    vector_reverse_inplace(vec);
    vector_destroy(vec);
}

static void run_vector_sort(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        int val = rand();
        vector_push(vec, &val);
    }
    vector_sort(vec, lc_compare_int);
    vector_destroy(vec);
}

static void run_vector_unique(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        int val = i / 10;
        vector_push(vec, &val);
    }
    vector_unique(vec);
    vector_destroy(vec);
}

static void run_vector_find_string_last(int n) {
    Vector *vec = vector_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        vector_push(vec, buf);
    }
    const char *target = "str_99999";
    vector_find(vec, target);
    vector_destroy(vec);
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
        {"push 1M ints (no reserve)", N_PUSH, 0, run_vector_push_no_reserve},
        {"push 1M ints (reserved)", N_PUSH, 0, run_vector_push_reserved},
        {"push 100k strings", N_STRING, 0, run_vector_push_string},
        {"pop_back 1M ints", N_PUSH, 0, run_vector_pop_back},
        {"sequential at 1M", N_PUSH, 0, run_vector_sequential_at},
        {"random at 1M", N_PUSH, 0, run_vector_random_at},
        {"find (50k in 100k)", N_FIND, 0, run_vector_find_mid},
        {"find last element", N_FIND, 0, run_vector_find_last},
        {"find not found", N_FIND, 0, run_vector_find_not_found},
        {"insert at front 10k", N_INSERT, 0, run_vector_insert_front},
        {"insert at middle 10k", N_INSERT, 0, run_vector_insert_middle},
        {"insert at back 10k", N_INSERT, 0, run_vector_insert_back},
        {"remove from front 10k", N_INSERT, 0, run_vector_remove_front},
        {"remove from middle 10k", N_INSERT, 0, run_vector_remove_middle},
        {"remove from back 10k", N_INSERT, 0, run_vector_remove_back},
        {"insert_range (10k into 10k)", N_INSERT, 0, run_vector_insert_range},
        {"append (10k)", N_INSERT, 0, run_vector_append},
        {"remove_range (10k from middle)", N_INSERT, 0, run_vector_remove_range},
        {"reverse 100k ints", N_FIND, 0, run_vector_reverse},
        {"sort 100k ints (random)", N_FIND, 0, run_vector_sort},
        {"unique 100k ints (sorted)", N_FIND, 0, run_vector_unique},
        {"find string (last)", N_STRING, 0, run_vector_find_string_last},
        {NULL, 0, 0, NULL}
    };
    
    printf("\n");
    printf("libcontain Vector Benchmarks\n");
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
