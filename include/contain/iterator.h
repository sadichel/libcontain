/** @file iterator.h
 * @brief Iterator Decorator Pattern and Terminal Operations for Container Iteration
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * This header provides a rich set of iterator decorators and terminal
 * operations that work with the Container/Iterator protocol defined in
 * container.h. It enables functional-style pipelines for processing
 * container elements without materializing intermediate results.
 *
 * @section key_concepts Key Concepts
 *   - @ref decorator_pattern — each decorator wraps an inner iterator and
 *     transforms, filters, or limits the elements it yields
 *   - @ref ownership_transfer — decorators take ownership of their inner
 *     iterator and destroy it when they are themselves destroyed
 *   - @ref lazy_evaluation — transformations are applied on-demand during
 *     iteration; no intermediate containers are allocated
 *   - @ref terminal_ops — consuming functions that exhaust an iterator
 *     and produce a final result (count, collect, fold, etc.)
 *
 * @section pipeline_example Iterator Pipeline Example
 * @code
 *   // Filter even numbers, double them, take first 10, collect into vector
 *   Iterator *it = iter_take(
 *       iter_map(
 *           iter_filter(HeapIter(vec), is_even),
 *           double_int, sizeof(int)),
 *       10);
 *   Container *result = iter_collect(it);
 *
 *   // Linear usage
 *   Iterator *it = HeapIter(vec);
 *   it = iter_filter(it, is_even);
 *   it = iter_map(it, double_int, sizeof(int));
 *   it = iter_take(it, 10);
 *   Container *result = iter_collect(it);
 *
 *   // it and all intermediate iterators are destroyed by iter_collect
 * @endcode
 *
 * @section memory_management Memory Management
 *   - Decorator constructors (iter_filter, iter_map, etc.) take ownership
 *     of their inner iterator — do not use the inner iterator after
 *     passing it to a decorator
 *   - Terminal operations (iter_count, iter_collect, etc.) destroy the
 *     iterator they consume — do not use the iterator after passing it
 *     to a terminal
 *   - iter_drop is the exception — it does NOT take ownership
 *
 * @section usage Usage
 * @code
 *   #include "container.h"
 *   #include "iterator.h"
 *
 *   // Use HeapIter() to create a heap-allocated iterator for decorators
 *   Iterator *it = iter_filter(HeapIter(vec), predicate);
 *   const void *e;
 *   while ((e = iter_next(it))) { process(e); }
 *   iter_destroy(it);
 * @endcode
 */

#ifndef ITERATOR_PDR_H
#define ITERATOR_PDR_H

#include "container.h"

/* ============================================================================
 * Section: Decorator Iterators
 * ============================================================================
 *
 * Decorator iterators wrap an inner iterator and transform the sequence
 * of elements it produces. Each decorator:
 *
 *   1. Takes ownership of its inner iterator
 *   2. Propagates the container reference from inner to outer
 *   3. Implements the IteratorVTable interface (next, destroy)
 *   4. Recursively destroys inner iterators when destroyed
 *
 * Decorators can be chained to build complex pipelines:
 *   iter_map(iter_filter(iter_skip(base_iter, 5), pred), transform, size)
 *
 * The order of decorators matters — they are applied from innermost
 * to outermost as elements flow through the pipeline.
 *
 * Design Note: Container Parameter in Callbacks
 * ------------------------------------------------
 * All predicates and transform functions receive a `const Container *`
 * as their first parameter. This design choice provides several benefits:
 *
 *   • Hashmap Support — When iterating over hashmap values, the container
 *     parameter allows callbacks to access the associated keys or other
 *     container-specific metadata.
 *
 *   • Context-Aware Filtering — Predicates can make decisions based on
 *     container state (e.g., size, capacity, or custom attributes).
 *
 *   • API Consistency — Aligns with the Container/Iterator protocol in
 *     container.h where all operations have access to the container context.
 *
 * For simple use cases where the container is not needed, the parameter
 * can be safely ignored (compilers may warn about unused parameters).
 */

/**
 * @defgroup decorators Iterator Decorators
 * @brief Lazy transformation decorators for iterators
 *
 * Decorator iterators wrap an inner iterator and transform the sequence
 * of elements it produces. They are lazy — no work is done until
 * iter_next() is called.
 * @{
 */

/**
 * @defgroup filter Filter Decorator
 * @brief Skip elements that don't satisfy a predicate
 * @{
 */

/**
 * @struct FilterIter
 * @brief Internal structure for filter decorator (opaque)
 */
typedef struct {
    Iterator base;                      /**< Base iterator interface */
    Iterator *inner;                    /**< Inner iterator being filtered */
    bool (*pred)(const Container *ctx, const void *item); /**< Predicate function */
} FilterIter;

/** @cond INTERNAL */
static const void *_iter_filter_next(Iterator *it) {
    FilterIter *f = (FilterIter *)it;
    const void *entry;
    while ((entry = iter_next(f->inner))) {
        if (f->pred(f->inner->container, entry)) return entry;
    }
    return NULL;
}

static void _iter_filter_destroy(Iterator *it) {
    FilterIter *f = (FilterIter *)it;
    iter_destroy(f->inner);
    free(f);
}

static const IteratorVTable ITER_FILTER_OPS = {
    .next = _iter_filter_next,
    .destroy = _iter_filter_destroy
};
/** @endcond */

/**
 * @brief Create a filter iterator that skips elements not satisfying a predicate
 *
 * Wraps `inner` and skips any element for which `pred` returns false.
 * Only elements where pred(container, element) returns true are yielded.
 *
 * @param inner Source iterator (ownership taken)
 * @param pred  Predicate function; receives the source container and the
 *              current element pointer, returns true to include the element.
 *              If NULL, acts as a pass-through (no filtering).
 *
 * @return A new heap-allocated iterator that yields the filtered subset,
 *         or NULL if inner is NULL or allocation fails.
 *
 * @note Takes ownership of `inner` — inner is destroyed when the returned
 *       iterator is destroyed (either explicitly via iter_destroy or by
 *       a terminal operation).
 *
 * @par Example
 * @code
 *   bool is_even(const Container *c, const void *e) {
 *       return *(const int *)e % 2 == 0;
 *   }
 *   Iterator *it = iter_filter(HeapIter(vec), is_even);
 *   const int *e;
 *   while ((e = iter_next(it))) { printf("%d\n", *e); }
 *   iter_destroy(it);
 * @endcode
 *
 * @note The predicate receives the original source container, not a decorator
 *       wrapper, allowing predicates to inspect container metadata if needed.
 */
static inline Iterator *iter_filter(Iterator *inner, bool (*pred)(const Container *, const void *)) {
    if (!inner) return NULL;
    
    /* NULL predicate means pass-through (no filtering) */
    if (!pred) {
        return inner;  
    }
    
    FilterIter *f = (FilterIter *)malloc(sizeof(FilterIter));
    if (!f) {
        iter_destroy(inner);
        return NULL;
    }

    f->base = (Iterator){
        .container = inner->container,
        .ops = &ITER_FILTER_OPS
    };
    f->inner = inner;
    f->pred = pred;
    return &f->base;
}

/** @} */ /* end of filter group */

/**
 * @defgroup map Map Decorator
 * @brief Transform each element using a mapping function
 * @{
 */

/**
 * @struct MapIter
 * @brief Internal structure for map decorator (opaque)
 */
typedef struct {
    Iterator base;                      /**< Base iterator interface */
    Iterator *inner;                    /**< Inner iterator being transformed */
    void *(*map)(const Container *ctx, const void *item, void *buf); /**< Map function */
    void *buffer;                       /**< Internal buffer for transformed element */
} MapIter;

/** @cond INTERNAL */
static const void *_iter_map_next(Iterator *it) {
    MapIter *m = (MapIter *)it;
    const void *entry = iter_next(m->inner);
    return entry ? m->map(m->inner->container, entry, m->buffer) : NULL;
}

static void _iter_map_destroy(Iterator *it) {
    MapIter *m = (MapIter *)it;
    iter_destroy(m->inner);
    free(m->buffer);
    free(m);
}

static const IteratorVTable ITER_MAP_OPS = {
    .next = _iter_map_next,
    .destroy = _iter_map_destroy
};
/** @endcond */

/**
 * @brief Create a map iterator that transforms each element
 *
 * Transforms every element yielded by `inner` by passing it through a
 * user-supplied map function. The result is written into an internally
 * managed buffer of `stride` bytes and a pointer to that buffer is returned.
 *
 * @param inner  Source iterator (ownership taken)
 * @param map    Transform function; receives the source container, the current
 *               element pointer, and the output buffer. Must write the result
 *               into buf and return buf, or return NULL to signal early
 *               termination. If NULL, acts as a pass-through.
 * @param stride Size in bytes of the output element type. Must be greater than zero.
 *
 * @return A new heap-allocated iterator that yields transformed elements,
 *         or NULL if inner is NULL, stride is 0, or allocation fails.
 *
 * @note Takes ownership of `inner` — inner is destroyed when the returned
 *       iterator is destroyed.
 *
 * @note **String Transformations:** When mapping string containers
 *       (item_size == 0), the `item` parameter received by your map function
 *       is a `const char*` (the actual string). The `buf` parameter points to
 *       a buffer of `stride` bytes. Your map function must copy the transformed
 *       string into `buf` (ensuring it fits within stride-1 bytes plus null
 *       terminator) and return `buf`. For example:
 *       @code
 *       void *str_to_upper(const Container *ctx, const void *item, void *buf) {
 *           const char *str = (const char *)item;
 *           char *out = (char *)buf;
 *           strcpy(out, str);  // Caller ensured stride is large enough
 *           for (char *p = out; *p; p++) *p = toupper(*p);
 *           return buf;
 *       }
 *       // Use with stride = max_expected_string_length + 1
 *       Iterator *it = iter_map(HeapIter(str_vec), str_to_upper, 256);
 *       @endcode
 *
 * @warning The caller must consume the result before the next call to iter_next,
 *          as the internal buffer is reused on every iteration. Do not store
 *          pointers returned by iter_next across multiple calls.
 *
 * @warning For string transformations, ensure `stride` is large enough for the
 *          longest possible output string plus null terminator. Output longer
 *          than stride-1 will cause buffer overflow.
 *
 * @par Example (fixed-size)
 * @code
 *   void *double_int(const Container *c, const void *e, void *buf) {
 *       *(int *)buf = *(const int *)e * 2;
 *       return buf;
 *   }
 *   Iterator *it = iter_map(HeapIter(vec), double_int, sizeof(int));
 *   const int *e;
 *   while ((e = iter_next(it))) { printf("%d\n", *e); }
 *   iter_destroy(it);
 * @endcode
 */
static inline Iterator *iter_map(Iterator *inner, void *(*map)(const Container *, const void *, void *),
                                 size_t stride) {
    if (!inner) return NULL;
    
    /* NULL mapper means pass-through (no transformation) */
    if (!map) {
        return inner;  
    }
    
    if (stride == 0) {
        iter_destroy(inner);
        return NULL;
    }
    
    MapIter *m = (MapIter *)malloc(sizeof(MapIter));
    void *buf = malloc(stride);
    if (!m || !buf) {
        free(m);
        free(buf);
        iter_destroy(inner);
        return NULL;
    }
    m->base = (Iterator){
        .container = inner->container,
        .ops = &ITER_MAP_OPS
    };
    m->inner = inner;
    m->map = map;
    m->buffer = buf;
    return &m->base;
}

/** @} */ /* end of map group */

/**
 * @defgroup skip Skip Decorator
 * @brief Discard the first N elements
 * @{
 */

/**
 * @struct SkipIter
 * @brief Internal structure for skip decorator (opaque)
 */
typedef struct {
    Iterator base;          /**< Base iterator interface */
    Iterator *inner;        /**< Inner iterator being skipped */
    size_t skip;            /**< Number of elements remaining to skip */
} SkipIter;

/** @cond INTERNAL */
static const void *_iter_skip_next(Iterator *it) {
    SkipIter *s = (SkipIter *)it;

    while (s->skip > 0) {
        if (!iter_next(s->inner)) return NULL;
        s->skip--;
    }

    return iter_next(s->inner);
}

static void _iter_skip_destroy(Iterator *it) {
    SkipIter *s = (SkipIter *)it;
    iter_destroy(s->inner);
    free(s);
}

static const IteratorVTable ITER_SKIP_OPS = {
    .next = _iter_skip_next,
    .destroy = _iter_skip_destroy
};
/** @endcond */

/**
 * @brief Create a skip iterator that discards the first N elements
 *
 * Discards the first n elements from `inner`, then yields the rest
 * unchanged. If inner has fewer than n elements the returned iterator
 * is immediately exhausted.
 *
 * @param inner Source iterator (ownership taken)
 * @param n     Number of leading elements to discard
 *
 * @return A new heap-allocated iterator starting after the skipped prefix,
 *         or NULL if inner is NULL or allocation fails.
 *
 * @note Takes ownership of `inner` — inner is destroyed when the returned
 *       iterator is destroyed.
 *
 * @note The skip is deferred — no elements are consumed at construction time.
 *       The n elements are drained on the first call(s) to iter_next.
 *
 * @par Example
 * @code
 *   Iterator *it = iter_skip(HeapIter(vec), 3);
 *   const void *e;
 *   while ((e = iter_next(it))) { process(e); }
 *   iter_destroy(it);
 * @endcode
 */
static inline Iterator *iter_skip(Iterator *inner, size_t skip) {
    if (!inner) return NULL;
    SkipIter *s = (SkipIter *)malloc(sizeof(SkipIter));
    if (!s) {
        iter_destroy(inner);
        return NULL;
    }
    s->base = (Iterator){
        .container = inner->container,
        .ops = &ITER_SKIP_OPS
    };
    s->inner = inner;
    s->skip = skip;
    return &s->base;
}

/** @} */ /* end of skip group */

/**
 * @defgroup take Take Decorator
 * @brief Limit iteration to at most N elements
 * @{
 */

/**
 * @struct TakeIter
 * @brief Internal structure for take decorator (opaque)
 */
typedef struct {
    Iterator base;          /**< Base iterator interface */
    Iterator *inner;        /**< Inner iterator being limited */
    size_t take;            /**< Number of elements remaining to yield */
} TakeIter;

/** @cond INTERNAL */
static const void *_iter_take_next(Iterator *it) {
    TakeIter *t = (TakeIter *)it;
    if (t->take == 0) return NULL;
    const void *val = iter_next(t->inner);
    if (!val) return NULL;
    t->take--;
    return val;
}

static void _iter_take_destroy(Iterator *it) {
    TakeIter *t = (TakeIter *)it;
    iter_destroy(t->inner);
    free(t);
}

static const IteratorVTable ITER_TAKE_OPS = {
    .next = _iter_take_next,
    .destroy = _iter_take_destroy
};
/** @endcond */

/**
 * @brief Create a take iterator that yields at most N elements
 *
 * Yields at most n elements from `inner`, then stops. The inner iterator
 * is left in whatever state it was after the nth element was taken.
 *
 * @param inner Source iterator (ownership taken)
 * @param n     Maximum number of elements to yield
 *
 * @return A new heap-allocated iterator that yields at most n elements,
 *         or NULL if inner is NULL or allocation fails.
 *
 * @note Takes ownership of `inner` — inner is destroyed when the returned
 *       iterator is destroyed, even if not all n elements were consumed.
 *
 * @par Example
 * @code
 *   Iterator *it = iter_take(HeapIter(vec), 5);
 *   const void *e;
 *   while ((e = iter_next(it))) { process(e); }
 *   iter_destroy(it);
 * @endcode
 *
 * @note If inner has fewer than n elements, iter_take yields all of them
 *       and then returns NULL. The inner iterator is still properly destroyed.
 */
static inline Iterator *iter_take(Iterator *inner, size_t take) {
    if (!inner) return NULL;
    TakeIter *t = (TakeIter *)malloc(sizeof(TakeIter));
    if (!t) {
        iter_destroy(inner);
        return NULL;
    }
    t->base = (Iterator){
        .container = inner->container,
        .ops = &ITER_TAKE_OPS
    };
    t->inner = inner;
    t->take = take;
    return &t->base;
}

/** @} */ /* end of take group */

/**
 * @defgroup flatten Flatten Decorator
 * @brief Flatten a sequence of containers into a single sequence
 * @{
 */

/**
 * @struct FlattenIter
 * @brief Internal structure for flatten decorator (opaque)
 */
typedef struct {
    Iterator base;          /**< Base iterator interface */
    Iterator *outer;        /**< Iterator over Container* elements */
    Iterator inner;         /**< Current inner iterator (stack-allocated) */
    bool inner_active;      /**< Whether inner iterator is active */
} FlattenIter;

/** @cond INTERNAL */
static const void *_iter_flatten_next(Iterator *it) {
    FlattenIter *f = (FlattenIter *)it;

    for (;;) {
        if (f->inner_active) {
            const void *entry = iter_next(&f->inner);
            if (entry) return entry;
            f->inner_active = false;
        }

        const void *raw = iter_next(f->outer);
        if (!raw) return NULL;

        Container *inner_c = *(Container **)raw;
        if (!inner_c) continue;

        f->inner = Iter(inner_c);
        f->inner_active = true;
    }
}

static void _iter_flatten_destroy(Iterator *it) {
    FlattenIter *f = (FlattenIter *)it;
    iter_destroy(f->outer);
    free(f);
}

static const IteratorVTable ITER_FLATTEN_OPS = {
    .next = _iter_flatten_next,
    .destroy = _iter_flatten_destroy
};
/** @endcond */

/**
 * @brief Create a flatten iterator that concatenates sequences
 *
 * Flattens a two-level sequence. The outer iterator must yield elements
 * of type Container * (i.e. pointers to containers). iter_flatten walks
 * each inner container in sequence, yielding every element of each one
 * before moving to the next.
 *
 * @param outer Iterator over Container* elements (ownership taken)
 *
 * @return A new heap-allocated iterator that yields elements from each inner
 *         container consecutively, or NULL if outer is NULL or allocation fails.
 *
 * @note Takes ownership of `outer` — outer is destroyed when the returned
 *       iterator is destroyed.
 *
 * @note The inner containers themselves are NOT owned or destroyed — only
 *       the outer iterator is managed.
 *
 * @note The inner containers are iterated with stack-allocated base Iterators
 *       — no heap allocation per inner container. NULL inner containers are
 *       silently skipped.
 *
 * @par Example
 * @code
 *   // vec contains Container* pointers to multiple HashSet instances
 *   Iterator *it = iter_flatten(HeapIter(vec));
 *   const int *e;
 *   while ((e = iter_next(it))) { printf("%d\n", *e); }
 *   iter_destroy(it);
 * @endcode
 */
static inline Iterator *iter_flatten(Iterator *outer) {
    if (!outer) return NULL;
    FlattenIter *f = (FlattenIter *)malloc(sizeof(FlattenIter));
    if (!f) {
        iter_destroy(outer);
        return NULL;
    }

    f->base = (Iterator){
        .container = outer->container,
        .ops = &ITER_FLATTEN_OPS
    };
    f->outer = outer;
    f->inner_active = false;
    return &f->base;
}

/** @} */ /* end of flatten group */

/**
 * @defgroup zip Zip Decorator
 * @brief Pair elements from two iterators
 * @{
 */

/**
 * @struct ZipIter
 * @brief Internal structure for zip decorator (opaque)
 */
typedef struct {
    Iterator base;          /**< Base iterator interface */
    Iterator *left;         /**< Left source iterator */
    Iterator *right;        /**< Right source iterator */
    void *(*merge)(const Container *ca, const void *a, const Container *cb, const void *b, void *buf);
    void *buffer;           /**< Internal buffer for merged result */
} ZipIter;

/** @cond INTERNAL */
static const void *_iter_zip_next(Iterator *it) {
    ZipIter *z = (ZipIter *)it;
    const void *a = iter_next(z->left);
    const void *b = iter_next(z->right);
    if (!a || !b) return NULL;
    return z->merge(z->left->container, a, z->right->container, b, z->buffer);
}

static void _iter_zip_destroy(Iterator *it) {
    ZipIter *z = (ZipIter *)it;
    iter_destroy(z->left);
    iter_destroy(z->right);
    free(z->buffer);
    free(z);
}

static const IteratorVTable ITER_ZIP_OPS = {
    .next = _iter_zip_next,
    .destroy = _iter_zip_destroy
};
/** @endcond */

/**
 * @brief Create a zip iterator that pairs elements from two iterators
 *
 * Pairs one element from each of two independent iterators per step and
 * passes both to a user-supplied merge function that combines them into
 * a single result. The result is written into an internally managed buffer
 * of `stride` bytes, and a pointer to that buffer is returned.
 *
 * @param left   First source iterator (ownership taken)
 * @param right  Second source iterator (ownership taken)
 * @param merge  Combines one element from each source. Receives:
 *               - ca: container of left
 *               - a: current element from left
 *               - cb: container of right
 *               - b: current element from right
 *               - buf: output buffer (stride bytes)
 *               Must write the result into buf and return buf, or return NULL
 *               to stop iteration early.
 * @param stride Size in bytes of the merged output type. Must be greater than zero.
 *
 * @return A new heap-allocated iterator that yields merged pairs, or NULL
 *         if any parameter is invalid or allocation fails.
 *
 * @note Takes ownership of both `left` and `right` — both are destroyed when
 *       the returned iterator is destroyed.
 *
 * @note Iteration stops as soon as either source is exhausted or merge returns
 *       NULL. The shorter iterator determines the total number of pairs.
 *
 * @warning The caller must consume the result before the next call to iter_next,
 *          as the internal buffer is reused on every iteration.
 *
 * @par Example
 * @code
 *   typedef struct { int a, b; } Pair;
 *   void *make_pair(const Container *ca, const void *a,
 *                   const Container *cb, const void *b,
 *                   void *buf) {
 *       ((Pair *)buf)->a = *(const int *)a;
 *       ((Pair *)buf)->b = *(const int *)b;
 *       return buf;
 *   }
 *   Iterator *it = iter_zip(HeapIter(va), HeapIter(vb),
 *                           make_pair, sizeof(Pair));
 *   Pair *p;
 *   while ((p = iter_next(it))) { printf("%d+%d\n", p->a, p->b); }
 *   iter_destroy(it);
 * @endcode
 */
static inline Iterator *iter_zip(Iterator *left, Iterator *right,
                                 void *(*merge)(const Container *, const void *, const Container *, const void *,
                                                void *),
                                 size_t stride) {
    if (!left || !right || !merge || stride == 0) {
        iter_destroy(left);
        iter_destroy(right);
        return NULL;
    }
    ZipIter *z = (ZipIter *)malloc(sizeof(ZipIter));
    void *buf = malloc(stride);
    if (!z || !buf) {
        free(z);
        free(buf);
        iter_destroy(left);
        iter_destroy(right);
        return NULL;
    }
    z->base = (Iterator){
        .container = left->container,
        .ops = &ITER_ZIP_OPS
    };
    z->left = left;
    z->right = right;
    z->merge = merge;
    z->buffer = buf;
    return &z->base;
}

/** @} */ /* end of zip group */

/**
 * @defgroup peek Peek Decorator
 * @brief Look ahead at the next element without consuming it
 * @{
 */

/**
 * @struct PeekIter
 * @brief Internal structure for peek decorator (opaque)
 */
typedef struct {
    Iterator base;           /**< Base iterator interface */
    Iterator *inner;         /**< Inner iterator being peeked */
    const void *stashed;     /**< Cached element from a peek call */
    bool has_stashed;        /**< Flag indicating if an element is cached */
} PeekIter;

/** @cond INTERNAL */
static const void *_iter_peek_next(Iterator *it) {
    PeekIter *p = (PeekIter *)it;
    if (p->has_stashed) {
        p->has_stashed = false;
        const void *tmp = p->stashed;
        p->stashed = NULL;
        return tmp;
    }
    return iter_next(p->inner);
}

static void _iter_peek_destroy(Iterator *it) {
    PeekIter *p = (PeekIter *)it;
    iter_destroy(p->inner);
    free(p);
}

static const IteratorVTable ITER_PEEK_OPS = {
    .next = _iter_peek_next,
    .destroy = _iter_peek_destroy
};
/** @endcond */

/**
 * @brief Peek at the next element without advancing the iterator
 *
 * Returns the next element without consuming it. Repeated calls to
 * iter_peek return the same element until iter_next is called.
 *
 * @param it Peekable iterator created with iter_peekable()
 *
 * @return Pointer to the next element, or NULL if the iterator is exhausted.
 *
 * @warning Only call on iterators created with iter_peekable().
 *          Calling on a non-peekable iterator is undefined behaviour.
 *          Use iter_peekable() to wrap any iterator before peeking.
 */
static inline const void *iter_peek(Iterator *it) {
    if (!it) return NULL;

    PeekIter *p = (PeekIter *)(void *)it;
    if (!p->has_stashed) {
        p->stashed = iter_next(p->inner);
        if (p->stashed) {
            p->has_stashed = true;
        }
    }
    return p->stashed;
}

/**
 * @brief Create a peekable iterator that allows lookahead
 *
 * @param inner Source iterator (ownership taken)
 * @return A new decorator supporting peeking, or NULL.
 */
static inline Iterator *iter_peekable(Iterator *inner) {
    if (!inner) return NULL;
    
    PeekIter *p = (PeekIter *)malloc(sizeof(PeekIter));
    if (!p) {
        iter_destroy(inner);
        return NULL;
    }

    p->base = (Iterator){
        .container = inner->container,
        .ops = &ITER_PEEK_OPS
    };
    p->inner = inner;
    p->stashed = NULL;
    p->has_stashed = false;
    
    return &p->base;
}

/** @} */ /* end of peek group */

/** @} */ /* end of decorators group */

/* ============================================================================
 * Section: Terminal Operations
 * ============================================================================
 *
 * Terminal operations consume an iterator and produce a final result.
 * All terminals (except iter_drop) take ownership of the iterator and
 * call iter_destroy before returning. The caller must not use the
 * iterator after passing it to a terminal.
 *
 * Terminals enable common patterns like counting, searching, folding,
 * and collecting results into new containers.
 */

/**
 * @defgroup terminals Terminal Operations
 * @brief Consuming operations that exhaust an iterator and produce a result
 *
 * Terminal operations consume an iterator and produce a final result.
 * All terminals (except iter_drop) take ownership of the iterator and
 * call iter_destroy before returning. The caller must not use the
 * iterator after passing it to a terminal.
 * @{
 */

/**
 * @brief Discard the next n elements from an iterator in place
 *
 * Does NOT destroy the iterator — it remains valid and continues from the
 * (n+1)th element. Use this to seek forward within a pipeline without
 * terminating it.
 *
 * @param it Iterator to advance (NOT consumed)
 * @param n  Number of elements to discard
 *
 * @note Unlike all other terminals, iter_drop does NOT take ownership and
 *       does NOT call iter_destroy. The caller is still responsible for
 *       destroying the iterator.
 *
 * @par Example
 * @code
 *   Iterator *it = HeapIter(vec);
 *   iter_drop(it, 5);  // Skip first 5 elements
 *   void *e;
 *   while ((e = iter_next(it))) { process(e); }
 *   iter_destroy(it);
 * @endcode
 */
static inline void iter_drop(Iterator *it, size_t n) {
    while (n-- > 0 && iter_next(it));
}

/**
 * @brief Check if any element satisfies a predicate
 *
 * Returns true if at least one element satisfies `pred`. Short-circuits
 * on the first match. Consumes and destroys `it`.
 *
 * @param it   Iterator to search (consumed)
 * @param pred Predicate function; receives container and element.
 *             If NULL, returns true if the iterator has at least one element.
 *
 * @return true if any element satisfies pred, false if the iterator is empty
 *         or no element matches.
 *
 * @par Example
 * @code
 *   bool has_negative(const Container *c, const void *e) {
 *       return *(const int *)e < 0;
 *   }
 *   if (iter_any(HeapIter(vec), has_negative)) {
 *       printf("Vector contains negative numbers\n");
 *   }
 * @endcode
 */
static inline bool iter_any(Iterator *it, bool (*pred)(const Container *, const void *)) {
    if (!it) return false;
    bool found = false;
    const void *e;
    while ((e = iter_next(it))) {
        if (!pred || pred(it->container, e)) {  // NULL pred = match anything
            found = true;
            break;
        }
    }
    iter_destroy(it);
    return found;
}

/**
 * @brief Check if all elements satisfy a predicate
 *
 * Returns true if every element satisfies `pred`. Short-circuits on the
 * first failure. Consumes and destroys `it`.
 *
 * @param it   Iterator to check (consumed)
 * @param pred Predicate function; receives container and element
 *
 * @return true if all elements satisfy pred, or if the iterator is empty
 *         (vacuous truth), false if any element fails the predicate.
 *
 * @par Example
 * @code
 *   bool is_positive(const Container *c, const void *e) {
 *       return *(const int *)e > 0;
 *   }
 *   if (iter_all(HeapIter(vec), is_positive)) {
 *       printf("All elements are positive\n");
 *   }
 * @endcode
 */
static inline bool iter_all(Iterator *it, bool (*pred)(const Container *, const void *)) {
    bool res = true;
    const void *e;
    while ((e = iter_next(it))) {
        if (pred && !pred(it->container, e)) {
            res = false;
            break;
        }
    }
    iter_destroy(it);
    return res;
}

/**
 * @brief Count all remaining elements
 *
 * Counts all elements remaining in `it`. Consumes and destroys `it`.
 *
 * @param it Iterator to count (consumed)
 * @return Number of elements in the iterator, or 0 for an empty iterator.
 *
 * @note To count with a predicate, compose iter_filter then iter_count.
 *
 * @par Example
 * @code
 *   // Count elements matching a predicate
 *   size_t even_count = iter_count(
 *       iter_filter(HeapIter(vec), is_even));
 * @endcode
 */
static inline size_t iter_count(Iterator *it) {
    size_t n = 0;
    while (iter_next(it)) n++;
    iter_destroy(it);
    return n;
}

/**
 * @brief Apply a function to every element
 *
 * Calls `f(container, element)` for every element in `it` in order.
 * Consumes and destroys `it`.
 *
 * @param it Iterator to traverse (consumed)
 * @param f  Function to call for each element; receives container and element
 *
 * @warning f must not modify the element in a way that invalidates the source
 *          container's iteration order.
 *
 * @par Example
 * @code
 *   void print_int(const Container *c, const void *e) {
 *       printf("%d\n", *(const int *)e);
 *   }
 *   iter_for_each(HeapIter(vec), print_int);
 * @endcode
 */
static inline void iter_for_each(Iterator *it, void (*f)(const Container *, const void *)) {
    const void *e;
    while ((e = iter_next(it))) f(it->container, e);
    iter_destroy(it);
}

/**
 * @brief Find the first element satisfying a predicate
 *
 * Returns the first element that satisfies pred, or NULL if none found.
 * Consumes and destroys `it`.
 *
 * @param it   Iterator to search (consumed)
 * @param pred Predicate function; receives container and element.
 *             If NULL, returns the first element.
 *
 * @return Pointer to the first matching element, or NULL if not found.
 *
 * @warning For base iterators the pointer points directly into container storage.
 *          For map iterators the pointer points to an internal buffer that is
 *          valid until the next call to iter_next or iter_destroy.
 *
 * @par Example
 * @code
 *   int *first_even = (int *)iter_find(HeapIter(vec), is_even);
 *   if (first_even) printf("First even: %d\n", *first_even);
 *
 *   // Get first element without predicate
 *   int *first = (int *)iter_find(HeapIter(vec), NULL);
 * @endcode
 */
static inline const void *iter_find(Iterator *it, bool (*pred)(const Container *, const void *)) {
    if (!it) return NULL;
    
    const void *e;
    while ((e = iter_next(it))) {
        if (!pred || pred(it->container, e)) {
            iter_destroy(it);
            return e;
        }
    }
    
    iter_destroy(it);
    return NULL;
}

/**
 * @brief Left-fold over all elements
 *
 * Calls acc = f(container, element, acc) for each element in order,
 * threading the accumulator through. Returns the final accumulator value.
 * Consumes and destroys `it`.
 *
 * @param it  Iterator to fold (consumed)
 * @param acc Initial accumulator value; also the return value on empty input
 * @param f   Fold function; receives container, element, and current accumulator,
 *            must return the updated accumulator
 *
 * @return The final accumulator value after processing all elements.
 *
 * @par Example (sum all ints)
 * @code
 *   void *add(const Container *c, const void *e, void *acc) {
 *       *(int *)acc += *(const int *)e;
 *       return acc;
 *   }
 *   int sum = 0;
 *   iter_fold(HeapIter(vec), &sum, add);
 *   printf("Sum: %d\n", sum);
 * @endcode
 */
static inline void *iter_fold(Iterator *it, void *acc, void *(*f)(const Container *, const void *, void *)) {
    const void *e;
    while ((e = iter_next(it))) acc = f(it->container, e, acc);
    iter_destroy(it);
    return acc;
}

/**
 * @brief Materialize all elements into a new container
 *
 * Materializes all elements into a new container of the same type as the
 * source, created via the source container's instance() vtable function.
 * Inserts every element via insert(). Consumes and destroys `it`.
 *
 * @param it Iterator to collect (consumed)
 *
 * @return A new container containing all elements from the iterator, or NULL
 *         on allocation failure or empty source. On insert failure the partially
 *         built container is destroyed and NULL is returned.
 *
 * @note The returned container is heap-allocated and owned by the caller.
 *       Use container_destroy() to free it when done.
 *
 * @par Example
 * @code
 *   Container *result = iter_collect(
 *       iter_filter(HeapIter(vec), is_even));
 *   if (result) {
 *       printf("Collected %zu even numbers\n", container_len(result));
 *       container_destroy(result);
 *   }
 * @endcode
 */
static inline Container *iter_collect(Iterator *it) {
    if (!it) return NULL;
    
    Container *dst = it->container->ops->instance(it->container);
    if (!dst) {
        iter_destroy(it);
        return NULL;
    }
    
    const void *e;
    size_t count = 0;
    while ((e = iter_next(it))) {
        if (!dst->ops->insert(dst, e)) {
            dst->ops->destroy(dst);
            iter_destroy(it);
            return NULL;
        }
        count++;
    }
    iter_destroy(it);
    
    // Return NULL if no elements were collected
    if (count == 0) {
        dst->ops->destroy(dst);
        return NULL;
    }
    return dst;
}

/**
 * @brief Insert all elements into an existing container
 *
 * Inserts all elements from `it` into an existing container `dst` via
 * dst's insert() vtable function. Returns the number of elements
 * successfully inserted. Stops on the first insert failure.
 * Consumes and destroys `it`.
 *
 * @param it  Iterator to collect (consumed)
 * @param dst Existing container to insert into (NOT consumed)
 *
 * @return The number of elements successfully inserted, or 0 if it or dst is NULL.
 *
 * @note dst must already exist and be of a compatible type. Unlike iter_collect,
 *       dst is not created or destroyed here — the caller owns dst throughout.
 *
 * @par Example
 * @code
 *   size_t n = iter_collect_in(
 *       iter_filter(HeapIter(vec), is_even), existing_set);
 *   printf("Inserted %zu elements\n", n);
 * @endcode
 */
static inline size_t iter_collect_in(Iterator *it, Container *dst) {
    if (!it || !dst) {
        iter_destroy(it);
        return 0;
    }
    size_t n = 0;
    const void *e;
    while ((e = iter_next(it))) {
        if (dst->ops->insert(dst, e))
            n++;
        else
            break;
    }
    iter_destroy(it);
    return n;
}

/** @} */ /* end of terminals group */

#endif /* ITERATOR_PDR_H */
