#include "AtomicFunctions.h"
#include "BlockPool.h"
#include "NVMAccesser.h"
#include "PagePool.h"
#include <linux/slab.h>

static struct PageSubPool * CreatePageSubPool(logical_block_t block, int empty)
{
    struct PageSubPool * sp;

    sp = kmalloc(sizeof(struct PageSubPool), GFP_KERNEL);
    if (empty)
    {
        memset(sp->bitmap, 0, sizeof(sp->bitmap));
        sp->count = 512;
    }
    else
    {
        memset(sp->bitmap, -1, sizeof(sp->bitmap));
        sp->count = 0;
    }
    sp->nextIndex = 0;
    INIT_LIST_HEAD(&sp->list);
    sp->block = block;
    return sp;
}

static void DestroyPageSubPool(void * subPool)
{
    kfree(subPool);
}

void PagePoolInit(struct PagePool * pool, struct BlockPool * blockPool, struct NVMAccesser acc)
{
    RadixTreeInit(&pool->tree);
    pool->subPoolNum = 0;
    pool->blockPool = blockPool;
    spin_lock_init(&pool->lock);
    INIT_LIST_HEAD(&pool->nonFull);
    pool->acc = acc;
}

void PagePoolUninit(struct PagePool * pool)
{
    struct PageSubPool *sp, *next;
    list_for_each_entry_safe(sp, next, &pool->nonFull, list)
    {
        list_del(&sp->list);
        RadixTreeSet(&pool->tree, sp->block, NULL);
        DestroyPageSubPool(sp);
    }
    spin_lock_uninit(&pool->lock);
    RadixTreeUninit(&pool->tree, DestroyPageSubPool);
}

static inline unsigned long PagePoolFfz(unsigned long word)
{
    asm("rep; bsf %1,%0" : "=r"(word) : "r"(~word));
    return word;
}

static inline int BitmapZeroGet(UINT64 * bitmap, int startIndex)
{
    int i, j;
    for (i = startIndex, j = 0; j < 8; ++j)
    {
        if (bitmap[i] != -1UL)
        {
            return i * 64 + PagePoolFfz(bitmap[i]);
        }
        ++i;
        if (i >= 8)
            i -= 8;
    }
    return -1;
}

static inline void BitmapSet(UINT64 * bitmap, int index)
{
    UINT64 indexOfUINT64 = index / 64;
    UINT64 indexInsideUINT64 = index % 64;
    bitmap[indexOfUINT64] |= (1UL << indexInsideUINT64);
}

static inline void NextIndexSet(struct PageSubPool * sp, int index)
{
    sp->nextIndex = index / 64;
}

static logical_page_t AllocPageFromPageSubPool(struct PageSubPool * sp)
{
    int index;

    index = BitmapZeroGet(sp->bitmap, sp->nextIndex);
    if (index == -1)
        return invalid_page;
    BitmapSet(sp->bitmap, index);
    NextIndexSet(sp, index);
    sp->count--;
    return index + logical_block_to_page(sp->block);
}

static inline void AddPageSubPool(struct PagePool * pool, struct PageSubPool * sp)
{
    list_add(&pool->nonFull, &sp->list);
    pool->subPoolNum++;
    RadixTreeSet(&pool->tree, sp->block, sp);
}

static struct PageSubPool * PageSubPoolPrepare(struct PagePool * pool)
{
    logical_block_t block;
    struct PageSubPool * sp;

    if (!BlockPoolGet(pool->blockPool, 1, &block))
        return NULL;
    NVMAccesserSplit(&pool->acc, logical_block_to_addr(block));
    NVMAccesserTrim(&pool->acc, logical_block_to_addr(block));
    sp = CreatePageSubPool(block, 1);
    RadixTreeSetPrepare(&pool->tree, block);
    return sp;
}

static void PageSubPoolDeprecate(struct PagePool * pool, struct PageSubPool * sp, int removeFromTree)
{
    NVMAccesserTrim(&pool->acc, logical_block_to_addr(sp->block));
    NVMAccesserMerge(&pool->acc, logical_block_to_addr(sp->block));
    BlockPoolPut(pool->blockPool, 1, &sp->block);
    DestroyPageSubPool(sp);
    if (removeFromTree)
    {
        RadixTreeSet(&pool->tree, sp->block, NULL);
    }
}

static struct PageSubPool * PageSubPoolAllocated(struct PagePool * pool, struct PageSubPool * sp)
{
    if (sp->count == 0)
    {
        RadixTreeSet(&pool->tree, sp->block, NULL);
        list_del(&sp->list);
        pool->subPoolNum--;
        return sp;
    }
    return NULL;
}

logic_addr_t PagePoolAlloc(struct PagePool * pool)
{
    logical_page_t page = invalid_page;
    struct PageSubPool *sp, *newSp, *fullSp;

    sp = newSp = fullSp = NULL;

    spin_lock(&pool->lock);
    if (list_empty(&pool->nonFull))
    {
        spin_unlock(&pool->lock);
        if (!(newSp = PageSubPoolPrepare(pool)))
            goto out;
        spin_lock(&pool->lock);
        if (list_empty(&pool->nonFull))
        {
            AddPageSubPool(pool, newSp);
            newSp = NULL;
        }
    }

    sp = list_entry(pool->nonFull.next, struct PageSubPool, list);
    page = AllocPageFromPageSubPool(sp);
    fullSp = PageSubPoolAllocated(pool, sp);
    spin_unlock(&pool->lock);
out:
    if (newSp)
        PageSubPoolDeprecate(pool, newSp, 0);
    if (fullSp)
        DestroyPageSubPool(fullSp);
    if (page == invalid_page)
        return invalid_nvm_addr;
    return logical_page_to_addr(page);
}

logic_addr_t PagePoolAllocWithHint(struct PagePool * pool, logic_addr_t hint)
{
    logical_page_t page = invalid_page;
    struct PageSubPool *sp, *newSp, *fullSp;

    sp = newSp = fullSp = NULL;

    spin_lock(&pool->lock);
    if (hint != invalid_nvm_addr)
    {
        sp = RadixTreeGet(&pool->tree, logical_addr_to_block(hint));
    }
    if (!sp)
    {
        if (list_empty(&pool->nonFull))
        {
            spin_unlock(&pool->lock);
            if (!(newSp = PageSubPoolPrepare(pool)))
                goto out;
            spin_lock(&pool->lock);
            if (list_empty(&pool->nonFull))
            {
                AddPageSubPool(pool, newSp);
                newSp = NULL;
            }
        }
        sp = list_entry(pool->nonFull.next, struct PageSubPool, list);
    }

    page = AllocPageFromPageSubPool(sp);
    fullSp = PageSubPoolAllocated(pool, sp);
    spin_unlock(&pool->lock);
out:
    if (newSp)
        PageSubPoolDeprecate(pool, newSp, 0);
    if (fullSp)
        DestroyPageSubPool(fullSp);

    if (page == invalid_page)
        return invalid_nvm_addr;
    return logical_page_to_addr(page);
}

static inline int BitmapIndexOfPage(logical_page_t page)
{
    return page & 511;
}

void PagePoolFree(struct PagePool * pool, logic_addr_t addr)
{
    struct PageSubPool *sp, *newSp, *emptySp;
    logical_block_t block;
    int index;
    logical_page_t page = logical_addr_to_page(addr);

    sp = newSp = emptySp = NULL;
    block = logical_page_to_block(page);
    spin_lock(&pool->lock);
    sp = RadixTreeGet(&pool->tree, block);
    if (sp == NULL)
    {
        spin_unlock(&pool->lock);

        newSp = CreatePageSubPool(block, 0);

        spin_lock(&pool->lock);
        sp = RadixTreeGet(&pool->tree, block);
        if (!sp)
        {
            RadixTreeSet(&pool->tree, block, newSp);
            list_add(&newSp->list, &pool->nonFull);
            pool->subPoolNum++;
            sp = newSp;
            newSp = NULL;
        }
    }

    index = BitmapIndexOfPage(page);
    BitmapSet(sp->bitmap, index);
    sp->nextIndex = index;
    sp->count++;

    if (sp->count == 512 && pool->subPoolNum >= 3)
    {
        list_del(&sp->list);
        pool->subPoolNum--;
        RadixTreeSet(&pool->tree, sp->block, NULL);
        emptySp = sp;
    }

    spin_unlock(&pool->lock);
    if (newSp)
    {
        DestroyPageSubPool(newSp);
    }
    if (emptySp)
    {
        PageSubPoolDeprecate(pool, emptySp, 100);
    }
}

void PagePoolRecoveryInit(struct PagePool * pool, struct BlockPool * blockPool, struct NVMAccesser acc)
{
    PagePoolInit(pool, blockPool, acc);
}

void PagePoolRecoveryNotifyPageBusy(struct PagePool * pool, logic_addr_t addr)
{
    struct PageSubPool * sp;
    logical_block_t block;
    int index;
    logical_page_t page = logical_addr_to_page(addr);

    sp = NULL;
    block = logical_page_to_block(page);
    sp = RadixTreeGet(&pool->tree, block);
    if (sp == NULL)
    {
        sp = CreatePageSubPool(block, 1);
        RadixTreeSet(&pool->tree, block, sp);
        list_add(&sp->list, &pool->nonFull);
        pool->subPoolNum++;
        BlockPoolRecoveryNotifyBlockBusy(pool->blockPool, logical_addr_to_block(page), 1);
    }

    index = BitmapIndexOfPage(page);
    BitmapSet(sp->bitmap, index);
    sp->count--;

    if (sp->count == 0)
    {
        list_del(&sp->list);
        pool->subPoolNum--;
        RadixTreeSet(&pool->tree, sp->block, NULL);
        kfree(sp);
    }
}

void PagePoolRecoveryEnd(struct PagePool * pool)
{
}