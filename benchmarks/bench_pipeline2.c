/**
 * @file bench_pipeline2.c
 * @brief libcontain Iterator vs Chainer2 pipeline benchmarks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/vector.h>
#include <contain/iterator.h>
#include <contain/chainer2.h>

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_HUGE 1000000
#define N_FLATTEN 10
#define N_ZIP 1000000
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
 * Test helpers
 * ============================================================================ */

static Vector *create_test_vector(int n) {
    Vector *vec = vector_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        vector_push(vec, &i);
    }
    return vec;
}

static Vector *create_vector_of_vectors(int num_vectors, int elements_per_vector) {
    Vector *outer = vector_create(sizeof(Vector *));
    for (int i = 0; i < num_vectors; i++) {
        Vector *inner = create_test_vector(elements_per_vector);
        vector_push(outer, &inner);
    }
    return outer;
}

static bool is_even(const Container *ctx, const void *item) {
    (void)ctx;
    return *(const int *)item % 2 == 0;
}

static void *double_int(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    *(int *)buf = *(const int *)item * 2;
    return buf;
}

static void *square_int(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    int val = *(const int *)item;
    *(int *)buf = val * val;
    return buf;
}

static void *sum_fold(const Container *ctx, const void *item, void *acc) {
    (void)ctx;
    *(int *)acc += *(const int *)item;
    return acc;
}

static void sum_sink(const Container *ctx, const void *item) {
    (void)ctx;
    sink += *(const int *)item;
}

typedef struct { int a; int b; } Pair;

static void *make_pair(const Container *ca, const void *a,
                       const Container *cb, const void *b, void *buf) {
    (void)ca;
    (void)cb;
    Pair *p = (Pair *)buf;
    p->a = *(const int *)a;
    p->b = *(const int *)b;
    return buf;
}

/* ============================================================================
 * Iterator pipeline benchmarks
 * ============================================================================ */

static void bench_iterator_filter_only(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    while (iter_next(it)) {}
    iter_destroy(it);
    vector_destroy(vec);
}

static void bench_iterator_filter_map(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_map(
        iter_filter(HeapIter((Container *)vec), is_even),
        double_int, sizeof(int));
    while (iter_next(it)) {}
    iter_destroy(it);
    vector_destroy(vec);
}

static void bench_iterator_filter_map_take(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_take(
        iter_map(
            iter_filter(HeapIter((Container *)vec), is_even),
            double_int, sizeof(int)),
        10);
    while (iter_next(it)) {}
    iter_destroy(it);
    vector_destroy(vec);
}

static void bench_iterator_full_pipeline(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_take(
        iter_map(
            iter_filter(
                iter_skip(HeapIter((Container *)vec), 10),
                is_even),
            square_int, sizeof(int)),
        10);
    while (iter_next(it)) {}
    iter_destroy(it);
    vector_destroy(vec);
}

static void bench_iterator_fold(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    int sum = 0;
    iter_fold(it, &sum, sum_fold);
    sink += sum;
    vector_destroy(vec);
}

static void bench_iterator_collect(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    Container *result = iter_collect(it);
    sink += container_len(result);
    container_destroy(result);
    vector_destroy(vec);
}

static void bench_iterator_for_each(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_filter(HeapIter((Container *)vec), is_even);
    iter_for_each(it, sum_sink);
    vector_destroy(vec);
}

static void bench_iterator_multiple_filters(int n) {
    Vector *vec = create_test_vector(n);
    Iterator *it = iter_filter(
        iter_filter(
            iter_filter(HeapIter((Container *)vec), is_even),
            is_even),
        is_even);
    while (iter_next(it)) {}
    iter_destroy(it);
    vector_destroy(vec);
}

static void bench_iterator_flatten(int n) {
    Vector *outer = create_vector_of_vectors(n, n);
    Iterator *it = iter_flatten(HeapIter((Container *)outer));
    while (iter_next(it)) {}
    iter_destroy(it);
    for (size_t i = 0; i < vector_len(outer); i++) {
        Vector *inner = *(Vector **)vector_at(outer, i);
        vector_destroy(inner);
    }
    vector_destroy(outer);
}

static void bench_iterator_zip(int n) {
    Vector *vec1 = create_test_vector(n);
    Vector *vec2 = create_test_vector(n);
    Iterator *it = iter_zip(HeapIter((Container *)vec1),
                            HeapIter((Container *)vec2),
                            make_pair, sizeof(Pair));
    while (iter_next(it)) {}
    iter_destroy(it);
    vector_destroy(vec1);
    vector_destroy(vec2);
}

/* ============================================================================
 * Chainer2 pipeline benchmarks (same API as Chainer)
 * ============================================================================ */

static void bench_chainer2_filter_only(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_count(&c);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_filter_map(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    chain_count(&c);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_filter_map_take(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_map(&c, double_int, sizeof(int));
    chain_take(&c, 10);
    chain_count(&c);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_full_pipeline(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_skip(&c, 10);
    chain_filter(&c, is_even);
    chain_map(&c, square_int, sizeof(int));
    chain_take(&c, 10);
    chain_count(&c);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_fold(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    int sum = 0;
    chain_fold(&c, &sum, sum_fold);
    sink += sum;
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_collect(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    Container *result = chain_collect(&c);
    sink += container_len(result);
    container_destroy(result);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_for_each(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_for_each(&c, sum_sink);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_multiple_filters(int n) {
    Vector *vec = create_test_vector(n);
    Chainer c = Chain((Container *)vec);
    chain_filter(&c, is_even);
    chain_filter(&c, is_even);
    chain_filter(&c, is_even);
    chain_count(&c);
    chain_destroy(&c);
    vector_destroy(vec);
}

static void bench_chainer2_flatten(int n) {
    Vector *outer = create_vector_of_vectors(n, n);
    Chainer c = Chain((Container *)outer);
    chain_flatten(&c);
    chain_count(&c);
    chain_destroy(&c);
    for (size_t i = 0; i < vector_len(outer); i++) {
        Vector *inner = *(Vector **)vector_at(outer, i);
        vector_destroy(inner);
    }
    vector_destroy(outer);
}

static void bench_chainer2_zip(int n) {
    Vector *vec1 = create_test_vector(n);
    Vector *vec2 = create_test_vector(n);
    Chainer c = Chain((Container *)vec1);
    chain_zip(&c, (Container *)vec2, make_pair, sizeof(Pair));
    chain_count(&c);
    chain_destroy(&c);
    vector_destroy(vec1);
    vector_destroy(vec2);
}

/* ============================================================================
 * Benchmark runner
 * ============================================================================ */

typedef struct {
    const char *name;
    int n;
    double iter_time;
    double chain2_time;
    void (*iter_fn)(int);
    void (*chain2_fn)(int);
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
        {"Filter Only", N_HUGE, 0, 0, bench_iterator_filter_only, bench_chainer2_filter_only},
        {"Filter + Map", N_HUGE, 0, 0, bench_iterator_filter_map, bench_chainer2_filter_map},
        {"Filter + Map + Take", N_HUGE, 0, 0, bench_iterator_filter_map_take, bench_chainer2_filter_map_take},
        {"Full Pipeline", N_HUGE, 0, 0, bench_iterator_full_pipeline, bench_chainer2_full_pipeline},
        {"Fold", N_HUGE, 0, 0, bench_iterator_fold, bench_chainer2_fold},
        {"Collect", N_HUGE, 0, 0, bench_iterator_collect, bench_chainer2_collect},
        {"For Each", N_HUGE, 0, 0, bench_iterator_for_each, bench_chainer2_for_each},
        {"Multiple Filters", N_HUGE, 0, 0, bench_iterator_multiple_filters, bench_chainer2_multiple_filters},
        {"Flatten", N_FLATTEN, 0, 0, bench_iterator_flatten, bench_chainer2_flatten},
        {"Zip", N_ZIP, 0, 0, bench_iterator_zip, bench_chainer2_zip},
        {NULL, 0, 0, 0, NULL, NULL}
    };
    
    printf("\n");
    printf("Iterator vs Chainer2 Performance Benchmark\n");
    printf("\n");
    
    /* Warmup */
    for (int i = 0; benches[i].name; i++) {
        for (int j = 0; j < WARMUP; j++) {
            benches[i].iter_fn(benches[i].n);
            benches[i].chain2_fn(benches[i].n);
        }
    }
    
    /* Measure */
    for (int i = 0; benches[i].name; i++) {
        double iter_total = 0, chain2_total = 0;
        for (int j = 0; j < RUNS; j++) {
            iter_total += measure_time(benches[i].iter_fn, benches[i].n);
            chain2_total += measure_time(benches[i].chain2_fn, benches[i].n);
        }
        benches[i].iter_time = iter_total / RUNS;
        benches[i].chain2_time = chain2_total / RUNS;
    }
    
    /* Print results */
    printf("+--------------------------------+----------+-----------------+-----------------+----------------+\n");
    printf("| Operation                      | N        | Iterator (ms)   | Chainer2 (ms)   | Speedup        |\n");
    printf("+--------------------------------+----------+-----------------+-----------------+----------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        double speedup = benches[i].chain2_time / benches[i].iter_time;
        const char *winner = (speedup < 1.0) ? "Chainer2" : "Iterator";
        printf("| %-30s | %-8d | %15.3f | %15.3f | %-8s %5.2f |\n",
               benches[i].name, benches[i].n,
               benches[i].iter_time * 1000,
               benches[i].chain2_time * 1000,
               winner, speedup);
    }
    
    printf("+--------------------------------+----------+-----------------+-----------------+----------------+\n");
    printf("\n");
    printf("Note: Speedup < 1.0 means Chainer2 is faster\n");
    printf("      Speedup > 1.0 means Iterator is faster\n");
    printf("\n");
    
    return 0;
}