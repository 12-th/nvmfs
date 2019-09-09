#ifndef COMMON_H
#define COMMON_H

#ifdef DEBUG
extern void panic(const char * fmt, ...);
#define ASSERT(expr, ...)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            panic("nvmblk error " __VA_ARGS__);                                                                        \
        }                                                                                                              \
    } while (0)
#else
#define ASSERT(expr, ...)
#endif

#define STATIC_ASSERT(expr, msg) typedef char static_assertion_##msg[(expr) ? 1 : -1]

#define NOT_REACH_HERE() ASSERT(FALSE, "program should not reach here\n")

#ifndef ALIGN(x, a)
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#endif

#include <linux/string.h>

#endif