/**
 * @file bench_linkedlist.c
 * @brief libcontain LinkedList benchmarks using generic API
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <contain/linkedlist.h>

/* Volatile sink to prevent optimization */
static volatile int sink = 0;

#define N_PUSH 500000
#define N_FIND 10000
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

static void run_list_push_back(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_push_back(list, &i);
    }
    sink += linkedlist_len(list);
    linkedlist_destroy(list);
}

static void run_list_push_front(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_push_front(list, &i);
    }
    sink += linkedlist_len(list);
    linkedlist_destroy(list);
}

static void run_list_push_string(int n) {
    LinkedList *list = linkedlist_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        linkedlist_push_back(list, buf);
    }
    sink += linkedlist_len(list);
    linkedlist_destroy(list);
}

static void run_list_pop_back(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    for (int i = 0; i < n; i++) linkedlist_pop_back(list);
    linkedlist_destroy(list);
}

static void run_list_pop_front(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    for (int i = 0; i < n; i++) linkedlist_pop_front(list);
    linkedlist_destroy(list);
}

static void run_list_sequential_at(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += *(int *)linkedlist_at(list, i);
    }
    (void)sum;
    linkedlist_destroy(list);
}

static void run_list_random_at(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    volatile int sum = 0;
    for (int i = 0; i < n; i++) {
        int idx = rand() % n;
        sum += *(int *)linkedlist_at(list, idx);
    }
    (void)sum;
    linkedlist_destroy(list);
}

static void run_list_find_mid(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    int val = n / 2;
    linkedlist_find(list, &val);
    linkedlist_destroy(list);
}

static void run_list_find_last(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    int val = n - 1;
    linkedlist_find(list, &val);
    linkedlist_destroy(list);
}

static void run_list_find_not_found(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    int val = -1;
    linkedlist_find(list, &val);
    linkedlist_destroy(list);
}

static void run_list_insert_front(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_insert(list, 0, &i);
    }
    linkedlist_destroy(list);
}

static void run_list_insert_middle(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_insert(list, linkedlist_len(list) / 2, &i);
    }
    linkedlist_destroy(list);
}

static void run_list_insert_back(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_insert(list, linkedlist_len(list), &i);
    }
    linkedlist_destroy(list);
}

static void run_list_remove_front(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    for (int i = 0; i < n; i++) linkedlist_remove(list, 0);
    linkedlist_destroy(list);
}

static void run_list_remove_middle(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    for (int i = 0; i < n; i++) linkedlist_remove(list, linkedlist_len(list) / 2);
    linkedlist_destroy(list);
}

static void run_list_remove_back(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    for (int i = 0; i < n; i++) linkedlist_remove(list, linkedlist_len(list) - 1);
    linkedlist_destroy(list);
}

static void run_list_insert_range(int n) {
    LinkedList *dst = linkedlist_create(sizeof(int));
    LinkedList *src = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_push_back(dst, &i);
        linkedlist_push_back(src, &i);
    }
    linkedlist_insert_range(dst, n / 2, src, 0, n);
    linkedlist_destroy(dst);
    linkedlist_destroy(src);
}

static void run_list_append(int n) {
    LinkedList *dst = linkedlist_create(sizeof(int));
    LinkedList *src = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        linkedlist_push_back(dst, &i);
        linkedlist_push_back(src, &i);
    }
    linkedlist_append(dst, src);
    linkedlist_destroy(dst);
    linkedlist_destroy(src);
}

static void run_list_remove_range(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n * 2; i++) linkedlist_push_back(list, &i);
    linkedlist_remove_range(list, n / 2, n + n / 2);
    linkedlist_destroy(list);
}

static void run_list_reverse(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) linkedlist_push_back(list, &i);
    linkedlist_reverse_inplace(list);
    linkedlist_destroy(list);
}

static void run_list_sort(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        int val = rand();
        linkedlist_push_back(list, &val);
    }
    linkedlist_sort(list, lc_compare_int);
    linkedlist_destroy(list);
}

static void run_list_unique(int n) {
    LinkedList *list = linkedlist_create(sizeof(int));
    for (int i = 0; i < n; i++) {
        int val = i / 10;
        linkedlist_push_back(list, &val);
    }
    linkedlist_unique(list);
    linkedlist_destroy(list);
}

static void run_list_find_string_last(int n) {
    LinkedList *list = linkedlist_str();
    char buf[64];
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "str_%d", i);
        linkedlist_push_back(list, buf);
    }
    const char *target = "str_99999";
    linkedlist_find(list, target);
    linkedlist_destroy(list);
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
        {"push_back 500k ints", N_PUSH, 0, run_list_push_back},
        {"push_front 500k ints", N_PUSH, 0, run_list_push_front},
        {"push 100k strings", N_STRING, 0, run_list_push_string},
        {"pop_back 500k ints", N_PUSH, 0, run_list_pop_back},
        {"pop_front 500k ints", N_PUSH, 0, run_list_pop_front},
        {"sequential at 100k", N_FIND, 0, run_list_sequential_at},
        {"random at 100k", N_FIND, 0, run_list_random_at},
        {"find (50k in 100k)", N_FIND, 0, run_list_find_mid},
        {"find last element", N_FIND, 0, run_list_find_last},
        {"find not found", N_FIND, 0, run_list_find_not_found},
        {"insert at front 10k", N_INSERT, 0, run_list_insert_front},
        {"insert at middle 10k", N_INSERT, 0, run_list_insert_middle},
        {"insert at back 10k", N_INSERT, 0, run_list_insert_back},
        {"remove from front 10k", N_INSERT, 0, run_list_remove_front},
        {"remove from middle 10k", N_INSERT, 0, run_list_remove_middle},
        {"remove from back 10k", N_INSERT, 0, run_list_remove_back},
        {"insert_range (10k into 10k)", N_INSERT, 0, run_list_insert_range},
        {"append (10k)", N_INSERT, 0, run_list_append},
        {"remove_range (10k from middle)", N_INSERT, 0, run_list_remove_range},
        {"reverse 100k ints", N_FIND, 0, run_list_reverse},
        {"sort 100k ints (random)", N_FIND, 0, run_list_sort},
        {"unique 100k ints (sorted)", N_FIND, 0, run_list_unique},
        {"find string (last)", N_STRING, 0, run_list_find_string_last},
        {NULL, 0, 0, NULL}
    };
    
    printf("\n");
    printf("libcontain LinkedList Benchmarks\n");
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