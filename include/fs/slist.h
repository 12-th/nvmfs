#ifndef SLIST_H
#define SLIST_H

#include "common.h"

struct slist
{
    struct slist * next;
};

static inline void SlistInit(struct slist * head)
{
    head->next = NULL;
}

static inline void SlistReinit(struct slist * head)
{
    head->next = NULL;
}

static inline void SlistAppend(struct slist * prev, struct slist * data)
{
    data->next = prev->next;
    prev->next = data;
}

static inline struct slist * SlistRemove(struct slist * prev)
{
    struct slist * data = prev->next;
    if (data)
    {
        prev->next = data->next;
    }
    return data;
}

#define slist_for_each(head, cur) for (cur = (head)->next; cur != NULL; cur = cur->next)

#define slist_for_each_safe(head, cur, prev, next)                                                                     \
    for (prev = (head), cur = (head)->next, next = cur ? cur->next : NULL; cur != NULL;                                \
         cur = next, next = next ? next->next : NULL, prev = (prev->next == cur) ? prev : prev->next)

#define slist_entry(ptr, type, member) container_of(ptr, type, member)

#endif