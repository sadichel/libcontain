/**
 * @file bench_deque.c
 * @brief libcontain Deque benchmarks using generic API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEQUE_IMPLEMENTATION
#include <contain/deque.h>

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

static void run_deque_push_back_no_reserve(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_push_back(deq, &i);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void run_deque_push_back_reserved(int n) {
    Deque *deq = deque_create(sizeof(int));
    deque_reserve(deq, n);
    for (int i = 0; i < n; i++) {
        deque_push_back(deq, &i);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void run_deque_push_front_no_reserve(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_push_front(deq, &i);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void run_deque_push_front_reserved(int n) {
    Deque *deq = deque_create(sizeof(int));
    deque_reserve(deq, n);
    for (int i = 0; i < n; i++) {
        deque_push_front(deq, &i);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void run_deque_push_string(int n) {
    Deque *deq = deque_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        deque_push_back(deq, buf);
    }
    sink += deque_len(deq);
    deque_destroy(deq);
}

static void run_deque_pop_back(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    for (int i = 0; i < n; i++) deque_pop_back(deq);
    deque_destroy(deq);
}

static void run_deque_pop_front(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    for (int i = 0; i < n; i++) deque_pop_front(deq);
    deque_destroy(deq);
}

static void run_deque_sequential_at(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += *(int *)deque_at(deq, i);
    }
    (void)sum;
    deque_destroy(deq);
}

static void run_deque_random_at(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        int idx = rand() % n;
        sum += *(int *)deque_at(deq, idx);
    }
    (void)sum;
    deque_destroy(deq);
}

static void run_deque_find_mid(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    int val = n / 2;
    deque_find(deq, &val);
    deque_destroy(deq);
}

static void run_deque_find_last(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    int val = n - 1;
    deque_find(deq, &val);
    deque_destroy(deq);
}

static void run_deque_find_not_found(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    int val = -1;
    deque_find(deq, &val);
    deque_destroy(deq);
}

static void run_deque_insert_front(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_insert(deq, 0, &i);
    }
    deque_destroy(deq);
}

static void run_deque_insert_middle(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_insert(deq, deque_len(deq) / 2, &i);
    }
    deque_destroy(deq);
}

static void run_deque_insert_back(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_insert(deq, deque_len(deq), &i);
    }
    deque_destroy(deq);
}

static void run_deque_remove_front(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    for (int i = 0; i < n; i++) deque_remove(deq, 0);
    deque_destroy(deq);
}

static void run_deque_remove_middle(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    for (int i = 0; i < n; i++) deque_remove(deq, deque_len(deq) / 2);
    deque_destroy(deq);
}

static void run_deque_remove_back(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    for (int i = 0; i < n; i++) deque_remove(deq, deque_len(deq) - 1);
    deque_destroy(deq);
}

static void run_deque_insert_range(int n) {
    Deque *dst = deque_create(sizeof(int));
    Deque *src = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_push_back(dst, &i);
        deque_push_back(src, &i);
    }
    deque_insert_range(dst, n / 2, src, 0, n);
    deque_destroy(dst);
    deque_destroy(src);
}

static void run_deque_append(int n) {
    Deque *dst = deque_create(sizeof(int));
    Deque *src = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        deque_push_back(dst, &i);
        deque_push_back(src, &i);
    }
    deque_append(dst, src);
    deque_destroy(dst);
    deque_destroy(src);
}

static void run_deque_remove_range(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n * 2; i++) deque_push_back(deq, &i);
    deque_remove_range(deq, n / 2, n + n / 2);
    deque_destroy(deq);
}

static void run_deque_reverse(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) deque_push_back(deq, &i);
    deque_reverse_inplace(deq);
    deque_destroy(deq);
}

static void run_deque_sort(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        int val = rand();
        deque_push_back(deq, &val);
    }
    deque_sort(deq, lc_compare_int);
    deque_destroy(deq);
}

static void run_deque_unique(int n) {
    Deque *deq = deque_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        int val = i / 10;
        deque_push_back(deq, &val);
    }
    deque_unique(deq);
    deque_destroy(deq);
}

static void run_deque_find_string_last(int n) {
    Deque *deq = deque_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        deque_push_back(deq, buf);
    }
    const char *target = "str_99999";
    deque_find(deq, target);
    deque_destroy(deq);
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
        {"push_back 1M ints (no reserve)", N_PUSH, 0, run_deque_push_back_no_reserve},
        {"push_back 1M ints (reserved)", N_PUSH, 0, run_deque_push_back_reserved},
        {"push_front 1M ints (no reserve)", N_PUSH, 0, run_deque_push_front_no_reserve},
        {"push_front 1M ints (reserved)", N_PUSH, 0, run_deque_push_front_reserved},
        {"push 100k strings", N_STRING, 0, run_deque_push_string},
        {"pop_back 1M ints", N_PUSH, 0, run_deque_pop_back},
        {"pop_front 1M ints", N_PUSH, 0, run_deque_pop_front},
        {"sequential at 1M", N_PUSH, 0, run_deque_sequential_at},
        {"random at 1M", N_PUSH, 0, run_deque_random_at},
        {"find (50k in 100k)", N_FIND, 0, run_deque_find_mid},
        {"find last element", N_FIND, 0, run_deque_find_last},
        {"find not found", N_FIND, 0, run_deque_find_not_found},
        {"insert at front 10k", N_INSERT, 0, run_deque_insert_front},
        {"insert at middle 10k", N_INSERT, 0, run_deque_insert_middle},
        {"insert at back 10k", N_INSERT, 0, run_deque_insert_back},
        {"remove from front 10k", N_INSERT, 0, run_deque_remove_front},
        {"remove from middle 10k", N_INSERT, 0, run_deque_remove_middle},
        {"remove from back 10k", N_INSERT, 0, run_deque_remove_back},
        {"insert_range (10k into 10k)", N_INSERT, 0, run_deque_insert_range},
        {"append (10k)", N_INSERT, 0, run_deque_append},
        {"remove_range (10k from middle)", N_INSERT, 0, run_deque_remove_range},
        {"reverse 100k ints", N_FIND, 0, run_deque_reverse},
        {"sort 100k ints (random)", N_FIND, 0, run_deque_sort},
        {"unique 100k ints (sorted)", N_FIND, 0, run_deque_unique},
        {"find string (last)", N_STRING, 0, run_deque_find_string_last},
        {NULL, 0, 0, NULL}
    };
    
    printf("\n");
    printf("libcontain Deque Benchmarks\n");
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
    printf("+----------------------------------+----------+-----------------+\n");
    printf("| Operation                        | N        | Time (ms)       |\n");
    printf("+----------------------------------+----------+-----------------+\n");
    
    for (int i = 0; benches[i].name; i++) {
        printf("| %-32s | %-8d | %15.3f |\n",
               benches[i].name, benches[i].n,
               benches[i].time * 1000);
    }
    
    printf("+----------------------------------+----------+-----------------+\n");
    printf("\n");
    
    return 0;
}