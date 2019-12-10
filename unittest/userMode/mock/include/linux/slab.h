#ifndef LINUX_SLAB_H
#define LINUX_SLAB_H

#include "common.h"
#include <stdlib.h>
#include <sys/mman.h>

struct page
{
};

#define kmalloc(size, gfp) malloc(size)
#define kfree(ptr) free(ptr)

static inline void * kzmalloc(size_t size, gfp_t flags)
{
    void * ptr;

    ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

static inline struct page * alloc_pages(gfp_t flags, unsigned int order)
{
    void * addr =
        mmap(NULL, 1 << (order + 12), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_POPULATE | MAP_PRIVATE, 0, 0);
    if (addr == MAP_FAILED)
    {
        return NULL;
    }
    if (flags & __GFP_ZERO)
    {
        memset(addr, 0, 1 << (order + 12));
    }
    return (struct page *)(addr);
}

static inline struct page * alloc_page(gfp_t flags)
{
    return alloc_pages(flags, 0);
}

static inline void free_pages(unsigned long addr, unsigned int order)
{
    munmap((void *)addr, 1 << (order + 12));
}

static inline void free_page(unsigned long addr)
{
    free_pages(addr, 0);
}

static inline void * __get_free_page(gfp_t flags)
{
    return alloc_page(flags);
}

static inline unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
    return (unsigned long)alloc_pages(gfp_mask, order);
}

static inline void * page_address(struct page * page)
{
    return page;
}

static inline void * __pa(void * addr)
{
    return addr;
}

static inline void * __va(void * addr)
{
    return addr;
}

#endif