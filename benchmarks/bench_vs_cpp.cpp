/**
 * @file bench_vs_cpp.cpp
 * @brief Benchmark comparing typed C containers vs C++ STL
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>

extern "C" {
#define VECTOR_IMPLEMENTATION
#define DEQUE_IMPLEMENTATION
#define LINKEDLIST_IMPLEMENTATION
#define HASHSET_IMPLEMENTATION
#define HASHMAP_IMPLEMENTATION

#include <contain/vector.h>
#include <contain/deque.h>
#include <contain/linkedlist.h>
#include <contain/hashset.h>
#include <contain/hashmap.h>
#include <contain/typed.h>

/* Declare typed C containers */
DECL_VECTOR_TYPE(int, sizeof(int), IntVector)
DECL_DEQUE_TYPE(int, sizeof(int), IntDeque)
DECL_LINKEDLIST_TYPE(int, sizeof(int), IntList)
DECL_HASHSET_TYPE(int, sizeof(int), IntSet)
DECL_HASHMAP_TYPE(int, int, sizeof(int), sizeof(int), IntIntMap)

/* String containers */
DECL_VECTOR_TYPE(const char*,  0, StringVector)
DECL_HASHSET_TYPE(const char*,  0, StringSet)
DECL_HASHMAP_TYPE(const char*,  int, 0, sizeof(int), StrIntMap)
DECL_HASHMAP_TYPE(const char*,  const char*,  0, 0, StrStrMap)
}

using namespace std::chrono;

const int WARMUP = 3;
const int RUNS = 10;

static std::string random_string(int len) {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::mt19937 rng(42);
    static std::uniform_int_distribution<> dist(0, sizeof(alphanum) - 2);
    std::string result;
    result.reserve(len);
    for (int i = 0; i < len; i++) {
        result += alphanum[dist(rng)];
    }
    return result;
}

struct BenchResult {
    const char* name;
    int n;
    double c_min;
    double c_avg;
    double c_max;
    double cpp_min;
    double cpp_avg;
    double cpp_max;
    double diff_percent;
    const char* winner;
};

std::vector<BenchResult> results;

template<typename FuncC, typename FuncCpp>
void measure_and_record(const char* name, int N, FuncC&& func_c, FuncCpp&& func_cpp) {
    /* Warmup */
    for (int i = 0; i < WARMUP; i++) {
        func_c();
        func_cpp();
    }
    
    /* Measure C */
    double c_min = 1e9, c_max = 0, c_sum = 0;
    for (int i = 0; i < RUNS; i++) {
        auto start = high_resolution_clock::now();
        func_c();
        auto end = high_resolution_clock::now();
        double ms = duration<double, std::milli>(end - start).count();
        if (ms < c_min) c_min = ms;
        if (ms > c_max) c_max = ms;
        c_sum += ms;
    }
    double c_avg = c_sum / RUNS;
    
    /* Measure C++ */
    double cpp_min = 1e9, cpp_max = 0, cpp_sum = 0;
    for (int i = 0; i < RUNS; i++) {
        auto start = high_resolution_clock::now();
        func_cpp();
        auto end = high_resolution_clock::now();
        double ms = duration<double, std::milli>(end - start).count();
        if (ms < cpp_min) cpp_min = ms;
        if (ms > cpp_max) cpp_max = ms;
        cpp_sum += ms;
    }
    double cpp_avg = cpp_sum / RUNS;
    
    /* Calculate percentage difference */
    double diff_percent = ((cpp_avg - c_avg) / c_avg) * 100.0;
    const char* winner = (c_avg < cpp_avg) ? "C" : "C++";
    
    results.push_back({name, N, c_min, c_avg, c_max, cpp_min, cpp_avg, cpp_max, diff_percent, winner});
}

/* ============================================================================
 * Vector Benchmarks
 * ============================================================================ */

static void bench_c_vector_push(int N) {
    IntVector *vec = IntVector_create();
    IntVector_reserve(vec, N);
    for (int i = 0; i < N; i++) IntVector_push(vec, i);
    IntVector_destroy(vec);
}

static void bench_cpp_vector_push(int N) {
    std::vector<int> vec;
    vec.reserve(N);
    for (int i = 0; i < N; i++) vec.push_back(i);
}

static void bench_c_vector_at(int N) {
    IntVector *vec = IntVector_create();
    IntVector_reserve(vec, N);
    for (int i = 0; i < N; i++) IntVector_push(vec, i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += IntVector_at(vec, i);
    (void)dummy;
    IntVector_destroy(vec);
}

static void bench_cpp_vector_at(int N) {
    std::vector<int> vec;
    vec.reserve(N);
    for (int i = 0; i < N; i++) vec.push_back(i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += vec[i];
    (void)dummy;
}

static void bench_c_vector_insert_front(int N) {
    IntVector *vec = IntVector_create();
    for (int i = 0; i < N; i++) IntVector_insert(vec, 0, i);
    IntVector_destroy(vec);
}

static void bench_cpp_vector_insert_front(int N) {
    std::vector<int> vec;
    for (int i = 0; i < N; i++) vec.insert(vec.begin(), i);
}

/* ============================================================================
 * Deque Benchmarks
 * ============================================================================ */

static void bench_c_deque_push_back(int N) {
    IntDeque *deq = IntDeque_create();
    IntDeque_reserve(deq, N);
    for (int i = 0; i < N; i++) IntDeque_push_back(deq, i);
    IntDeque_destroy(deq);
}

static void bench_cpp_deque_push_back(int N) {
    std::deque<int> deq;
    for (int i = 0; i < N; i++) deq.push_back(i);
}

static void bench_c_deque_push_front(int N) {
    IntDeque *deq = IntDeque_create();
    IntDeque_reserve(deq, N);
    for (int i = 0; i < N; i++) IntDeque_push_front(deq, i);
    IntDeque_destroy(deq);
}

static void bench_cpp_deque_push_front(int N) {
    std::deque<int> deq;
    for (int i = 0; i < N; i++) deq.push_front(i);
}

static void bench_c_deque_at(int N) {
    IntDeque *deq = IntDeque_create();
    IntDeque_reserve(deq, N);
    for (int i = 0; i < N; i++) IntDeque_push_back(deq, i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += IntDeque_at(deq, i);
    (void)dummy;
    IntDeque_destroy(deq);
}

static void bench_cpp_deque_at(int N) {
    std::deque<int> deq;
    for (int i = 0; i < N; i++) deq.push_back(i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += deq[i];
    (void)dummy;
}

/* ============================================================================
 * LinkedList Benchmarks
 * ============================================================================ */

static void bench_c_list_push_back(int N) {
    IntList *list = IntList_create();
    for (int i = 0; i < N; i++) IntList_push_back(list, i);
    IntList_destroy(list);
}

static void bench_cpp_list_push_back(int N) {
    std::list<int> list;
    for (int i = 0; i < N; i++) list.push_back(i);
}

static void bench_c_list_push_front(int N) {
    IntList *list = IntList_create();
    for (int i = 0; i < N; i++) IntList_push_front(list, i);
    IntList_destroy(list);
}

static void bench_cpp_list_push_front(int N) {
    std::list<int> list;
    for (int i = 0; i < N; i++) list.push_front(i);
}

/* ============================================================================
 * HashSet Benchmarks
 * ============================================================================ */

static void bench_c_hashset_insert(int N) {
    IntSet *set = IntSet_create();
    IntSet_reserve(set, N);
    for (int i = 0; i < N; i++) IntSet_insert(set, i);
    IntSet_destroy(set);
}

static void bench_cpp_unordered_set_insert(int N) {
    std::unordered_set<int> set;
    set.reserve(N);
    for (int i = 0; i < N; i++) set.insert(i);
}

static void bench_c_hashset_contains(int N) {
    IntSet *set = IntSet_create();
    IntSet_reserve(set, N);
    for (int i = 0; i < N; i++) IntSet_insert(set, i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += IntSet_contains(set, i) ? 1 : 0;
    (void)dummy;
    IntSet_destroy(set);
}

static void bench_cpp_unordered_set_contains(int N) {
    std::unordered_set<int> set;
    set.reserve(N);
    for (int i = 0; i < N; i++) set.insert(i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += (set.find(i) != set.end()) ? 1 : 0;
    (void)dummy;
}

/* ============================================================================
 * HashMap Benchmarks
 * ============================================================================ */

static void bench_c_hashmap_insert(int N) {
    IntIntMap *map = IntIntMap_create();
    IntIntMap_reserve(map, N);
    for (int i = 0; i < N; i++) IntIntMap_insert(map, i, i);
    IntIntMap_destroy(map);
}

static void bench_cpp_unordered_map_insert(int N) {
    std::unordered_map<int, int> map;
    map.reserve(N);
    for (int i = 0; i < N; i++) map[i] = i;
}

static void bench_c_hashmap_get(int N) {
    IntIntMap *map = IntIntMap_create();
    IntIntMap_reserve(map, N);
    for (int i = 0; i < N; i++) IntIntMap_insert(map, i, i);
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += IntIntMap_get(map, i);
    (void)dummy;
    IntIntMap_destroy(map);
}

static void bench_cpp_unordered_map_get(int N) {
    std::unordered_map<int, int> map;
    for (int i = 0; i < N; i++) map[i] = i;
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += map[i];
    (void)dummy;
}

/* ============================================================================
 * String Vector Benchmarks
 * ============================================================================ */

static void bench_c_string_vector_push(int N) {
    StringVector *vec = StringVector_create();
    StringVector_reserve(vec, N);
    for (int i = 0; i < N; i++) {
        StringVector_push(vec, random_string(16).c_str());
    }
    StringVector_destroy(vec);
}

static void bench_cpp_string_vector_push(int N) {
    std::vector<std::string> vec;
    vec.reserve(N);
    for (int i = 0; i < N; i++) vec.push_back(random_string(16));
}

static void bench_c_string_vector_at(int N) {
    StringVector *vec = StringVector_create();
    StringVector_reserve(vec, N);
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        StringVector_push(vec, keys.back().c_str());
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) {
        dummy += strlen(StringVector_at(vec, i));
    }
    (void)dummy;
    StringVector_destroy(vec);
}

static void bench_cpp_string_vector_at(int N) {
    std::vector<std::string> vec;
    vec.reserve(N);
    for (int i = 0; i < N; i++) vec.push_back(random_string(16));
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += vec[i].size();
    (void)dummy;
}

/* ============================================================================
 * String HashSet Benchmarks
 * ============================================================================ */

static void bench_c_string_set_insert(int N) {
    StringSet *set = StringSet_create();
    StringSet_reserve(set, N);
    for (int i = 0; i < N; i++) {
        StringSet_insert(set, random_string(16).c_str());
    }
    StringSet_destroy(set);
}

static void bench_cpp_string_unordered_set_insert(int N) {
    std::unordered_set<std::string> set;
    set.reserve(N);
    for (int i = 0; i < N; i++) set.insert(random_string(16));
}

static void bench_c_string_set_contains(int N) {
    StringSet *set = StringSet_create();
    StringSet_reserve(set, N);
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        StringSet_insert(set, keys.back().c_str());
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) {
        dummy += StringSet_contains(set, keys[i].c_str()) ? 1 : 0;
    }
    (void)dummy;
    StringSet_destroy(set);
}

static void bench_cpp_string_unordered_set_contains(int N) {
    std::unordered_set<std::string> set;
    set.reserve(N);
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        set.insert(keys.back());
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) {
        dummy += (set.find(keys[i]) != set.end()) ? 1 : 0;
    }
    (void)dummy;
}

/* ============================================================================
 * String->Int HashMap Benchmarks
 * ============================================================================ */

static void bench_c_str_int_map_insert(int N) {
    StrIntMap *map = StrIntMap_create();
    StrIntMap_reserve(map, N);
    for (int i = 0; i < N; i++) {
        StrIntMap_insert(map, random_string(16).c_str(), i);
    }
    StrIntMap_destroy(map);
}

static void bench_cpp_str_int_unordered_map_insert(int N) {
    std::unordered_map<std::string, int> map;
    map.reserve(N);
    for (int i = 0; i < N; i++) map[random_string(16)] = i;
}

static void bench_c_str_int_map_get(int N) {
    StrIntMap *map = StrIntMap_create();
    StrIntMap_reserve(map, N);
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        StrIntMap_insert(map, keys.back().c_str(), i);
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += StrIntMap_get(map, keys[i].c_str());
    (void)dummy;
    StrIntMap_destroy(map);
}

static void bench_cpp_str_int_unordered_map_get(int N) {
    std::unordered_map<std::string, int> map;
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        map[keys.back()] = i;
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += map[keys[i]];
    (void)dummy;
}

/* ============================================================================
 * String->String HashMap Benchmarks
 * ============================================================================ */

static void bench_c_str_str_map_insert(int N) {
    StrStrMap *map = StrStrMap_create();
    StrStrMap_reserve(map, N);
    for (int i = 0; i < N; i++) {
        std::string k = random_string(16);
        std::string v = random_string(16);
        StrStrMap_insert(map, k.c_str(), v.c_str());
    }
    StrStrMap_destroy(map);
}

static void bench_cpp_str_str_unordered_map_insert(int N) {
    std::unordered_map<std::string, std::string> map;
    map.reserve(N);
    for (int i = 0; i < N; i++) map[random_string(16)] = random_string(16);
}

static void bench_c_str_str_map_get(int N) {
    StrStrMap *map = StrStrMap_create();
    StrStrMap_reserve(map, N);
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        std::string v = random_string(16);
        StrStrMap_insert(map, keys.back().c_str(), v.c_str());
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) {
        dummy += strlen(StrStrMap_get(map, keys[i].c_str()));
    }
    (void)dummy;
    StrStrMap_destroy(map);
}

static void bench_cpp_str_str_unordered_map_get(int N) {
    std::unordered_map<std::string, std::string> map;
    std::vector<std::string> keys;
    for (int i = 0; i < N; i++) {
        keys.push_back(random_string(16));
        map[keys.back()] = random_string(16);
    }
    volatile int dummy = 0;
    for (int i = 0; i < N; i++) dummy += map[keys[i]].size();
    (void)dummy;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main() {
    const int N_VECTOR = 500000;
    const int N_DEQUE = 500000;
    const int N_LIST = 500000;
    const int N_HASH = 100000;
    const int N_SMALL = 10000;
    const int N_STRING = 100000;

    std::cout << "\n";
    std::cout << "libcontain (C) vs C++ STL Performance Benchmark\n";
    std::cout << "\n";
    
    /* Vector */
    measure_and_record("vector_push", N_VECTOR, 
        [&](){ bench_c_vector_push(N_VECTOR); },
        [&](){ bench_cpp_vector_push(N_VECTOR); });
    
    measure_and_record("vector_at", N_VECTOR,
        [&](){ bench_c_vector_at(N_VECTOR); },
        [&](){ bench_cpp_vector_at(N_VECTOR); });
    
    measure_and_record("insert_front", N_SMALL,
        [&](){ bench_c_vector_insert_front(N_SMALL); },
        [&](){ bench_cpp_vector_insert_front(N_SMALL); });
    
    /* Deque */
    measure_and_record("deque_push_back", N_DEQUE,
        [&](){ bench_c_deque_push_back(N_DEQUE); },
        [&](){ bench_cpp_deque_push_back(N_DEQUE); });
    
    measure_and_record("deque_push_front", N_DEQUE,
        [&](){ bench_c_deque_push_front(N_DEQUE); },
        [&](){ bench_cpp_deque_push_front(N_DEQUE); });
    
    measure_and_record("deque_at", N_DEQUE,
        [&](){ bench_c_deque_at(N_DEQUE); },
        [&](){ bench_cpp_deque_at(N_DEQUE); });
    
    /* LinkedList */
    measure_and_record("list_push_back", N_LIST,
        [&](){ bench_c_list_push_back(N_LIST); },
        [&](){ bench_cpp_list_push_back(N_LIST); });
    
    measure_and_record("list_push_front", N_LIST,
        [&](){ bench_c_list_push_front(N_LIST); },
        [&](){ bench_cpp_list_push_front(N_LIST); });
    
    /* HashSet */
    measure_and_record("hashset_insert", N_HASH,
        [&](){ bench_c_hashset_insert(N_HASH); },
        [&](){ bench_cpp_unordered_set_insert(N_HASH); });
    
    measure_and_record("hashset_contains", N_HASH,
        [&](){ bench_c_hashset_contains(N_HASH); },
        [&](){ bench_cpp_unordered_set_contains(N_HASH); });
    
    /* HashMap */
    measure_and_record("hashmap_insert", N_HASH,
        [&](){ bench_c_hashmap_insert(N_HASH); },
        [&](){ bench_cpp_unordered_map_insert(N_HASH); });
    
    measure_and_record("hashmap_get", N_HASH,
        [&](){ bench_c_hashmap_get(N_HASH); },
        [&](){ bench_cpp_unordered_map_get(N_HASH); });
    
    /* String Vector */
    measure_and_record("string_vector_push", N_STRING,
        [&](){ bench_c_string_vector_push(N_STRING); },
        [&](){ bench_cpp_string_vector_push(N_STRING); });
    
    measure_and_record("string_vector_at", N_STRING,
        [&](){ bench_c_string_vector_at(N_STRING); },
        [&](){ bench_cpp_string_vector_at(N_STRING); });
    
    /* String HashSet */
    measure_and_record("string_set_insert", N_STRING,
        [&](){ bench_c_string_set_insert(N_STRING); },
        [&](){ bench_cpp_string_unordered_set_insert(N_STRING); });
    
    measure_and_record("string_set_contains", N_STRING,
        [&](){ bench_c_string_set_contains(N_STRING); },
        [&](){ bench_cpp_string_unordered_set_contains(N_STRING); });
    
    /* String->Int HashMap */
    measure_and_record("str_int_map_insert", N_STRING,
        [&](){ bench_c_str_int_map_insert(N_STRING); },
        [&](){ bench_cpp_str_int_unordered_map_insert(N_STRING); });
    
    measure_and_record("str_int_map_get", N_STRING,
        [&](){ bench_c_str_int_map_get(N_STRING); },
        [&](){ bench_cpp_str_int_unordered_map_get(N_STRING); });
    
    /* String->String HashMap */
    measure_and_record("str_str_map_insert", N_STRING,
        [&](){ bench_c_str_str_map_insert(N_STRING); },
        [&](){ bench_cpp_str_str_unordered_map_insert(N_STRING); });
    
    measure_and_record("str_str_map_get", N_STRING,
        [&](){ bench_c_str_str_map_get(N_STRING); },
        [&](){ bench_cpp_str_str_unordered_map_get(N_STRING); });
    
    /* Print results table with percentage */
    std::cout << "+--------------------------+----------+-----------+-----------+----------+----------+\n";
    std::cout << "| Operation                | N        | C (ms)    | C++ (ms)  | Winner   | Diff     |\n";
    std::cout << "+--------------------------+----------+-----------+-----------+----------+----------+\n";

    for (const auto& r : results) {
        double diff = (r.cpp_avg - r.c_avg) / r.c_avg * 100.0;
        std::cout << "| " << std::setw(24) << std::left << r.name 
                << " | " << std::setw(8) << r.n 
                << " | " << std::setw(9) << std::right << std::fixed << std::setprecision(2) << r.c_avg
                << " | " << std::setw(9) << r.cpp_avg
                << " | " << std::setw(6) << std::right << r.winner 
                << "   | " << std::setw(7) << std::showpos << std::setprecision(1) << diff << std::noshowpos << "% |\n";
    }

    std::cout << "+--------------------------+----------+-----------+-----------+----------+----------+\n";

    int c_count = 0, cpp_count = 0;
    for (const auto& r : results) {
        if (std::string(r.winner) == "C") c_count++;
        else cpp_count++;
    }

    std::cout << "| TOTAL WINS               |          |           |           | C " 
          << std::setw(2) << c_count << "     | C++ " << std::setw(2) << cpp_count << "   |\n";
    std::cout << "+--------------------------+----------+-----------+-----------+----------+----------+\n";

    std::cout << "\n";

    return 0;
}