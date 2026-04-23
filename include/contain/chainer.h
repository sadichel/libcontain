/** @file chainer.h
 * @brief Lazy fused pipeline over a Container
 * libcontain - https://github.com/sadichel/libcontain 
 *
 * The Chainer provides a declarative, multi-stage transformation
 * pipeline for Containers. Users can chain operations such as filter,
 * map, skip, and take. Execution is deferred until a terminal
 * function is called, at which point all chains are applied in
 * a single fused pass — no intermediate containers are allocated,
 * and the source is traversed exactly once.
 *
 * @section key_features Key Features
 *   - @ref fused_execution — Single pass over the source container
 *   - @ref runtime_pipeline — Build pipelines dynamically
 *   - @ref no_virtual_calls — Direct switch dispatch in hot path
 *   - @ref reusable — Reset and bind to new containers
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
 * @section pipeline_flow Pipeline Flow
 * @verbatim
 *   Source Container (vec)
 *         |
 *         v
 *    [SKIP N]          ← skip first N elements
 *         |
 *         v
 *   [FILTER pred]      ← drop elements failing predicate
 *         |
 *         v
 *    [MAP func]        ← transform each element (buffered)
 *         |
 *         v
 *    [TAKE M]          ← limit to first M surviving elements
 *         |
 *         v
 *      Terminal        ← chain_collect / chain_fold / chain_for_each / ...
 * @endverbatim
 *
 * @section memory_management Memory Management
 *   - The Chainer does NOT own the source container.
 *   - The Chainer is reusable: terminals restore chain state automatically.
 *   - Use chain_bind() to change the source container between runs.
 *   - Chain counts (for skip/take) are automatically restored.
 *   - Call chain_destroy() when done to free all resources.
 *
 * @section reuse_example Reuse Example
 * @code
 *   Chainer c = Chain(vec1);
 *   chain_filter(&c, is_even);
 *   chain_skip(&c, 2);
 *   chain_take(&c, 5);
 *
 *   size_t n1 = chain_count(&c);    // counts on vec1
 *
 *   chain_bind(&c, vec2);
 *   size_t n2 = chain_count(&c);    // counts on vec2
 *
 *   chain_destroy(&c);
 * @endcode
 *
 * @section thread_safety Thread-safety
 *   Chainer is NOT thread-safe. Each thread must use its own instance.
 *   Do not share a Chainer across threads during execution.
 */

#ifndef CHAINER_PDR_H
#define CHAINER_PDR_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "container.h"

/**
 * @defgroup chainer_types Chainer Types
 * @brief Core types for the pipeline system
 * @{
 */

/**
 * @enum ChainKind
 * @brief Type of pipeline chain
 *
 * Each chain in the pipeline is one of these four types:
 *
 * @var CHAIN_FILTER
 * Holds a predicate function pointer. Elements for which pred returns false
 * are dropped. buffer is NULL.
 *
 * @var CHAIN_MAP
 * Holds a map function pointer and an output buffer of stride bytes.
 * The map function receives the source container, the current element,
 * and the buffer; it must write its result into buffer and return buffer,
 * or return NULL to drop the element. The buffer is reused across elements.
 *
 * @var CHAIN_TAKE
 * Holds init_count (original) and curr_count (current). curr_count is
 * decremented after each element is emitted. When curr_count reaches zero,
 * iteration stops entirely. init_count allows _chain_reset() to restore state.
 *
 * @var CHAIN_SKIP
 * Holds init_count (original) and curr_count (current). curr_count is
 * decremented after each element is processed. When curr_count reaches zero,
 * the chain no longer affects elements. init_count allows _chain_reset()
 * to restore state.
 */
typedef enum {
    CHAIN_FILTER,   /**< Filter chain — drops elements failing predicate */
    CHAIN_MAP,      /**< Map chain — transforms elements via mapping function */
    CHAIN_TAKE,     /**< Take chain — limits to at most N elements */
    CHAIN_SKIP      /**< Skip chain — discards first N elements */
} ChainKind;

/**
 * @struct ChainType
 * @brief One step in the pipeline
 *
 * Chains are stored in a growable array inside Chainer and freed by
 * chain_destroy() after the Chainer is no longer needed.
 *
 * The union holds type-specific data:
 *   - FILTER: pred function pointer
 *   - MAP: map function pointer and output buffer
 *   - TAKE: init_count and curr_count
 *   - SKIP: init_count and curr_count (same layout as TAKE)
 */
typedef struct {
    ChainKind kind;                     /**< Type of this chain */
    union {
        /** Predicate function for filter chains */
        bool (*pred)(const Container *, const void *);
        /** Map function for map chains */
        void *(*map)(const Container *, const void *, void *);
        /** Count state for skip/take chains */
        struct { uint32_t init_count; uint32_t curr_count; };
    };
    void *buffer;                       /**< Output buffer for map chains only */
} ChainType;

/**
 * @struct Chainer
 * @brief Pipeline state for lazy fused execution
 *
 * @var Chainer::base
 * Source container. Not owned; must outlive the Chainer.
 *
 * @var Chainer::chains
 * Growable array of pipeline chains, heap-allocated.
 * NULL until the first chain is added.
 *
 * @var Chainer::len
 * Number of chains currently in the pipeline.
 *
 * @var Chainer::capacity
 * Allocated capacity of the chains array.
 */
typedef struct {
    const Container *base;      /**< Source container (not owned) */
    ChainType *chains;          /**< Array of pipeline chains */
    size_t len;                 /**< Number of chains in pipeline */
    size_t capacity;            /**< Allocated capacity of chains array */
} Chainer;

/** @} */ /* end of chainer_types group */

/**
 * @defgroup chainer_internal Internal Functions
 * @brief Internal helpers for pipeline management (not part of public API)
 * @{
 */

/**
 * @brief Restore skip/take chain counts to their initial values
 *
 * Called automatically by every terminal after execution to enable pipeline reuse.
 *
 * @param c Chainer to reset (can be NULL)
 */
static inline void _chain_reset(Chainer *c) {
    if (!c) return;
    for (size_t i = 0; i < c->len; i++) {
        ChainType *ch = &c->chains[i];
        if (ch->kind == CHAIN_SKIP || ch->kind == CHAIN_TAKE) {
            ch->curr_count = ch->init_count;
        }
    }
}

/**
 * @brief Ensure the chains array has room for at least one more chain
 *
 * Doubles capacity when needed (starting at 4).
 *
 * @param c Chainer to grow
 * @return true on success, false on allocation failure
 *
 * @note On failure the Chainer state is unchanged and the caller
 *       silently skips adding the chain.
 */
static inline bool _chain_grow(Chainer *c) {
    if (c->len < c->capacity) return true;
    size_t new_cap = c->capacity ? c->capacity * 2 : 4;
    ChainType *tmp = (ChainType *)realloc(c->chains, new_cap * sizeof(ChainType));
    if (!tmp) return false;
    c->chains = tmp;
    c->capacity = new_cap;
    return true;
}

/**
 * @brief Pull the next element through the pipeline [HOT PATH]
 *
 * Pulls the next element from c->base via the container's own next()
 * and runs it through every recorded chain in order.
 *
 * Filter chains drop elements that fail their predicate.
 * Map chains transform the element in place using the chain buffer.
 * Skip chains discard the first N elements.
 * Take chains stop iteration after M elements have been emitted.
 *
 * @param it Iterator cursor for the container (initialised with Iter(c->base))
 * @param c  Chainer containing the pipeline
 * @return The first element that survives all chains, or NULL when
 *         the source is exhausted or a take chain limit is reached
 */
static inline const void *_chain_fused_next(Iterator *it, const Chainer *c) {
    const void *entry;
    
    while ((entry = iter_next(it))) {
        const void *curr = entry;
        bool dropped = false;
        
        for (size_t i = 0; i < c->len; i++) {
            ChainType *ch = &c->chains[i];

            switch (ch->kind) {
                case CHAIN_FILTER:
                    if (!ch->pred(c->base, curr)) { 
                        curr = NULL; 
                        dropped = true;
                    }
                    break;

                case CHAIN_MAP:
                    if (curr) {
                        curr = ch->map(c->base, curr, ch->buffer);
                        if (!curr) dropped = true;
                    } else {
                        dropped = true;
                    }
                    break;

                case CHAIN_SKIP:
                    if (ch->curr_count) {
                        ch->curr_count--;
                        curr = NULL;
                        dropped = true;
                    }
                    break;

                case CHAIN_TAKE:
                    if (!ch->curr_count) return NULL;
                    ch->curr_count--;
                    break;
            }
            if (dropped) break;
        }
        if (dropped) continue;
        return curr;
    }
    return NULL;
}

/** @} */ /* end of chainer_internal group */

/**
 * @defgroup chainer_api Public API
 * @brief Chainer initialization, management, builders, and terminals
 * @{
 */

/**
 * @defgroup chainer_init Initialization
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
    return (Chainer) {
        .base = base,
        .chains = NULL,
        .len = 0,
        .capacity = 0
    };
}

/**
 * @brief Destroy a Chainer and free all resources
 *
 * Frees all chain resources: output buffers for MAP chains and
 * the chains array itself. Resets len, capacity, and chains to zero/NULL.
 *
 * @param c Chainer to destroy (can be NULL)
 */
static inline void chain_destroy(Chainer *c) {
    if (!c) return;

    for (size_t i = 0; i < c->len; i++) {
        free(c->chains[i].buffer);
    }

    free(c->chains);
    c->chains = NULL;
    c->len = 0;
    c->capacity = 0;
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
 *   chain_skip(&c, 2);
 *   chain_take(&c, 5);
 *   chain_count(&c);        // runs on vec1
 *   chain_bind(&c, vec2);
 *   chain_count(&c);        // runs on vec2 with same pipeline
 *   chain_destroy(&c);
 * @endcode
 */
static inline void chain_bind(Chainer *c, const Container *new_base) {
    if (!c) return;
    c->base = new_base;
}

/** @} */ /* end of chainer_init group */

/**
 * @defgroup chainer_builders Chain Builders
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
    if (_chain_grow(c)) {
        ChainType *ch = &c->chains[c->len++];
        ch->kind = CHAIN_FILTER;
        ch->pred = pred;
        ch->buffer = NULL;
    }
    return c;
}

/**
 * @brief Append a map chain to the pipeline
 *
 * Every element that reaches this chain is transformed by map, which writes
 * its result into an internally allocated buffer of stride bytes.
 *
 * @param c      Chainer to modify
 * @param map    Map function; receives container, element, and output buffer.
 *               Must write result to buffer and return it, or return NULL to drop.
 * @param stride Size in bytes of the output element type. Must be greater than zero.
 * @return c (for call chaining)
 *
 * @note The buffer is reused across elements — it holds only the most
 *       recently mapped value.
 *
 * @note The map function may return NULL to drop the element, acting as
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
static inline Chainer *chain_map(Chainer *c, void *(*map)(const Container *, const void *, void *),
    size_t stride) {
    if (_chain_grow(c)) {
        void *buf = malloc(stride);
        if (buf) {
            ChainType *ch = &c->chains[c->len++];
            ch->kind = CHAIN_MAP;
            ch->map = map;
            ch->buffer = buf;
        }
    }
    return c;
}

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
 *   chain_skip(&c, 3);    // skip first 3 elements
 *   chain_take(&c, 5);    // take next 5
 *   // Result: elements 3-7 from source
 * @endcode
 */
static inline Chainer *chain_skip(Chainer *c, uint32_t n) {
    if (_chain_grow(c)) {
        ChainType *ch = &c->chains[c->len++];
        ch->kind = CHAIN_SKIP;
        ch->init_count = n;
        ch->curr_count = n;
        ch->buffer = NULL;
    }
    return c;
}

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
 *   chain_filter(&c, is_even);
 *   chain_take(&c, 5);    // take first 5 even numbers
 * @endcode
 */
static inline Chainer *chain_take(Chainer *c, uint32_t n) {
    if (_chain_grow(c)) {
        ChainType *ch = &c->chains[c->len++];
        ch->kind = CHAIN_TAKE;
        ch->init_count = n;
        ch->curr_count = n;
        ch->buffer = NULL;
    }
    return c;
}

/** @} */ /* end of chainer_builders group */

/**
 * @defgroup chainer_terminals Terminals
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

/** @} */ /* end of chainer_terminals group */

/** @} */ /* end of chainer_api group */

#endif /* CHAINER_PDR_H */