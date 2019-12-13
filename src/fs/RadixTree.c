#include "AtomicFunctions.h"
#include "RadixTree.h"
#include "common.h"
#include <linux/slab.h>

void RadixTreeInit(struct RadixTree * root)
{
    root->root = kzalloc(sizeof(struct RadixTreeNode), GFP_KERNEL);
}

static inline UINT64 IndexOfFirstLayer(UINT64 key)
{
    return (key & (0x1ffUL << 18)) >> 18;
}

static inline UINT64 IndexOfSecondLayer(UINT64 key)
{
    return (key & (0x1ffUL << 9)) >> 9;
}

static inline UINT64 IndexOfThirdLayer(UINT64 key)
{
    return key & 0x1ffUL;
}

void RadixTreeSet(struct RadixTree * root, UINT64 key, void * value)
{
    struct RadixTreeNode *second, *third;
    UINT64 index1 = IndexOfFirstLayer(key);
    UINT64 index2 = IndexOfSecondLayer(key);
    UINT64 index3 = IndexOfThirdLayer(key);

    ASSERT(root->root->slots[index1]);
    second = root->root->slots[index1];

    ASSERT(second->slots[index2]);
    third = second->slots[index2];

    third->slots[index3] = value;
}

void RadixTreeSetPrepare(struct RadixTree * root, UINT64 key)
{
    struct RadixTreeNode *second, *third;
    UINT64 index1 = IndexOfFirstLayer(key);
    UINT64 index2 = IndexOfSecondLayer(key);

    if (!root->root->slots[index1])
    {
        second = kzalloc(sizeof(struct RadixTreeNode), GFP_KERNEL);
        if (!BOOL_COMPARE_AND_SWAP(&root->root->slots[index1], NULL, second))
        {
            kfree(second);
        }
    }
    second = root->root->slots[index1];

    if (!second->slots[index2])
    {
        third = kzalloc(sizeof(struct RadixTreeNode), GFP_KERNEL);
        if (!BOOL_COMPARE_AND_SWAP(&second->slots[index2], NULL, third))
        {
            kfree(third);
        }
    }
}

void * RadixTreeGet(struct RadixTree * root, UINT64 key)
{
    struct RadixTreeNode *second, *third;
    UINT64 index1 = IndexOfFirstLayer(key);
    UINT64 index2 = IndexOfSecondLayer(key);
    UINT64 index3 = IndexOfThirdLayer(key);

    if (!root->root->slots[index1])
        return NULL;
    second = root->root->slots[index1];

    if (!second->slots[index2])
        return NULL;
    third = second->slots[index2];

    return third->slots[index3];
}

static void DeleteThirdLayer(struct RadixTreeNode * third, void (*ValueDeleteFunc)(void *))
{
    int i;

    for (i = 0; i < 512; ++i)
    {
        if (third->slots[i])
        {
            ValueDeleteFunc(third->slots[i]);
        }
    }
}

static void DeleteSecondLayer(struct RadixTreeNode * second, void (*ValueDeleteFunc)(void *))
{
    int i;
    for (i = 0; i < 512; ++i)
    {
        if (second->slots[i])
        {
            DeleteThirdLayer(second->slots[i], ValueDeleteFunc);
            kfree(second->slots[i]);
        }
    }
}

static void DeleteFirstLayer(struct RadixTreeNode * first, void (*ValueDeleteFunc)(void *))
{
    int i;
    for (i = 0; i < 512; ++i)
    {
        if (first->slots[i])
        {
            DeleteSecondLayer(first->slots[i], ValueDeleteFunc);
            kfree(first->slots[i]);
        }
    }
}

void RadixTreeUninit(struct RadixTree * root, void (*ValueDeleteFunc)(void *))
{
    DeleteFirstLayer(root->root, ValueDeleteFunc);
    kfree(root->root);
}