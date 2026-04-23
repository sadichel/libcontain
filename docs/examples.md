# Examples

Real-world working examples demonstrating libcontain features.

---

## Word Frequency Analysis

```c
#define HASHMAP_IMPLEMENTATION
#include <contain/hashmap.h>
#include <contain/typed.h>

DECL_HASHMAP_TYPE(const char*, int, 0, sizeof(int), WordCount)

static bool is_not_stopword(const Container *ctx, const void *item) {
    (void)ctx;
    const char *word = (const char *)item;
    static const char *stop[] = {"the", "and", "of", "to", "a", "in", "for"};
    for (size_t i = 0; i < sizeof(stop)/sizeof(stop[0]); i++) {
        if (strcmp(word, stop[i]) == 0) return false;
    }
    return true;
}

static void *count_word(const Container *ctx, const void *item, void *acc) {
    (void)ctx;
    WordCount *map = (WordCount *)acc;
    const char *word = (const char *)item;
    int current = WordCount_get_or_default(map, word, 0);
    WordCount_insert(map, word, current + 1);
    return acc;
}

int main(void) {
    /* Tokenize text into vector */
    Vector *tokens = vector_str();
    const char *text = "the quick brown fox jumps over the lazy dog";
    char word[64];
    size_t len = 0;
    for (const char *p = text; *p; p++) {
        if (isalpha(*p)) {
            word[len++] = tolower(*p);
        } else if (len > 0) {
            word[len] = '\0';
            vector_push(tokens, word);
            len = 0;
        }
    }
    if (len > 0) vector_push(tokens, word);

    /* Count frequencies */
    WordCount *freq = WordCount_create();
    Iterator *it = HeapIter((Container *)tokens);
    it = iter_filter(it, is_not_stopword);
    iter_fold(it, freq, count_word);

    /* Print results */
    Iterator it2 = WordCount_iter(freq);
    const void *entry;
    while ((entry = iter_next(&it2))) {
        printf("%s: %d\n",
               WordCount_entry_key(freq, entry),
               WordCount_entry_value(freq, entry));
    }

    WordCount_destroy(freq);
    vector_destroy(tokens);
    return 0;
}
```

---

## Pool Allocator

```c
#define LINKEDLIST_IMPLEMENTATION
#include <contain/linkedlist.h>

/* Create pool allocator for 32-byte entries, 128 per block, 8-byte alignment */
Allocator *pool = pool_allocator_create(32, 128, 8, GROW_GOLDEN);

/* Use with linked list */
LinkedList *list = linkedlist_create_with_allocator(sizeof(MyStruct), pool);

/* Use with hash set */
HashSet *set = hashset_create_with_allocator(sizeof(int), pool);

/* Cleanup */
linkedlist_destroy(list);
hashset_destroy(set);
allocator_destroy(pool);
```

---

## SIMD Aligned Vector

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
#include <immintrin.h>

/* 32-byte aligned vector for AVX operations */
Vector *vec = vector_create_aligned(sizeof(__m256), 32);

__m256 data = _mm256_set1_ps(3.14f);
vector_push(vec, &data);

const __m256 *ptr = vector_front(vec);
__m256 result = _mm256_add_ps(ptr[0], ptr[1]);

vector_destroy(vec);
```

---

## Cache-Line Friendly Thread Counters

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>

/* 64-byte alignment to prevent false sharing between threads */
typedef struct {
    atomic_int counter;
    char padding[64 - sizeof(atomic_int)];
} alignas(64) CacheLineAligned;

Vector *vec = vector_create_aligned(sizeof(CacheLineAligned), 64);

CacheLineAligned val = {0};
vector_push(vec, &val);
```

---

## Iterator Pipeline

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
#include <contain/iterator.h>

/* Filter even numbers */
static bool is_even(const Container *ctx, const void *item) {
    (void)ctx;
    return *(const int *)item % 2 == 0;
}

/* Double integer value */
static void *double_int(const Container *ctx, const void *item, void *buf) {
    (void)ctx;
    *(int *)buf = *(const int *)item * 2;
    return buf;
}

int main(void) {
    Vector *vec = create_int_range(0, 1000);

    /* Build iterator pipeline */
    Iterator *it = HeapIter((Container *)vec);
    it = iter_filter(it, is_even);
    it = iter_map(it, double_int, sizeof(int));
    it = iter_take(it, 100);

    /* Execute pipeline and collect results */
    Vector *result = (Vector *)iter_collect(it);

    printf("Collected %zu elements\n", vector_len(result));

    vector_destroy(result);
    vector_destroy(vec);
    return 0;
}
```

---

## Pipeline Reuse with Chainer

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
#include <contain/chainer.h>

/* Build pipeline once */
Chainer *c = Chain((Container *)vec1);
chain_filter(c, is_even);
chain_map(c, double_int, sizeof(int));
chain_take(c, 100);

/* Run on multiple datasets */
size_t n1 = chain_count(c);
chain_bind(c, (Container *)vec2);
size_t n2 = chain_count(c);
chain_bind(c, (Container *)vec3);
size_t n3 = chain_count(c);

chain_destroy(c);
```

---

## Flatten Nested Containers

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
#include <contain/chainer.h>
#include <contain/typed.h>

DECL_VECTOR_TYPE(int, sizeof(int), IntVector)

/* Create vector of vectors */
Vector *outer = vector_create(sizeof(Vector*));
for (int i = 0; i < 10; i++) {
    Vector *inner = vector_create(sizeof(int));
    for (int j = 0; j < 100; j++) {
        int val = i * 100 + j;
        vector_push(inner, &val);
    }
    vector_push(outer, &inner);
}

/* Flatten and process */
Chainer *c = Chain((Container *)outer);
chain_flatten(c);
chain_filter(c, is_even);
chain_take(c, 500);

/* Collect into IntVector (correct type for int values) */
IntVector *result = IntVector_create();
size_t count = chain_collect_in(c, (Container *)result);

chain_destroy(c);

printf("Collected %zu ints\n", count);
for (size_t i = 0; i < IntVector_len(result); i++) {
    printf("%d ", IntVector_at(result, i));
}
printf("\n");

/* Cleanup */
for (size_t i = 0; i < vector_len(outer); i++) {
    Vector *inner = *(Vector **)vector_at(outer, i);
    vector_destroy(inner);
}
vector_destroy(outer);
IntVector_destroy(result);
```

---

## Zip Two Vectors

```c
#define VECTOR_IMPLEMENTATION
#include <contain/vector.h>
#include <contain/chainer.h>
#include <contain/typed.h>

typedef struct { int a; int b; } Pair;

DECL_VECTOR_TYPE(Pair, sizeof(Pair), PairVector);

void *merge_pairs(const Container *ca, const void *a,
                  const Container *cb, const void *b,
                  void *buf) {
    ((Pair *)buf)->a = *(const int *)a;
    ((Pair *)buf)->b = *(const int *)b;
    return buf;
}

IntVector *va = IntVector_create();
IntVector *vb = IntVector_create();
for (int i = 0; i < 10; i++) {
    IntVector_push(va, i);
    IntVector_push(vb, i * 2);
}

Chainer *c = Chain((Container *)va));
chain_zip(c, (Container *)vb), merge_pairs, sizeof(Pair));

PairVector *pairs = PairVector_create();
chain_collect_in(c, (Container *)pairs);

chain_destroy(c);
PairVector_destroy(pairs);
IntVector_destroy(va);
IntVector_destroy(vb);
```

---