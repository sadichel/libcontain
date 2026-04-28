/** @file contain.h
 *  @brief Master include for all libcontain containers
 *  libcontain - https://github.com/sadichel/libcontain 
 *
 *  This header includes all containers at once. Define
 *  LIBCONTAIN_IMPLEMENTATION before including this file to
 *  instantiate all implementations for header-only mode.
 * 
 */

#ifndef LIBCONTAIN_CONTAIN_H
#define LIBCONTAIN_CONTAIN_H

#include "container.h"
#include "vector.h"
#include "deque.h"
#include "linkedlist.h"
#include "hashset.h"
#include "hashmap.h"
#include "iterator.h"
#include "chainer.h"
#include "chainer2.h"
#include "allocator.h"
#include "lc_utils.h"

#endif /* LIBCONTAIN_CONTAIN_H */

/* ----------------------------------------------------------------------------
 * Implementation section
 * ---------------------------------------------------------------------------- */

#ifdef LIBCONTAIN_IMPLEMENTATION

#ifndef VECTOR_IMPLEMENTATION
#define VECTOR_IMPLEMENTATION
#endif
#ifndef DEQUE_IMPLEMENTATION
#define DEQUE_IMPLEMENTATION
#endif
#ifndef LINKEDLIST_IMPLEMENTATION
#define LINKEDLIST_IMPLEMENTATION
#endif
#ifndef HASHSET_IMPLEMENTATION
#define HASHSET_IMPLEMENTATION
#endif
#ifndef HASHMAP_IMPLEMENTATION
#define HASHMAP_IMPLEMENTATION
#endif
#ifndef ALLOCATOR_IMPLEMENTATION
#define ALLOCATOR_IMPLEMENTATION
#endif
#ifndef CHAINER_IMPLEMENTATION
#define CHAINER_IMPLEMENTATION
#endif
#ifndef CHAINER2_IMPLEMENTATION
#define CHAINER2_IMPLEMENTATION
#endif

#include "vector.h"
#include "deque.h"
#include "linkedlist.h"
#include "hashset.h"
#include "hashmap.h"
#include "allocator.h"
#include "chainer.h"
#include "chainer2.h"

#undef VECTOR_IMPLEMENTATION
#undef DEQUE_IMPLEMENTATION
#undef LINKEDLIST_IMPLEMENTATION
#undef HASHSET_IMPLEMENTATION
#undef HASHMAP_IMPLEMENTATION
#undef ALLOCATOR_IMPLEMENTATION
#undef CHAINER_IMPLEMENTATION
#undef CHAINER2_IMPLEMENTATION

#endif /* LIBCONTAIN_IMPLEMENTATION */
