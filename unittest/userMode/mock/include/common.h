#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#define ASSERT(expr, ...) assert(expr)
#define STATIC_ASSERT(expr, msg) typedef char static_assertion_##fmt[(expr) ? 1 : -1]

#define NOT_REACH_HERE() ASSERT(FALSE, "program should not reach here\n")

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef bool
#define bool char
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef likely
#define likely(expr) __builtin_expect(!!(expr), 1)
#endif

#ifndef unlikely
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#endif

#define gfp_t int
#define GFP_KERNEL 0
#define __GFP_ZERO 1

#define offsetof(type, member) (unsigned long)(&(((type *)0)->member))
#define container_of(ptr, type, member) (type *)(((unsigned long)ptr) - offsetof(type, member))

#include <errno.h>

#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#endif

#include <string.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE (1UL << 12)
#endif

#define ALWAYS_INLINE __attribute__((always_inline))

#endif