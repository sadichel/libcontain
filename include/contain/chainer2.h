/** @file chainer2.h
 * @brief Extensible lazy fused pipeline with cached function pointers
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * The Chainer provides a declarative, multi-stage transformation
 * pipeline for Containers. Users can chain stages such as filter,
 * map, skip, take, flatten, and zip. Execution is deferred until
 * a terminal function is called, at which point all stages are
 * applied in a single fused pass — no intermediate containers are
 * allocated, and the source is traversed exactly once.
 *
 * @section key_features Key Features
 *   - @ref fused_execution — Single pass over the source container
 *   - @ref runtime_pipeline — Build pipelines dynamically
 *   - @ref generator_support — Flatten and other multi-output chains work
 *   - @ref extensible — Add custom chain types without modifying core
 *   - @ref no_vtables — Direct function pointers for speed
 *
 * @section example Example Pipeline
 * @code
 *   Chainer c = Chain(vec);                    // base container
 *        chain_skip(&c, 2)                     // skip first 2 elements
 *        chain_filter(&c, is_even)             // filter even numbers
 *        chain_map(&c, double_int, sizeof(int))// double each element
 *        chain_take(&c, 5);                    // take first 5
 * @endcode
 *
 * @section terminal_example Terminal Execution
 * @code
 *   Container *result = chain_collect(&c);     // collect into new container
 *   size_t n = chain_count(&c);                // count surviving elements
 *   bool has = chain_any(&c, pred);            // check if any element matches
 *   chain_for_each(&c, print_element);         // apply function to each
 *   acc = chain_fold(&c, acc, fold_fn);        // accumulate/fold
 * @endcode
 *
 * @section generator_example Generator Chains (flatten, zip)
 * @code
 *   Chainer c = Chain(outer_vec);              // vector of vectors
 *   chain_flatten(&c);                         // flatten into single stream
 *   chain_filter(&c, is_even);                 // then filter
 *   size_t n = chain_count(&c);                // count all evens
 * @endcode
 *
 * @section memory_management Memory Management
 *   - The Chainer does NOT own the source container.
 *   - The Chainer is reusable: terminals restore chain state automatically.
 *   - Use chain_bind() to change the source container between runs.
 *   - Call chain_destroy() when done to free all resources.
 *
 * @section thread_safety Thread-safety
 *   Chainer is NOT thread-safe. Each thread must use its own instance.
 *   Do not share a Chainer across threads during execution.
 */

#ifndef CHAINER2_PDR_H
#define CHAINER2_PDR_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "container.h"

/**
 * @defgroup chainer2_types Chainer2 Types
 * @brief Core types for the extensible pipeline system
 * @{
 */

/**
 * @enum ChainStatus
 * @brief Return value from chain process functions
 *
 * Each chain returns one of these status codes to control pipeline flow.
 *
 * @var CHAIN_PASS
 * Element passes through; continue to next chain.
 * The source iterator does NOT advance (used by flatten).
 *
 * @var CHAIN_SKIP
 * Discard this element; fetch next element from source.
 * The source iterator advances to the next element.
 *
 * @var CHAIN_STOP
 * Stop iteration entirely; no more elements will be processed.
 * The terminal function returns immediately.
 */
typedef enum {
    CHAIN_PASS,     /**< Element passes through, keep same source element */
    CHAIN_SKIP,     /**< Skip this element, get next from source */
    CHAIN_STOP      /**< Stop iteration entirely */
} ChainStatus;

/**
 * @typedef ChainProcessFunc
 * @brief Process one element through a chain
 *
 * @param chain Pointer to the chain's private data
 * @param c     Container context
 * @param item  Pointer to current element (may be modified)
 * @return ChainStatus to control flow
 */
typedef ChainStatus (*ChainProcessFunc)(void *chain, const Container *c, void **item);

/**
 * @typedef ChainBusyFunc
 * @brief Check if chain has pending work
 *
 * Returns true if the chain has pending work and the same source element
 * should be reused. Used by generator chains like flatten that produce
 * multiple outputs from one source element.
 *
 * @param chain Pointer to the chain's private data
 * @return true if chain is busy and source element should be reused
 */
typedef bool (*ChainBusyFunc)(const void *chain);

/**
 * @typedef ChainResetFunc
 * @brief Reset chain state to initial values
 *
 * Called by terminals after pipeline execution to enable reuse.
 *
 * @param chain Pointer to the chain's private data
 */
typedef void (*ChainResetFunc)(void *chain);

/**
 * @typedef ChainDestroyFunc
 * @brief Free chain-specific resources
 *
 * Called by chain_destroy() when the Chainer is destroyed.
 *
 * @param chain Pointer to the chain's private data
 */
typedef void (*ChainDestroyFunc)(void *chain);

/**
 * @struct ChainType
 * @brief Minimal header for all chain types
 *
 * Stores cached function pointers for direct calls (no vtable indirection
 * in the hot path). Chain-specific data follows this header in memory.
 *
 * The process, is_busy, reset, and destroy pointers are set at chain
 * creation time and never change. This design eliminates virtual dispatch
 * overhead during iteration.
 */
typedef struct ChainType {
    ChainProcessFunc process;   /**< Process function for this chain */
    ChainBusyFunc    is_busy;   /**< Check if chain has pending work */
    ChainResetFunc   reset;     /**< Reset chain to initial state */
    ChainDestroyFunc destroy;   /**< Free chain-specific resources */
    /* ChainType-specific data follows */
} ChainType;

/**
 * @struct Chainer
 * @brief Pipeline state for lazy fused execution
 *
 * @var Chainer::base
 * Source container. Not owned; must outlive the Chainer.
 *
 * @var Chainer::chains
 * Array of pointers to chain instances. Each chain is a heap-allocated block
 * containing ChainType header + private data. Cached pointers eliminate
 * arena math in the hot path.
 *
 * @var Chainer::chain_len
 * Number of chains currently in the pipeline.
 *
 * @var Chainer::chain_cap
 * Allocated capacity of the chains array.
 *
 * @var Chainer::has_generators
 * True if any chain is a generator (e.g., flatten). Enables the slow-path
 * iteration logic that supports multiple outputs per source element.
 */
typedef struct {
    const Container *base;          /**< Source container (not owned) */
    ChainType **chains;             /**< Array of chain pointers */
    uint32_t      chain_len;        /**< Number of chains in pipeline */
    uint32_t      chain_cap;        /**< Allocated capacity of chains array */
    bool          has_generators;   /**< True if any chain is a generator */
} Chainer;

/** @} */ /* end of chainer2_types group */

/**
 * @defgroup chainer2_internal Internal Functions
 * @brief Internal helpers for pipeline management (not part of public API)
 * @{
 */

/**
 * @brief Ensure the chains array has room for at least one more chain
 *
 * Doubles capacity when needed (starting at 8).
 *
 * @param c Chainer to grow
 * @return true on success, false on allocation failure
 *
 * @note On failure the Chainer state is unchanged.
 */
static inline bool _chain_grow(Chainer *c) {
    if (c->chain_len < c->chain_cap) return true;
    uint32_t new_cap = c->chain_cap ? c->chain_cap * 2 : 8;
    ChainType **tmp = (ChainType **)realloc(c->chains, new_cap * sizeof(ChainType *));
    if (!tmp) return false;
    c->chains = tmp;
    c->chain_cap = new_cap;
    return true;
}

/**
 * @brief Reset all chains to their initial state
 *
 * Called automatically by every terminal after execution to enable pipeline reuse.
 * For skip/take chains, this restores curr_n to init_n.
 * For flatten chains, this clears the active inner iterator state.
 *
 * @param c Chainer to reset (can be NULL)
 */
static inline void _chain_reset(Chainer *c) {
    if (!c) return;
    for (uint32_t i = 0; i < c->chain_len; i++) {
        ChainType *chain = c->chains[i];
        if (chain->reset) chain->reset(chain);
    }
}

/**
 * @brief Pull the next element through the pipeline [HOT PATH]
 *
 * Pulls the next element from c->base via the container's own next()
 * and runs it through every recorded chain in order.
 *
 * For chains without generators (filter, map, skip, take), the fast path
 * is used. For chains with generators (flatten), the slow path is used
 * which supports multiple outputs per source element.
 *
 * @param it Iterator cursor for the container (initialised with Iter(c->base))
 * @param c  Chainer containing the pipeline
 * @return The first element that survives all chains, or NULL when
 *         the source is exhausted or a chain returns CHAIN_STOP
 */
static inline const void *_chain_fused_next(Iterator *it, Chainer *c) {
    void *curr = NULL;
    
    /* Fast path: no generators (filter, map, skip, take) */
    if (!c->has_generators) {
        while ((curr = (void *)iter_next(it))) {
            bool skip = false;
            for (uint32_t i = 0; i < c->chain_len; i++) {
                ChainStatus status = c->chains[i]->process(c->chains[i], c->base, &curr);
                if (status == CHAIN_SKIP) { skip = true; break; }
                if (status == CHAIN_STOP) return NULL;
            }
            if (!skip) return curr;
        }
        return NULL;
    }
    
    /* Slow path: with generators (flatten, etc.) */
    while (true) {
        int32_t start = 0;
        bool needs_fetch = true;
        
        /* Find the rightmost busy chain (working backwards from the end) */
        for (int32_t i = (int32_t)c->chain_len - 1; i >= 0; i--) {
            ChainType *chain = c->chains[i];
            if (chain->is_busy && chain->is_busy(chain)) {
                start = i;
                needs_fetch = false;
                break;
            }
        }
        
        /* If no busy chain, fetch next source element */
        if (needs_fetch) {
            curr = (void *)iter_next(it);
            if (!curr) return NULL;
        }
        
        /* Process from the busy chain (or start) to the end */
        bool skip = false;
        for (uint32_t i = (uint32_t)start; i < c->chain_len; i++) {
            ChainType *chain = c->chains[i];
            ChainStatus status = chain->process(chain, c->base, &curr);
            
            if (status == CHAIN_SKIP) {
                skip = true;
                break;
            }
            if (status == CHAIN_STOP) return NULL;
        }
        
        if (!skip) return curr;
    }
}

/** @} */ /* end of chainer2_internal group */

/**
 * @defgroup chainer2_api Public API
 * @brief Chainer initialization, management, builders, and terminals
 * @{
 */

/**
 * @defgroup chainer2_init Initialization
 * @brief Create and destroy Chainer instances
 * @{
 */

/**
 * @brief Create a new Chainer over a container
 *
 * Initialises a new Chainer over base. No memory is allocated until
 * the first chain is added. base is not owned — it must remain valid
 * until the terminal is called.
 *
 * @param base Source container (must outlive the Chainer)
 * @return Initialized Chainer value
 *
 * @note Take its address to pass to chain_* functions:
 * @code
 *   Chainer c = Chain(vec);
 *   chain_filter(&c, pred);
 *   Container *r = chain_collect(&c);
 * @endcode
 */
static inline Chainer Chain(const Container *base) {
    return (Chainer){
        .base = base,
        .chains = NULL,
        .chain_len = 0,
        .chain_cap = 0,
        .has_generators = false
    };
}

/**
 * @brief Change the source container for a Chainer
 *
 * Useful for reusing the same pipeline configuration on different containers.
 * The pipeline chains are preserved; only the data source changes.
 *
 * @param c         Chainer to rebind
 * @param new_base  New source container
 *
 * @par Example
 * @code
 *   Chainer c = Chain(vec1);
 *   chain_filter(&c, is_even);
 *   chain_count(&c);        // runs on vec1
 *   chain_bind(&c, vec2);
 *   chain_count(&c);        // runs on vec2 with same pipeline
 *   chain_destroy(&c);
 * @endcode
 */
static inline void chain_bind(Chainer *c, const Container *new_base) {
    if (!c) return;
    c->base = new_base;
    _chain_reset(c);
}

/**
 * @brief Destroy a Chainer and free all resources
 *
 * Frees all chain resources (including map buffers) and the chains array
 * itself. Resets all fields to zero.
 *
 * @param c Chainer to destroy (can be NULL)
 */
static inline void chain_destroy(Chainer *c) {
    if (!c) return;
    for (uint32_t i = 0; i < c->chain_len; i++) {
        ChainType *chain = c->chains[i];
        if (chain->destroy) chain->destroy(chain);
        free(chain);
    }
    free(c->chains);
    memset(c, 0, sizeof(Chainer));
}

/** @} */ /* end of chainer2_init group */

/**
 * @defgroup chainer2_builders Chain Builders
 * @brief Append transformation stages to the pipeline
 *
 * Each builder appends a new chain to the pipeline. Chains are executed
 * in the order they are added. Builders return c to allow call chaining.
 *
 * @note If allocation fails, the chain is silently skipped — the Chainer
 *       remains usable and the terminal executes without that chain.
 * @{
 */

/**
 * @struct FilterChain
 * @brief Internal structure for filter chain (opaque)
 */
typedef struct {
    ChainType base;                                 /**< Base chain header */
    bool (*predicate)(const Container *ctx, const void *item); /**< Predicate function */
} FilterChain;

/** @cond INTERNAL */
static ChainStatus _chain_filter_process(FilterChain *f, const Container *c, void **item) {
    return f->predicate(c, *item) ? CHAIN_PASS : CHAIN_SKIP;
}
/** @endcond */

/**
 * @brief Append a filter chain to the pipeline
 *
 * Elements for which pred(container, element) returns false are dropped
 * and never reach subsequent chains or the terminal.
 *
 * @param c    Chainer to modify
 * @param pred Predicate function; returns true to keep the element
 * @return c (for call chaining)
 *
 * @par Example
 * @code
 *   chain_filter(&c, is_even);   // keep only even numbers
 * @endcode
 */
static inline Chainer *chain_filter(Chainer *c, bool (*pred)(const Container *, const void *)) {
    if (!pred) return c;
    if (!_chain_grow(c)) return c;
    
    FilterChain *fc = (FilterChain *)malloc(sizeof(FilterChain));
    if (!fc) return c;
    
    fc->base.process = (ChainProcessFunc)_chain_filter_process;
    fc->base.is_busy = NULL;
    fc->base.reset = NULL;
    fc->base.destroy = NULL;
    fc->predicate = pred;
    
    c->chains[c->chain_len++] = (ChainType *)fc;
    return c;
}

/**
 * @struct MapChain
 * @brief Internal structure for map chain (opaque)
 */
typedef struct {
    ChainType base;                                 /**< Base chain header */
    void *(*mapper)(const Container *ctx, const void *item, void *buf); /**< Map function */
    void *buffer;                                   /**< Internal output buffer */
} MapChain;

/** @cond INTERNAL */
static ChainStatus _chain_map_process(MapChain *m, const Container *c, void **item) {
    *item = m->mapper(c, *item, m->buffer);
    return *item ? CHAIN_PASS : CHAIN_SKIP;
}

static void _chain_map_destroy(MapChain *m) {
    free(m->buffer);
}
/** @endcond */

/**
 * @brief Append a map chain to the pipeline
 *
 * Every element that reaches this chain is transformed by map, which writes
 * its result into an internally allocated buffer of stride bytes.
 *
 * @param c      Chainer to modify
 * @param mapper Map function; receives container, element, and output buffer.
 *               Must write result to buffer and return it, or return NULL to drop.
 * @param stride Size in bytes of the output element type. Must be greater than zero.
 * @return c (for call chaining)
 *
 * @note The buffer is reused across elements — it holds only the most
 *       recently mapped value.
 *
 * @note The mapper may return NULL to drop the element, acting as
 *       a combined map+filter.
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
 *       chain_map(&c, str_to_upper, 256);
 *       @endcode
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
 *   chain_map(&c, double_int, sizeof(int));
 * @endcode
 */
static inline Chainer *chain_map(Chainer *c, void *(*mapper)(const Container *, const void *, void *), size_t stride) {
    if (!mapper || stride == 0) return c;
    if (!_chain_grow(c)) return c;
    
    MapChain *mc = (MapChain *)malloc(sizeof(MapChain));
    if (!mc) return c;
    
    mc->base.process = (ChainProcessFunc)_chain_map_process;
    mc->base.is_busy = NULL;
    mc->base.reset = NULL;
    mc->base.destroy = (ChainDestroyFunc)_chain_map_destroy;
    mc->mapper = mapper;
    mc->buffer = malloc(stride);
    if (!mc->buffer) { free(mc); return c; }
    
    c->chains[c->chain_len++] = (ChainType *)mc;
    return c;
}

/**
 * @struct SkipChain
 * @brief Internal structure for skip chain (opaque)
 */
typedef struct {
    ChainType base;     /**< Base chain header */
    uint32_t init_n;    /**< Initial skip count */
    uint32_t curr_n;    /**< Current remaining skip count */
} SkipChain;

/** @cond INTERNAL */
static ChainStatus _chain_skip_process(SkipChain *sk, const Container *c, void **item) {
    (void)c; (void)item;
    if (sk->curr_n == 0) return CHAIN_PASS;
    sk->curr_n--;
    return CHAIN_SKIP;
}

static void _chain_skip_reset(SkipChain *sk) {
    sk->curr_n = sk->init_n;
}
/** @endcond */

/**
 * @brief Append a skip chain to the pipeline
 *
 * The first n elements from the source are dropped and never reach
 * subsequent chains or the terminal.
 *
 * @param c Chainer to modify
 * @param n Number of leading elements to discard
 * @return c (for call chaining)
 *
 * @par Example
 * @code
 *   chain_skip(&c, 5);   // skip first 5 elements
 * @endcode
 */
static inline Chainer *chain_skip(Chainer *c, uint32_t n) {
    if (!_chain_grow(c)) return c;
    
    SkipChain *sc = (SkipChain *)malloc(sizeof(SkipChain));
    if (!sc) return c;
    
    sc->base.process = (ChainProcessFunc)_chain_skip_process;
    sc->base.is_busy = NULL;
    sc->base.reset = (ChainResetFunc)_chain_skip_reset;
    sc->base.destroy = NULL;
    sc->init_n = n;
    sc->curr_n = n;
    
    c->chains[c->chain_len++] = (ChainType *)sc;
    return c;
}

/**
 * @struct TakeChain
 * @brief Internal structure for take chain (opaque)
 */
typedef struct {
    ChainType base;     /**< Base chain header */
    uint32_t init_n;    /**< Initial take count */
    uint32_t curr_n;    /**< Current remaining take count */
} TakeChain;

/** @cond INTERNAL */
static ChainStatus _chain_take_process(TakeChain *t, const Container *c, void **item) {
    (void)c; (void)item;
    if (t->curr_n == 0) return CHAIN_STOP;
    t->curr_n--;
    return CHAIN_PASS;
}

static void _chain_take_reset(TakeChain *t) {
    t->curr_n = t->init_n;
}
/** @endcond */

/**
 * @brief Append a take chain to the pipeline
 *
 * At most n elements that reach this chain are passed through.
 * Once n elements have been emitted, iteration stops entirely.
 *
 * @param c Chainer to modify
 * @param n Maximum number of elements to yield
 * @return c (for call chaining)
 *
 * @par Example
 * @code
 *   chain_take(&c, 10);   // take first 10 elements
 * @endcode
 */
static inline Chainer *chain_take(Chainer *c, uint32_t n) {
    if (!_chain_grow(c)) return c;
    
    TakeChain *tc = (TakeChain *)malloc(sizeof(TakeChain));
    if (!tc) return c;
    
    tc->base.process = (ChainProcessFunc)_chain_take_process;
    tc->base.is_busy = NULL;
    tc->base.reset = (ChainResetFunc)_chain_take_reset;
    tc->base.destroy = NULL;
    tc->init_n = n;
    tc->curr_n = n;
    
    c->chains[c->chain_len++] = (ChainType *)tc;
    return c;
}

/**
 * @struct FlattenChain
 * @brief Internal structure for flatten chain (opaque)
 */
typedef struct {
    ChainType base;         /**< Base chain header */
    Iterator inner_it;      /**< Current inner iterator */
    bool active;            /**< Whether inner iterator is active */
} FlattenChain;

/** @cond INTERNAL */
static bool _chain_flatten_is_busy(const FlattenChain *fc) {
    return fc->active;
}

static ChainStatus _chain_flatten_process(FlattenChain *fc, const Container *c, void **item) {
    (void)c;
    
    if (fc->active) {
        void *res = (void *)iter_next(&fc->inner_it);
        if (res) {
            *item = res;
            return CHAIN_PASS;
        }
        fc->active = false;
        return CHAIN_SKIP;
    }
    
    if (!item || !*item) return CHAIN_SKIP;
    
    Container *inner = *(Container **)*item;
    if (!inner) return CHAIN_SKIP;
    
    fc->inner_it = Iter(inner);
    fc->active = true;
    
    void *res = (void *)iter_next(&fc->inner_it);
    if (res) {
        *item = res;
        return CHAIN_PASS;
    }
    
    fc->active = false;
    return CHAIN_SKIP;
}

static void _chain_flatten_reset(FlattenChain *fc) {
    fc->active = false;
}
/** @endcond */

/**
 * @brief Append a flatten chain to the pipeline
 *
 * Flattens a two-level sequence. The source container must contain pointers
 * to Container objects (e.g., Vector of Vector*). Flatten walks each inner
 * container in sequence, yielding every element of each one before moving
 * to the next.
 *
 * This is a generator chain — it produces multiple outputs from a single
 * source element. The is_busy flag tells the fused loop not to advance
 * the source iterator while the current inner container still has elements
 * to yield.
 *
 * @param c Chainer to modify
 * @return c (for call chaining)
 *
 * @par Example
 * @code
 *   Vector *outer = create_vector_of_vectors(3, 5);
 *   Chainer c = Chain((Container *)outer);
 *   chain_flatten(&c);   // yields 15 ints (5 from each inner vector)
 *   size_t n = chain_count(&c);  // 15
 * @endcode
 */
static inline Chainer *chain_flatten(Chainer *c) {
    if (!_chain_grow(c)) return c;
    
    FlattenChain *fc = (FlattenChain *)malloc(sizeof(FlattenChain));
    if (!fc) return c;
    
    fc->base.process = (ChainProcessFunc)_chain_flatten_process;
    fc->base.is_busy = (ChainBusyFunc)_chain_flatten_is_busy;
    fc->base.reset = (ChainResetFunc)_chain_flatten_reset;
    fc->base.destroy = NULL;
    fc->active = false;
    
    c->chains[c->chain_len++] = (ChainType *)fc;
    c->has_generators = true;
    return c;
}

/**
 * @struct ZipChain
 * @brief Internal structure for zip chain (opaque)
 */
typedef struct {
    ChainType base;                                     /**< Base chain header */
    Iterator other_it;                                  /**< Iterator over right container */
    void *buffer;                                       /**< Internal output buffer */
    void *(*merge)(const Container *a_ctx, const void *a,
                   const Container *b_ctx, const void *b, void *buf); /**< Merge function */
    const Container *other;                             /**< Right container */
    bool initialized;                                   /**< Whether other_it is initialised */
} ZipChain;

/** @cond INTERNAL */
static ChainStatus _chain_zip_process(ZipChain *zc, const Container *c, void **item) {
    if (!zc->initialized) {
        zc->other_it = Iter(zc->other);
        zc->initialized = true;
    }
    
    void *other_item = (void *)iter_next(&zc->other_it);
    if (!other_item) return CHAIN_STOP;
    
    void *result = zc->merge(c, *item, zc->other, other_item, zc->buffer);
    if (!result) return CHAIN_STOP;
    
    *item = result;
    return CHAIN_PASS;
}

static void _chain_zip_reset(ZipChain *zc) {
    zc->initialized = false;
}

static void _chain_zip_destroy(ZipChain *zc) {
    free(zc->buffer);
}
/** @endcond */

/**
 * @brief Append a zip chain to the pipeline
 *
 * Pairs elements from the source container (left) with elements from another
 * container (right). For each step, one element is taken from each source
 * and passed to the merge function, which combines them into a single result.
 * Iteration stops when either source is exhausted.
 *
 * @param c      Chainer to modify
 * @param other  Right container to zip with
 * @param merge  Merge function; receives left container, left element,
 *               right container, right element, and output buffer
 * @param stride Size in bytes of the merged output type
 * @return c (for call chaining)
 *
 * @par Example
 * @code
 *   void *merge_ints(const Container *ca, const void *a,
 *                    const Container *cb, const void *b, void *buf) {
 *       *(int *)buf = *(const int *)a + *(const int *)b;
 *       return buf;
 *   }
 *   chain_zip(&c, other_vec, merge_ints, sizeof(int));
 * @endcode
 */
static inline Chainer *chain_zip(Chainer *c, const Container *other,
    void *(*merge)(const Container *, const void *,
                   const Container *, const void *, void *),
    size_t stride) {
    
    if (!other || !merge || stride == 0) return c;
    if (!_chain_grow(c)) return c;
    
    ZipChain *zc = (ZipChain *)malloc(sizeof(ZipChain));
    if (!zc) return c;
    
    zc->base.process = (ChainProcessFunc)_chain_zip_process;
    zc->base.is_busy = NULL;
    zc->base.reset = (ChainResetFunc)_chain_zip_reset;
    zc->base.destroy = (ChainDestroyFunc)_chain_zip_destroy;
    zc->buffer = malloc(stride);
    if (!zc->buffer) { free(zc); return c; }
    zc->merge = merge;
    zc->other = other;
    zc->initialized = false;
    
    c->chains[c->chain_len++] = (ChainType *)zc;
    return c;
}

/** @} */ /* end of chainer2_builders group */

/**
 * @defgroup chainer2_terminals Terminals
 * @brief Consuming operations that execute the pipeline
 *
 * Terminal operations consume the pipeline and produce a final result.
 * All terminals call _chain_reset() before returning, enabling reuse.
 * @{
 */

/**
 * @brief Count the number of elements that survive all chains
 *
 * @param c Chainer to execute
 * @return Number of surviving elements
 *
 * @par Example
 * @code
 *   Chainer c = Chain(vec);
 *   chain_filter(&c, is_even);
 *   size_t n = chain_count(&c);   // counts even numbers
 * @endcode
 */
static inline size_t chain_count(Chainer *c) {
    if (!c || !c->base) return 0;
    Iterator it = Iter(c->base);
    size_t n = 0;
    while (_chain_fused_next(&it, c)) n++;
    _chain_reset(c);
    return n;
}

/**
 * @brief Check if any surviving element satisfies a predicate
 *
 * Short-circuits on the first match.
 *
 * @param c    Chainer to execute
 * @param pred Predicate function (if NULL, returns true if pipeline has any element)
 * @return true if any element matches, false otherwise
 *
 * @par Example
 * @code
 *   bool has_negative = chain_any(&c, is_negative);
 * @endcode
 */
static inline bool chain_any(Chainer *c, bool (*pred)(const Container *, const void *)) {
    if (!c || !c->base) return false;
    Iterator it = Iter(c->base);
    const void *e;
    while ((e = _chain_fused_next(&it, c))) {
        if (!pred || pred(c->base, e)) {
            _chain_reset(c);
            return true;
        }
    }
    _chain_reset(c);
    return false;
}

/**
 * @brief Check if every surviving element satisfies a predicate
 *
 * Short-circuits on the first failure.
 *
 * @param c    Chainer to execute
 * @param pred Predicate function
 * @return true if all elements match, or if pipeline is empty (vacuous truth)
 *
 * @note If pred is NULL, returns true if the pipeline produces at least one element.
 *
 * @par Example
 * @code
 *   bool all_positive = chain_all(&c, is_positive);
 * @endcode
 */
static inline bool chain_all(Chainer *c, bool (*pred)(const Container *, const void *)) {
    if (!c || !c->base) return true;
    if (!pred) {
        Iterator it = Iter(c->base);
        const void *e = _chain_fused_next(&it, c);
        _chain_reset(c);
        return e != NULL;
    }
    Iterator it = Iter(c->base);
    const void *e;
    while ((e = _chain_fused_next(&it, c))) {
        if (!pred(c->base, e)) {
            _chain_reset(c);
            return false;
        }
    }
    _chain_reset(c);
    return true;
}

/**
 * @brief Apply a function to every surviving element
 *
 * Typically used for side effects such as printing, aggregation,
 * or external state mutation.
 *
 * @param c Chainer to execute
 * @param f Function to call for each element
 *
 * @par Example
 * @code
 *   chain_for_each(&c, print_int);
 * @endcode
 */
static inline void chain_for_each(Chainer *c, void (*f)(const Container *, const void *)) {
    if (!c || !c->base || !f) return;
    Iterator it = Iter(c->base);
    const void *e;
    while ((e = _chain_fused_next(&it, c))) f(c->base, e);
    _chain_reset(c);
}

/**
 * @brief Reduce all surviving elements into a single accumulated value
 *
 * The accumulator is updated for each element by calling:
 *   acc = f(container, element, acc)
 *
 * @param c   Chainer to execute
 * @param acc Initial accumulator value (returned on empty pipeline)
 * @param f   Fold function
 * @return Final accumulator value
 *
 * @par Example (sum of all evens)
 * @code
 *   int sum = 0;
 *   chain_filter(&c, is_even);
 *   sum = *(int *)chain_fold(&c, &sum, add_int);
 * @endcode
 */
static inline void *chain_fold(Chainer *c, void *acc, void *(*f)(const Container *, const void *, void *)) {
    if (!c || !c->base || !f) return acc;
    Iterator it = Iter(c->base);
    const void *e;
    while ((e = _chain_fused_next(&it, c))) acc = f(c->base, e, acc);
    _chain_reset(c);
    return acc;
}

/**
 * @brief Find the first surviving element that satisfies a predicate
 *
 * Short-circuits on the first match.
 *
 * @param c    Chainer to execute
 * @param pred Predicate function (if NULL, returns the first element)
 * @return Pointer to the element, or NULL if not found
 *
 * @warning The pointer is only valid until the next iteration or chain_destroy().
 *
 * @par Example
 * @code
 *   int *first_even = chain_find(&c, is_even);
 * @endcode
 */
static inline const void *chain_find(Chainer *c, bool (*pred)(const Container *, const void *)) {
    if (!c || !c->base) return NULL;
    Iterator it = Iter(c->base);
    const void *e;
    while ((e = _chain_fused_next(&it, c))) {
        if (!pred || pred(c->base, e)) {
            _chain_reset(c);
            return e;
        }
    }
    _chain_reset(c);
    return NULL;
}

/**
 * @brief Get the first surviving element
 *
 * Equivalent to chain_find(c, NULL).
 *
 * @param c Chainer to execute
 * @return Pointer to the first element, or NULL if pipeline is empty
 *
 * @par Example
 * @code
 *   int *first = chain_first(&c);
 * @endcode
 */
static inline const void *chain_first(Chainer *c) {
    return chain_find(c, NULL);
}

/**
 * @brief Materialize all surviving elements into a new container
 *
 * Creates a new container of the same type as the source via the source's
 * instance() vtable function, then inserts all surviving elements.
 *
 * @param c Chainer to execute
 * @return New container with all surviving elements, or NULL on failure or empty pipeline
 *
 * @note The caller owns the returned container and must destroy it with
 *       container_destroy().
 *
 * @par Example
 * @code
 *   Container *evens = chain_collect(chain_filter(&c, is_even));
 * @endcode
 */
static inline Container *chain_collect(Chainer *c) {
    if (!c || !c->base) return NULL;
    
    Container *dst = c->base->ops->instance(c->base);
    if (!dst) {
        _chain_reset(c);
        return NULL;
    }
    
    Iterator it = Iter(c->base);
    const void *e;
    size_t count = 0;
    while ((e = _chain_fused_next(&it, c))) {
        if (!dst->ops->insert(dst, e)) {
            dst->ops->destroy(dst);
            _chain_reset(c);
            return NULL;
        }
        count++;
    }
    _chain_reset(c);
    
    if (count == 0) {
        dst->ops->destroy(dst);
        return NULL;
    }
    return dst;
}

/**
 * @brief Insert all surviving elements into an existing container
 *
 * @param c   Chainer to execute
 * @param dst Existing container to insert into (NOT consumed)
 * @return Number of elements successfully inserted (stops on first failure)
 *
 * @note dst must already exist and be of a compatible type. Unlike chain_collect,
 *       dst is not created or destroyed here — the caller owns dst throughout.
 *
 * @par Example
 * @code
 *   size_t n = chain_collect_into(chain_filter(&c, is_even), existing_set);
 * @endcode
 */
static inline size_t chain_collect_into(Chainer *c, Container *dst) {
    if (!c || !c->base || !dst) {
        if (c) _chain_reset(c);
        return 0;
    }
    
    Iterator it = Iter(c->base);
    size_t n = 0;
    const void *e;
    while ((e = _chain_fused_next(&it, c))) {
        if (dst->ops->insert(dst, e)) n++;
        else break;
    }
    _chain_reset(c);
    return n;
}

/** @} */ /* end of chainer2_terminals group */

/** @} */ /* end of chainer2_api group */

#endif /* CHAINER2_PDR_H */

