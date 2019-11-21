#include "slist.h"
#include "kutest.h"

static unsigned int GetSlistSize(struct slist * head)
{
    struct slist * it;
    unsigned int size = 0;
    slist_for_each(head, it)
    {
        size++;
    }
    return size;
}

TEST(slistTest, appendSingleNodeTest)
{
    struct slist head;
    struct slist n1;
    struct slist * it;
    unsigned int size;

    SlistInit(&head);
    SlistAppend(&head, &n1);
    slist_for_each(&head, it)
    {
        EXPECT_EQ(it, &n1);
    }
    size = GetSlistSize(&head);
    EXPECT_EQ(size, 1);
}

#define MULTI_NODE_COUNT 4
TEST(slistTest, appendMultipleNodeTest)
{
    struct slist head;
    struct slist node[MULTI_NODE_COUNT];
    struct slist * it;
    int i;

    SlistInit(&head);
    for(i = MULTI_NODE_COUNT - 1; i >= 0; --i)
    {
        SlistAppend(&head, &node[i]);
    }

    i = 0;
    slist_for_each(&head, it)
    {
        EXPECT_EQ(it, &node[i]);
        i++;
    }
}

TEST(slistTest, RemoveFromNullListTest)
{
    struct slist head;
    struct slist * data;

    SlistInit(&head);
    data = SlistRemove(&head);

    EXPECT_EQ(data, NULL);
}

TEST(slistTest, RemoveFromOneNodeListTest)
{
    struct slist head;
    struct slist node;
    struct slist * data;

    SlistInit(&head);
    SlistAppend(&head, &node);
    data = SlistRemove(&head);

    EXPECT_EQ(data, &node);
}

TEST(slistTest, RemoveFromMultiNodeListTest)
{
    struct slist head;
    struct slist node[MULTI_NODE_COUNT];
    struct slist * it;
    struct slist * data;
    int i;

    SlistInit(&head);
    for(i = MULTI_NODE_COUNT - 1; i >= 0; --i)
    {
        SlistAppend(&head, &node[i]);
    }
    data = SlistRemove(&head);

    EXPECT_EQ(data, &node[0]);

    i = 1;
    slist_for_each(&head, it)
    {
        EXPECT_EQ(it, &node[i]);
        i++;
    }
}

TEST(slistTest, RemoveAllTest)
{
    struct slist head;
    struct slist node[MULTI_NODE_COUNT];
    struct slist * it;
    struct slist * next;
    struct slist * prev;
    struct slist * data;
    unsigned int size;
    int i;

    SlistInit(&head);
    for(i = MULTI_NODE_COUNT - 1; i >= 0; --i)
    {
        SlistAppend(&head, &node[i]);
    }

    i = 0;
    slist_for_each_safe(&head, it, prev, next)
    {
        data = SlistRemove(prev);
        EXPECT_EQ(data, &node[i]);
        i++;
    }

    size = GetSlistSize(&head);

    EXPECT_EQ(size, 0);
}