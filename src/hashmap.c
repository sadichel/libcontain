/* ============================================================================
 * HashMap — Implementation
 * ============================================================================
 *
 * Important Notes:
 *   • Uses separate chaining with linked buckets for collision resolution
 *   • Load factor triggers: expand at 0.75, shrink at 0.25
 *   • All capacities are powers of two for fast bitmask indexing
 *   • String mode (key_size == 0 or val_size == 0) stores char* pointers
 *   • Pool allocator manages entries in contiguous blocks
 *   • Cached hash invalidated on any mutation
 *   • HashMapPair provides convenient key/value access during iteration
 * ============================================================================ */

#include <contain/hashmap.h>

/* -------------------------------------------------------------------------
 * Container vtable
 * ------------------------------------------------------------------------- */
static const void *hashmap_next(Iterator *iter);
static Container *hashmap_instance_adpt(const Container *map);
static Container *hashmap_clone_adpt(const Container *map);
static Array *hashmap_to_array_adpt(const Container *map);
static size_t hashmap_hash_adpt(const Container *map);
static bool hashmap_insert_adpt(Container *map, const void *pair);
static void hashmap_clear_adpt(Container *map);
static void hashmap_destroy_adpt(Container *map);

static const ContainerVTable HASHMAP_CONTAINER_OPS = {
    .next     = hashmap_next,
    .instance = hashmap_instance_adpt,
    .clone    = hashmap_clone_adpt,
    .as_array = hashmap_to_array_adpt,
    .hash     = hashmap_hash_adpt,
    .insert   = hashmap_insert_adpt,
    .clear    = hashmap_clear_adpt,
    .destroy  = hashmap_destroy_adpt
};

/* -------------------------------------------------------------------------
 * Internal structures
 * ------------------------------------------------------------------------- */
struct HashMapImpl {
    uint16_t key_offset;
    uint16_t val_offset;
    uint16_t key_size;
    uint16_t val_size;
    uint32_t stride;
    size_t cached_hash;
};

/* Hash entry stored in buckets */
typedef struct HashMapEntry {
    struct HashMapEntry *next;
    uint8_t data[];          /* flexible array for key+value storage */
} HashMapEntry;

/* Layout calculation result */
typedef struct {
    uint16_t key_offset;
    uint16_t val_offset;
    uint32_t stride;
} HashMapEntryLayout;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Create new entry with key and value */
static HashMapEntry *hashmap_entry_create(HashMap *map, const void *key, const void *val) {
    HashMapEntry *entry = (HashMapEntry *)allocator_alloc(map->alloc);
    if (!entry) return NULL;
    
    HashMapImpl *impl = (HashMapImpl *)map->impl;
    void *key_slot = lc_slot_at(entry->data, impl->key_offset);
    void *val_slot = lc_slot_at(entry->data, impl->val_offset);
    
    if (lc_slot_init(key_slot, key, impl->key_size) != LC_OK) {
        allocator_free(map->alloc, entry);
        return NULL;
    }

    if (lc_slot_init(val_slot, val, impl->val_size) != LC_OK) {
        lc_slot_free(key_slot, impl->key_size);
        allocator_free(map->alloc, entry);
        return NULL;
    }

    entry->next = NULL;
    impl->cached_hash = 0;
    map->container.len++;

    return entry;
}

/* Free entry and its contents */
static void hashmap_entry_free(HashMap *map, HashMapEntry *entry) {
    HashMapImpl *impl = (HashMapImpl *)map->impl;
    
    void *key_slot = lc_slot_at(entry->data, impl->key_offset);
    void *val_slot = lc_slot_at(entry->data, impl->val_offset);
    
    lc_slot_free(key_slot, impl->key_size);
    lc_slot_free(val_slot, impl->val_size);

    impl->cached_hash = 0;
    allocator_free(map->alloc, entry);
    map->container.len--;
}

/* Rehash entire map to new capacity (must be power of two) */
static int hashmap_rehash(HashMap *map, size_t new_capacity) {
    HashMapEntry **new_buckets = (HashMapEntry **)calloc(new_capacity, sizeof(HashMapEntry *));
    if (!new_buckets) return LC_ENOMEM;

    HashMapEntry **old_buckets = (HashMapEntry **)map->container.items;
    const HashMapImpl *impl = map->impl;

    const size_t old_cap = map->container.capacity;
    const size_t mask = new_capacity - 1;
    const size_t key_off = impl->key_offset;
    const size_t key_sz = impl->key_size;
    const lc_Hasher hash_fn = map->khash;

    for (size_t i = 0; i < old_cap; i++) {
        HashMapEntry *entry = old_buckets[i];
        while (entry) {
            HashMapEntry *next = entry->next;

            void *e_key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
            size_t b = lc_slot_hash(e_key, key_sz, hash_fn) & mask;
        
            entry->next = new_buckets[b];
            new_buckets[b] = entry;
            
            entry = next;
        }
    }

    free(old_buckets);
    map->container.items = (void **)new_buckets;
    map->container.capacity = new_capacity;

    return LC_OK;
}

/* Adjust capacity based on load factor */
static int hashmap_adjust_capacity(HashMap *map, bool allow_shrink) {
    size_t len = map->container.len;
    size_t cap = map->container.capacity;

    /* Expand: load > 0.75  =>  len * 4 > cap * 3 */
    if (len * 4 > cap * 3) {
        size_t target = cap * 2;
        if (target < cap) return LC_EOVERFLOW;
        if (target > HASHMAP_MIN_CAPACITY && target < cap) return LC_EOVERFLOW;
        return hashmap_rehash(map, target);
    }
    /* Shrink: only if allowed, and load < 0.25  =>  len * 4 < cap */
    else if (allow_shrink && cap > HASHMAP_MIN_CAPACITY && (len * 4 < cap)) {
        size_t target = cap / 2;
        /* Ensure target doesn't go below minimum */
        if (target < HASHMAP_MIN_CAPACITY) target = HASHMAP_MIN_CAPACITY;
        return hashmap_rehash(map, target);
    }

    return LC_OK;
}

/* Compute entry layout: key offset, value offset, total stride */
static bool hashmap_compute_layout(size_t key_sz, size_t key_align, 
                                    size_t val_sz, size_t val_align,
                                    HashMapEntryLayout *out) {
    if (key_align == 0 || (key_align & (key_align - 1)) != 0) return false;
    if (val_align == 0 || (val_align & (val_align - 1)) != 0) return false;
    
    const size_t ptr_size = sizeof(void *);

    /* String mode: store as char* pointers */
    if (key_sz == 0) { key_sz = ptr_size; key_align = ptr_size; }
    if (val_sz == 0) { val_sz = ptr_size; val_align = ptr_size; }

    /* Use size as alignment if default alignment is 1 */
    if (key_align == 1 && key_sz > 1) key_align = key_sz;
    if (val_align == 1 && val_sz > 1) val_align = val_sz;

    /* Ensure power of two */
    key_align = lc_bit_floor(key_align);
    val_align = lc_bit_floor(val_align);

    /* Key starts after next pointer, aligned to key_align */
    size_t key_start = (ptr_size + key_align - 1) & ~(key_align - 1);
    
    /* Value starts after key, aligned to val_align */
    size_t key_end = key_start + key_sz;
    size_t val_start = (key_end + val_align - 1) & ~(val_align - 1);
    
    /* Total stride = val_start + val_size */
    size_t stride = val_start + val_sz;

    /* Offsets relative to entry->data (which starts after next pointer) */
    size_t key_off = key_start - ptr_size;
    size_t val_off = val_start - ptr_size;

    if (key_off > UINT16_MAX || val_off > UINT16_MAX || stride > UINT32_MAX) {
        return false;
    }

    out->key_offset = (uint16_t)key_off;
    out->val_offset = (uint16_t)val_off;
    out->stride = (uint32_t)stride;

    return true;
}

/* -------------------------------------------------------------------------
 * Creation helpers
 * ------------------------------------------------------------------------- */
typedef struct {
    HashMap *map;
    void **buckets;
    Allocator *alloc;
} HashMapCreateCtx;

static void hashmap_create_ctx_cleanup(HashMapCreateCtx *ctx) {
    if (ctx->alloc) allocator_destroy(ctx->alloc);
    if (ctx->buckets) free(ctx->buckets);
    if (ctx->map) free(ctx->map);
}

static HashMap *hashmap_create_impl(const HashMapBuilder *cfg, const HashMapEntryLayout *layout) {
    HashMapCreateCtx ctx = {0};

    ctx.map = (HashMap *)malloc(sizeof(HashMap) + sizeof(HashMapImpl));
    if (!ctx.map) return NULL;

    size_t capacity = lc_bit_ceil(cfg->capacity);
    ctx.buckets = (void **)calloc(capacity, sizeof(void *));
    if (!ctx.buckets) {
        hashmap_create_ctx_cleanup(&ctx);
        return NULL;
    }

    if (cfg->alloc) {
        ctx.alloc = allocator_ref(cfg->alloc);
    } else {
        size_t max_align = cfg->key_align > cfg->val_align ? cfg->key_align : cfg->val_align;
        size_t alloc_align = max_align > HASHMAP_ALLOC_ALIGN ? max_align : HASHMAP_ALLOC_ALIGN;
        ctx.alloc = pool_allocator_create(layout->stride, HASHMAP_ALLOC_CAPACITY, alloc_align, GROW_GOLDEN);
    }
    
    if (!ctx.alloc) {
        hashmap_create_ctx_cleanup(&ctx);
        return NULL;
    }

    HashMapImpl *impl = (HashMapImpl *)(ctx.map + 1);
    impl->key_offset = layout->key_offset;
    impl->val_offset = layout->val_offset;
    impl->key_size = (uint16_t)cfg->key_size;
    impl->val_size = (uint16_t)cfg->val_size;
    impl->stride = layout->stride;
    impl->cached_hash = 0;

    ctx.map->container.items = ctx.buckets;
    ctx.map->container.len = 0;
    ctx.map->container.capacity = capacity;
    ctx.map->container.ops = &HASHMAP_CONTAINER_OPS;
    ctx.map->alloc = ctx.alloc;
    ctx.map->kcmp = cfg->kcmp;
    ctx.map->vcmp = cfg->vcmp;
    ctx.map->khash = cfg->khash;
    ctx.map->impl = impl;

    return ctx.map;
}

/* Create new map from existing map's type */
static HashMap *hashmap_create_from_impl(const HashMap *src, size_t capacity, bool share_alloc) {
    if (!src) return NULL;

    HashMapCreateCtx ctx = {0};

    ctx.map = (HashMap *)malloc(sizeof(HashMap) + sizeof(HashMapImpl));
    if (!ctx.map) return NULL;
    
    if (capacity < HASHMAP_MIN_CAPACITY) capacity = HASHMAP_MIN_CAPACITY;
    capacity = lc_bit_ceil(capacity);

    ctx.buckets = (void **)calloc(capacity, sizeof(void *));
    if (!ctx.buckets) {
        hashmap_create_ctx_cleanup(&ctx);
        return NULL;
    }

    const HashMapImpl *src_impl = src->impl;
    
    if (share_alloc) {
        ctx.alloc = allocator_ref(src->alloc);
    } else {
        ctx.alloc = pool_allocator_create(src_impl->stride, HASHMAP_ALLOC_CAPACITY,
                                          allocator_alignment(src->alloc), GROW_GOLDEN);
    }
    
    if (!ctx.alloc) {
        hashmap_create_ctx_cleanup(&ctx);
        return NULL;
    }

    HashMapImpl *impl = (HashMapImpl *)(ctx.map + 1);
    impl->key_offset = src_impl->key_offset;
    impl->val_offset = src_impl->val_offset;
    impl->key_size = src_impl->key_size;
    impl->val_size = src_impl->val_size;
    impl->stride = src_impl->stride;
    impl->cached_hash = 0;

    ctx.map->container.items = ctx.buckets;
    ctx.map->container.len = 0;
    ctx.map->container.capacity = capacity;
    ctx.map->container.ops = &HASHMAP_CONTAINER_OPS;
    ctx.map->alloc = ctx.alloc;
    ctx.map->kcmp = src->kcmp;
    ctx.map->vcmp = src->vcmp;
    ctx.map->khash = src->khash;
    ctx.map->impl = impl;

    return ctx.map;
}

/* -------------------------------------------------------------------------
 * Builder API
 * ------------------------------------------------------------------------- */
HashMapBuilder hashmap_builder(size_t key_size, size_t val_size) {
    return (HashMapBuilder){
        .key_size = key_size,
        .val_size = val_size,
        .key_align = 1,
        .val_align = 1,
        .capacity = HASHMAP_MIN_CAPACITY,
        .alloc = NULL,
        .kcmp = NULL,
        .vcmp = NULL,
        .khash = NULL,
    };
}

HashMapBuilder hashmap_builder_str_str(void) {
    return hashmap_builder(0, 0);
}

HashMapBuilder hashmap_builder_str_any(size_t val_size) {
    return hashmap_builder(0, val_size);
}

HashMapBuilder hashmap_builder_any_str(size_t key_size) {
    return hashmap_builder(key_size, 0);
}

HashMapBuilder hashmap_builder_capacity(HashMapBuilder b, size_t cap) {
    b.capacity = cap;
    return b;
}

HashMapBuilder hashmap_builder_hasher(HashMapBuilder b, lc_Hasher hasher) {
    b.khash = hasher;
    return b;
}

HashMapBuilder hashmap_builder_comparators(HashMapBuilder b, lc_Comparator kcmp, lc_Comparator vcmp) {
    b.kcmp = kcmp;
    b.vcmp = vcmp;
    return b;
}

HashMapBuilder hashmap_builder_alignment(HashMapBuilder b, size_t key_align, size_t val_align) {
    b.key_align = key_align;
    b.val_align = val_align;
    return b;
}

HashMapBuilder hashmap_builder_allocator(HashMapBuilder b, Allocator *alloc) {
    b.alloc = alloc;
    return b;
}

HashMap *hashmap_builder_build(HashMapBuilder b) {
    if (b.capacity == 0) return NULL;
    
    HashMapEntryLayout layout;
    if (!hashmap_compute_layout(b.key_size, b.key_align, b.val_size, b.val_align, &layout)) {
        return NULL;
    }

    if (b.alloc) {
        if (allocator_stride(b.alloc) < layout.stride) return NULL;
    }

    return hashmap_create_impl(&b, &layout);
}

/* -------------------------------------------------------------------------
 * Public creation functions
 * ------------------------------------------------------------------------- */
HashMap *hashmap_create(size_t key_size, size_t val_size) {
    return hashmap_builder_build(hashmap_builder(key_size, val_size));
}

HashMap *hashmap_create_with_capacity(size_t key_size, size_t val_size, size_t cap) {
    return hashmap_builder_build(hashmap_builder_capacity(hashmap_builder(key_size, val_size), cap));
}

HashMap *hashmap_create_with_hasher(size_t key_size, size_t val_size, lc_Hasher hasher) {
    return hashmap_builder_build(hashmap_builder_hasher(hashmap_builder(key_size, val_size), hasher));
}

HashMap *hashmap_create_with_comparator(size_t key_size, size_t val_size, lc_Comparator kcmp, lc_Comparator vcmp) {
    return hashmap_builder_build(hashmap_builder_comparators(hashmap_builder(key_size, val_size), kcmp, vcmp));
}

HashMap *hashmap_create_with_allocator(size_t key_size, size_t val_size, Allocator *alloc) {
    return hashmap_builder_build(hashmap_builder_allocator(hashmap_builder(key_size, val_size), alloc));
}

HashMap *hashmap_create_aligned(size_t key_size, size_t val_size, size_t key_align, size_t val_align) {
    return hashmap_builder_build(hashmap_builder_alignment(hashmap_builder(key_size, val_size), key_align, val_align));
}

HashMap *hashmap_str_any(size_t val_size) {
    return hashmap_builder_build(hashmap_builder_str_any(val_size));
}

HashMap *hashmap_str_any_with_capacity(size_t val_size, size_t cap) {
    return hashmap_builder_build(hashmap_builder_capacity(hashmap_builder_str_any(val_size), cap));
}

HashMap *hashmap_str_any_with_hasher(size_t val_size, lc_Hasher hasher) {
    return hashmap_builder_build(hashmap_builder_hasher(hashmap_builder_str_any(val_size), hasher));
}

HashMap *hashmap_str_any_with_comparator(size_t val_size, lc_Comparator vcmp) {
    return hashmap_builder_build(hashmap_builder_comparators(hashmap_builder_str_any(val_size), NULL, vcmp));
}

HashMap *hashmap_str_any_with_allocator(size_t val_size, Allocator *alloc) {
    return hashmap_builder_build(hashmap_builder_allocator(hashmap_builder_str_any(val_size), alloc));
}

HashMap *hashmap_str_any_aligned(size_t val_size, size_t val_align) {
    return hashmap_builder_build(hashmap_builder_alignment(hashmap_builder_str_any(val_size), 1, val_align));
}

HashMap *hashmap_any_str(size_t key_size) {
    return hashmap_builder_build(hashmap_builder_any_str(key_size));
}

HashMap *hashmap_any_str_with_capacity(size_t key_size, size_t cap) {
    return hashmap_builder_build(hashmap_builder_capacity(hashmap_builder_any_str(key_size), cap));
}

HashMap *hashmap_any_str_with_hasher(size_t key_size, lc_Hasher hasher) {
    return hashmap_builder_build(hashmap_builder_hasher(hashmap_builder_any_str(key_size), hasher));
}

HashMap *hashmap_any_str_with_comparator(size_t key_size, lc_Comparator kcmp) {
    return hashmap_builder_build(hashmap_builder_comparators(hashmap_builder_any_str(key_size), kcmp, NULL));
}

HashMap *hashmap_any_str_with_allocator(size_t key_size, Allocator *alloc) {
    return hashmap_builder_build(hashmap_builder_allocator(hashmap_builder_any_str(key_size), alloc));
}

HashMap *hashmap_any_str_aligned(size_t key_size, size_t key_align) {
    return hashmap_builder_build(hashmap_builder_alignment(hashmap_builder_any_str(key_size), key_align, 1));
}

HashMap *hashmap_str_str(void) {
    return hashmap_builder_build(hashmap_builder_str_str());
}

HashMap *hashmap_str_str_with_capacity(size_t cap) {
    return hashmap_builder_build(hashmap_builder_capacity(hashmap_builder_str_str(), cap));
}

HashMap *hashmap_str_str_with_hasher(lc_Hasher hasher) {
    return hashmap_builder_build(hashmap_builder_hasher(hashmap_builder_str_str(), hasher));
}

HashMap *hashmap_str_str_with_comparator(lc_Comparator cmp) {
    return hashmap_builder_build(hashmap_builder_comparators(hashmap_builder_str_str(), cmp, cmp));
}

HashMap *hashmap_str_str_with_allocator(Allocator *alloc) {
    return hashmap_builder_build(hashmap_builder_allocator(hashmap_builder_str_str(), alloc));
}

/* -------------------------------------------------------------------------
 * Destruction & configuration
 * ------------------------------------------------------------------------- */
void hashmap_destroy(HashMap *map) {
    if (!map) return;

    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
    size_t cap = map->container.capacity;
    
    for (size_t i = 0; i < cap; i++) {
        HashMapEntry *entry = buckets[i];
        while (entry) {
            HashMapEntry *next = entry->next;
            hashmap_entry_free(map, entry);
            entry = next;
        }
    }

    free(buckets);
    allocator_destroy(map->alloc);
    free(map);
}

int hashmap_set_hasher(HashMap *map, lc_Hasher hasher) {
    if (!map || !hasher) return LC_EINVAL;
    if (map->container.len != 0) return LC_EBUSY;
    map->khash = hasher;
    return LC_OK;
}

int hashmap_set_comparator(HashMap *map, lc_Comparator kcmp, lc_Comparator vcmp) {
    if (!map || !kcmp || !vcmp) return LC_EINVAL;
    if (map->container.len != 0) return LC_EBUSY;
    map->kcmp = kcmp;
    map->vcmp = vcmp;
    return LC_OK;
}

int hashmap_set_allocator(HashMap *map, Allocator *alloc) {
    if (!map || !alloc) return LC_EINVAL;
    if (map->container.len != 0) return LC_EBUSY;
    
    if (allocator_stride(alloc) < map->impl->stride)
        return LC_ETYPE;

    Allocator *old = map->alloc;
    map->alloc = allocator_ref(alloc);
    allocator_destroy(old);
    return LC_OK;
}

/* -------------------------------------------------------------------------
 * Core operations
 * ------------------------------------------------------------------------- */

/* Compute bucket index for a key */
static inline size_t hashmap_bucket(const HashMap *map, const void *key) {
    const HashMapImpl *impl = map->impl;
    size_t h = lc_slot_hash(key, impl->key_size, map->khash);
    return h & (map->container.capacity - 1);
}

/* Get value from specific bucket */
static void *hashmap_get_impl(const HashMap *map, const void *key, size_t bucket) {
    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
    
    const HashMapImpl *impl = map->impl;
    const size_t key_off = impl->key_offset;
    const size_t val_off = impl->val_offset;
    const size_t key_sz = impl->key_size;
    const lc_Comparator kcmp = map->kcmp;

    for (HashMapEntry *entry = buckets[bucket]; entry; entry = entry->next) {
        void *e_key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
        
        if (lc_slot_cmp(e_key, key, key_sz, kcmp) == 0) {
            return lc_slot_at(entry->data, val_off);
        }
    }
    
    return NULL;
}

const void *hashmap_get(const HashMap *map, const void *key) {
    if (!map || !key) return NULL;
    void *val_slot = hashmap_get_impl(map, key, hashmap_bucket(map, key));
    return  val_slot ? lc_slot_get(val_slot, map->impl->val_size) : NULL;
}

const void *hashmap_get_or_default(const HashMap *map, const void *key, const void *dval) {
    if (!map || !key) return dval;
    void *val_slot = hashmap_get_impl(map, key, hashmap_bucket(map, key));
    return val_slot ? lc_slot_get(val_slot, map->impl->val_size) : dval;
}

void *hashmap_get_mut(const HashMap *map, const void *key) {
    if (!map || !key) return NULL;
    return hashmap_get_impl(map, key, hashmap_bucket(map, key));
}

void *hashmap_get_mut_or_default(const HashMap *map, const void *key, const void *dval) {
    if (!map || !key) return (void *)dval;
    void *val_slot = hashmap_get_impl(map, key, hashmap_bucket(map, key));
    return val_slot ? val_slot : (void *)dval;
}

/* Insert into specific bucket */
static int hashmap_insert_impl(HashMap *map, const void *key, const void *val, size_t bucket) {
    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
    
    HashMapImpl *impl = (HashMapImpl *)map->impl;
    const size_t key_off = impl->key_offset;
    const size_t val_off = impl->val_offset;
    const size_t key_sz = impl->key_size;
    const size_t val_sz = impl->val_size;
    const lc_Comparator kcmp = map->kcmp;
    const lc_Comparator vcmp = map->vcmp;

    for (HashMapEntry *entry = buckets[bucket]; entry; entry = entry->next) {
        void *e_key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
        
        if (lc_slot_cmp(e_key, key, key_sz, kcmp) == 0) {
            void *val_slot = lc_slot_at(entry->data, val_off);
            void *e_val = lc_slot_get(val_slot, val_sz);
            
            if (lc_slot_cmp(e_val, val, val_sz, vcmp) == 0) 
                return LC_OK;

            int rc = lc_slot_set(val_slot, val, val_sz);
            if (rc == LC_OK) 
                impl->cached_hash = 0;
            return rc;
        }
    }

    HashMapEntry *new_node = hashmap_entry_create(map, key, val);
    if (!new_node) return LC_ENOMEM;

    new_node->next = buckets[bucket];
    buckets[bucket] = new_node;

    return LC_OK;
}

int hashmap_insert(HashMap *map, const void *key, const void *val) {
    if (!map || !key || !val) return LC_EINVAL;
    int rc = hashmap_insert_impl(map, key, val, hashmap_bucket(map, key));
    if (rc != LC_OK) return rc;
    return hashmap_adjust_capacity(map, false);
}

/* Remove from specific bucket */
static int hashmap_remove_impl(HashMap *map, const void *key, const void *val, size_t bucket) {
    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
    
    const HashMapImpl *impl = map->impl;
    const size_t key_off = impl->key_offset;
    const size_t val_off = impl->val_offset;
    const size_t key_sz = impl->key_size;
    const size_t val_sz = impl->val_size;
    const lc_Comparator kcmp = map->kcmp;
    const lc_Comparator vcmp = map->vcmp;

    HashMapEntry *prev = NULL;

    for (HashMapEntry *entry = buckets[bucket]; entry; entry = entry->next) {
        void *e_key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
        
        if (lc_slot_cmp(e_key, key, key_sz, kcmp) == 0) {
            if (val) {
                void *e_val = lc_slot_get(lc_slot_at(entry->data, val_off), val_sz);
                if (lc_slot_cmp(e_val, val, val_sz, vcmp) != 0)
                    return LC_ENOTFOUND;
            }

            if (prev) 
                prev->next = entry->next;
            else 
                buckets[bucket] = entry->next;

            hashmap_entry_free(map, entry);
            return LC_OK;
        }
        prev = entry;
    }

    return LC_ENOTFOUND;
}

int hashmap_remove(HashMap *map, const void *key) {
    if (!map || !key) return LC_EINVAL;
    int rc = hashmap_remove_impl(map, key, NULL, hashmap_bucket(map, key));
    if (rc != LC_OK) return rc;
    return hashmap_adjust_capacity(map, true);
}

int hashmap_remove_entry(HashMap *map, const void *key, const void *val) {
    if (!map || !key || !val) return LC_EINVAL;
    int rc = hashmap_remove_impl(map, key, val, hashmap_bucket(map, key));
    if (rc != LC_OK) return rc;
    return hashmap_adjust_capacity(map, true);
}

/* Contains checks */
static bool hashmap_contains_impl(const HashMap *map, const void *key, const void *val, size_t bucket) {
    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
    
    const HashMapImpl *impl = map->impl;
    const size_t key_off = impl->key_offset;
    const size_t val_off = impl->val_offset;
    const size_t key_sz = impl->key_size;
    const size_t val_sz = impl->val_size;
    const lc_Comparator kcmp = map->kcmp;
    const lc_Comparator vcmp = map->vcmp;

    for (HashMapEntry *entry = buckets[bucket]; entry; entry = entry->next) {
        void *e_key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
       
        if (lc_slot_cmp(e_key, key, key_sz, kcmp) == 0) {
            if (val) {
                void *e_val = lc_slot_get(lc_slot_at(entry->data, val_off), val_sz);
                return lc_slot_cmp(e_val, val, val_sz, vcmp) == 0;
            }
            return true;
        }
    }

    return false;
}

bool hashmap_contains(const HashMap *map, const void *key) {
    if (!map || !key) return false;
    return hashmap_contains_impl(map, key, NULL, hashmap_bucket(map, key));
}

bool hashmap_contains_entry(const HashMap *map, const void *key, const void *val) {
    if (!map || !key || !val) return false;
    return hashmap_contains_impl(map, key, val, hashmap_bucket(map, key));
}

/* Populate destination map from source */
static int hashmap_populate(HashMap *dst, const HashMap *src, bool check_duplicates) {
    HashMapEntry **dst_buckets = (HashMapEntry **)dst->container.items;
    HashMapEntry **src_buckets = (HashMapEntry **)src->container.items;
    
    const HashMapImpl *impl = src->impl;
    const size_t src_cap = src->container.capacity;
    const size_t key_off = impl->key_offset;
    const size_t val_off = impl->val_offset;
    const size_t key_sz = impl->key_size;
    const size_t val_sz = impl->val_size;
    const size_t dst_mask = dst->container.capacity - 1;
    const lc_Hasher khash = dst->khash;

    for (size_t i = 0; i < src_cap; i++) {
        for (HashMapEntry *entry = src_buckets[i]; entry; entry = entry->next) {
            void *key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
            void *val = lc_slot_get(lc_slot_at(entry->data, val_off), val_sz);
            size_t b = lc_slot_hash(key, key_sz, khash) & dst_mask;
            
            int rc;
            if (check_duplicates) {
                rc = hashmap_insert_impl(dst, key, val, b);
            } else {
                HashMapEntry *new_node = hashmap_entry_create(dst, key, val);
                if (!new_node) return LC_ENOMEM;
                new_node->next = dst_buckets[b];
                dst_buckets[b] = new_node;
                rc = LC_OK;
            }
            
            if (rc != LC_OK) return rc;
        }
    }
    return LC_OK;
}

/* Merge src into dst */
static int hashmap_merge_impl(HashMap *dst, const HashMap *src) {
    size_t total_len = dst->container.len + src->container.len;
    HashMap *tmp = hashmap_create_from_impl(dst, total_len, true);
    if (!tmp) return LC_ENOMEM;

    if (hashmap_populate(tmp, dst, false) != LC_OK || 
        hashmap_populate(tmp, src, true) != LC_OK) {
        hashmap_destroy(tmp);
        return LC_ENOMEM;
    }
    
    void *tmp_items = tmp->container.items;
    size_t tmp_cap = tmp->container.capacity;
    size_t tmp_len = tmp->container.len;

    tmp->container.items = dst->container.items;
    tmp->container.capacity = dst->container.capacity;
    tmp->container.len = dst->container.len;

    dst->container.items = tmp_items;
    dst->container.capacity = tmp_cap;
    dst->container.len = tmp_len;

    ((HashMapImpl *)dst->impl)->cached_hash = 0;

    hashmap_destroy(tmp);
    return LC_OK;
}

int hashmap_merge(HashMap *dst, const HashMap *src) {
    if (!dst || !src) return LC_EINVAL;
    if (dst == src) return LC_OK;

    if (dst->kcmp != src->kcmp) return LC_ETYPE;

    const HashMapImpl *di = dst->impl;
    const HashMapImpl *si = src->impl;

    if (di->key_size != si->key_size || di->val_size != si->val_size) {
        return LC_ETYPE;
    }

    return hashmap_merge_impl(dst, src);
}

int hashmap_reserve(HashMap *map, size_t expected) {
    if (!map) return LC_EINVAL;
    if (expected == 0) return LC_OK;

    if (expected > (SIZE_MAX / 4) * 3) return LC_EOVERFLOW;
    size_t min_cap = (expected * 4 + 2) / 3;

    if (min_cap <= map->container.capacity) return LC_OK;

    size_t target = lc_bit_ceil(min_cap);
    if (target < min_cap) return LC_EOVERFLOW;

    return hashmap_rehash(map, target);
}

int hashmap_clear(HashMap *map) {
    if (!map) return LC_EINVAL;

    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
    size_t cap = map->container.capacity;

    for (size_t i = 0; i < cap; i++) {
        HashMapEntry *entry = buckets[i];
        while (entry) {
            HashMapEntry *next = entry->next;
            hashmap_entry_free(map, entry);
            entry = next;
        }
        buckets[i] = NULL;
    }

    return LC_OK;
}

int hashmap_shrink_to_fit(HashMap *map) {
    if (!map) return LC_EINVAL;
    return hashmap_adjust_capacity(map, true);
}

/* -------------------------------------------------------------------------
 * Queries
 * ------------------------------------------------------------------------- */
size_t hashmap_len(const HashMap *map) { return map ? map->container.len : 0; }
size_t hashmap_size(const HashMap *map) { return hashmap_len(map); }
size_t hashmap_capacity(const HashMap *map) { return map ? map->container.capacity : 0; }
bool hashmap_is_empty(const HashMap *map) { return !map || map->container.len == 0; }

size_t hashmap_hash(const HashMap *map) {
    if (!map || map->container.len == 0) return 0;

    if (map->impl->cached_hash) return map->impl->cached_hash;
    
    HashMapImpl *impl = (HashMapImpl *)map->impl;
    const size_t cap = map->container.capacity;
    const size_t key_off = impl->key_offset;
    const size_t val_off = impl->val_offset;
    const size_t key_sz = impl->key_size;
    const size_t val_sz = impl->val_size;
    const lc_Hasher hash = map->khash;
    HashMapEntry **buckets = (HashMapEntry **)map->container.items;

    size_t h = 0;
    for (size_t i = 0; i < cap; i++) {
        for (HashMapEntry *entry = buckets[i]; entry; entry = entry->next) {
            void *e_key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
            void *e_val = lc_slot_get(lc_slot_at(entry->data, val_off), val_sz);
            size_t kx = lc_slot_hash(e_key, key_sz, hash);
            size_t vx = lc_slot_hash(e_val, val_sz, NULL);
            h ^= lc_hash_mix(kx ^ lc_hash_mix(vx));
        }
    }

    impl->cached_hash = lc_hash_mix(h ^ map->container.len);
    return impl->cached_hash;
}

static bool hashmap_submap(const HashMap *A, const HashMap *B) {
    if (A->container.len > B->container.len) return false;

    HashMapEntry **a_buckets = (HashMapEntry **)A->container.items;
    
    const HashMapImpl *ai = A->impl;
    const size_t a_cap = A->container.capacity;
    const size_t b_mask = B->container.capacity - 1;
    const size_t key_off = ai->key_offset;
    const size_t val_off = ai->val_offset;
    const size_t key_sz = ai->key_size;
    const size_t val_sz = ai->val_size;
    const lc_Hasher hash = B->khash;

    for (size_t i = 0; i < a_cap; i++) {
        for (HashMapEntry *entry = a_buckets[i]; entry; entry = entry->next) {
            void *key = lc_slot_get(lc_slot_at(entry->data, key_off), key_sz);
            void *val = lc_slot_get(lc_slot_at(entry->data, val_off), val_sz);
            size_t b = lc_slot_hash(key, key_sz, hash) & b_mask;
            
            if (!hashmap_contains_impl(B, key, val, b)) {
                return false;
            }
        }
    }
    
    return true;
}

bool hashmap_equals(const HashMap *A, const HashMap *B) {
    if (!A || !B) return false;
    if (A == B) return true;
    if (A->container.len != B->container.len) return false;

    const HashMapImpl *ai = A->impl;
    const HashMapImpl *bi = B->impl;

    if (ai->key_size != bi->key_size || ai->val_size != bi->val_size) {
        return false;
    }

    if (A->impl->cached_hash != 0 && B->impl->cached_hash != 0 
        && A->impl->cached_hash == B->impl->cached_hash) {
        return true;
    }

    return hashmap_submap(A, B);
}

const void *hashmap_entry_key(const HashMap *map, const void *entry_ptr) {
    if (!map || !entry_ptr) return NULL;
    const HashMapImpl *impl = map->impl;
    return lc_slot_get(lc_slot_at((void *)entry_ptr, impl->key_offset), impl->key_size);
}

const void *hashmap_entry_value(const HashMap *map, const void *entry_ptr) {
    if (!map || !entry_ptr) return NULL;
    const HashMapImpl *impl = map->impl;
    return lc_slot_get(lc_slot_at((void *)entry_ptr, impl->val_offset), impl->val_size);
}

HashMapPair hashmap_entry_pair(const HashMap *map, const void *entry_ptr) {
    return (HashMapPair){
        .key = hashmap_entry_key(map, entry_ptr),
        .value = hashmap_entry_value(map, entry_ptr)
    };
}

void *hashmap_entry_key_mut(const HashMap *map, const void *entry_ptr) {
    if (!map || !entry_ptr) return NULL;
    const HashMapImpl *impl = map->impl;
    return lc_slot_at((void *)entry_ptr, impl->key_offset);
}

void *hashmap_entry_value_mut(const HashMap *map, const void *entry_ptr) {
    if (!map || !entry_ptr) return NULL;
    const HashMapImpl *impl = map->impl;
    return lc_slot_at((void *)entry_ptr, impl->val_offset);
}

Allocator *hashmap_allocator(const HashMap *map) { return map ? map->alloc : NULL; }

/* -------------------------------------------------------------------------
 * Clone
 * ------------------------------------------------------------------------- */
HashMap *hashmap_clone(const HashMap *map) {
    if (!map) return NULL;

    HashMap *clone = hashmap_create_from_impl(map, map->container.capacity, false);
    if (!clone) return NULL;

    if (hashmap_populate(clone, map, false) != LC_OK) {
        hashmap_destroy(clone);
        return NULL;
    }
    
    return clone;
}

HashMap *hashmap_instance(const HashMap *map) {
    if (!map) return NULL;
    return hashmap_create_from_impl(map, HASHMAP_MIN_CAPACITY, false);
}

/* -------------------------------------------------------------------------
 * To Array
 * ------------------------------------------------------------------------- */
static Array *hashmap_collect_fixed(const HashMap *map, size_t item_size, bool want_values) {
    const size_t map_len = map->container.len;
    if (map_len == 0) return NULL;
    
    if (map_len > SIZE_MAX / item_size) return NULL;

    const HashMapImpl *impl = map->impl;
    const size_t target_off = want_values ? impl->val_offset : impl->key_offset;
    const size_t map_cap = map->container.capacity;

    if (map_len > (SIZE_MAX - sizeof(Array)) / item_size) return NULL;

    Array *arr = (Array *)malloc(sizeof(Array) + map_len * item_size);
    if (!arr) return NULL;

    arr->items = (uint8_t *)(arr + 1);
    arr->stride = item_size;
    arr->len = map_len;

    HashMapEntry **buckets = (HashMapEntry **)map->container.items;

    uint8_t *dst = (uint8_t *)arr->items;
    size_t pos = 0;
    
    for (size_t i = 0; i < map_cap; i++) {
        for (HashMapEntry *entry = buckets[i]; entry; entry = entry->next) {
            void *item = lc_slot_at(entry->data, target_off);
            memcpy(dst + (pos++ * item_size), item, item_size);
        }
    }

    return arr;
}

static Array *hashmap_collect_strings(const HashMap *map, bool want_values) {
    const size_t map_len = map->container.len;
    if (map_len == 0) return NULL;
    
    if (map_len > SIZE_MAX / sizeof(char *)) return NULL;
    
    const HashMapImpl *impl = map->impl;
    const size_t target_off = want_values ? impl->val_offset : impl->key_offset;
    const size_t map_cap = map->container.capacity;

    HashMapEntry **buckets = (HashMapEntry **)map->container.items;

    size_t total_chars = 0;
    for (size_t i = 0; i < map_cap; i++) {
        for (HashMapEntry *entry = buckets[i]; entry; entry = entry->next) {
            const char *str = *(const char **)lc_slot_at(entry->data, target_off);
            total_chars += strlen(str) + 1;
        }
    }

    const size_t ptrs_bytes = map_len * sizeof(char *);
    if (ptrs_bytes > SIZE_MAX - sizeof(Array) - total_chars) return NULL;

    Array *arr = (Array *)malloc(sizeof(Array) + ptrs_bytes + total_chars);
    if (!arr) return NULL;

    arr->items = (uint8_t *)(arr + 1);
    arr->stride = 0;
    arr->len = map_len;

    char **table = (char **)arr->items;
    char *pool = (char *)((uint8_t *)arr->items + ptrs_bytes);

    size_t pos = 0;
    for (size_t i = 0; i < map_cap; i++) {
        for (HashMapEntry *entry = buckets[i]; entry; entry = entry->next) {
            const char *str = *(const char **)lc_slot_at(entry->data, target_off);
            size_t len = strlen(str) + 1;

            table[pos++] = pool;
            memcpy(pool, str, len);
            pool += len;
        }
    }

    return arr;
}

Array *hashmap_keys(const HashMap *map) {
    if (!map) return NULL;
    const HashMapImpl *impl = map->impl;
    if (impl->key_size) return hashmap_collect_fixed(map, impl->key_size, false);
    return hashmap_collect_strings(map, false);
}

Array *hashmap_values(const HashMap *map) {
    if (!map) return NULL;
    const HashMapImpl *impl = map->impl;
    if (impl->val_size) return hashmap_collect_fixed(map, impl->val_size, true);
    return hashmap_collect_strings(map, true);
}

/* -------------------------------------------------------------------------
 * Iterator
 * ------------------------------------------------------------------------- */
static const void *hashmap_next(Iterator *it) {
    if (it->direction == ITER_REVERSE) return NULL;
    
    HashMap *map = (HashMap *)it->container;
    HashMapEntry **buckets = (HashMapEntry **)map->container.items;
  
    HashMapEntry *entry = (HashMapEntry *)it->state;

    if (entry && entry->next) {
        entry = entry->next;
    } else {
        entry = NULL;
        while (it->pos < map->container.capacity) {
            entry = buckets[it->pos++];
            if (entry) break;
        }
    }

    it->state = entry;
    return entry ? entry->data : NULL;
}

Iterator hashmap_iter(const HashMap *map) {
    return Iter((const Container *)map);
}

/* -------------------------------------------------------------------------
 * Container adapter functions
 * ------------------------------------------------------------------------- */
static Container *hashmap_instance_adpt(const Container *map) { 
    return (Container *)hashmap_instance((const HashMap *)map); 
}

static Container *hashmap_clone_adpt(const Container *map) { 
    return (Container *)hashmap_clone((const HashMap *)map); 
}

static Array *hashmap_to_array_adpt(const Container *map) { 
    return hashmap_keys((HashMap *)map); 
}

static size_t hashmap_hash_adpt(const Container *map) { 
    return hashmap_hash((const HashMap *)map); 
}

static bool hashmap_insert_adpt(Container *map, const void *pair) {
    const HashMapPair *p = (const HashMapPair *)pair;
    return hashmap_insert((HashMap *)map, p->key, p->value) == LC_OK;
}

static void hashmap_clear_adpt(Container *map) {
    hashmap_clear((HashMap *)map);
}

static void hashmap_destroy_adpt(Container *map) { 
    hashmap_destroy((HashMap *)map); 
}
