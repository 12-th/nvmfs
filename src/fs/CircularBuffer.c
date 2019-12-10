#include "CircularBuffer.h"
#include <linux/mm.h>
#include <linux/slab.h>

#define CIRCULAR_BUFFER_DEFAULT_SIZE 64

void CircularBufferInit(struct CircularBuffer * cb)
{
    cb->buffer = kvmalloc(sizeof(void *) * CIRCULAR_BUFFER_DEFAULT_SIZE, GFP_KERNEL);
    cb->head = cb->tail = 0;
    cb->size = CIRCULAR_BUFFER_DEFAULT_SIZE;
}

void CircularBufferUninit(struct CircularBuffer * cb)
{
    kvfree(cb->buffer);
}

void CircularBufferAdd(struct CircularBuffer * cb, void * data)
{
    if ((cb->head + 1) % cb->size == cb->tail)
    {
        void ** buffer;
        buffer = kvmalloc(sizeof(void *) * cb->size * 2, GFP_KERNEL);
        if (cb->tail == 0)
        {
            memcpy(buffer, cb->buffer, (cb->size - 1) * sizeof(void *));
        }
        else
        {
            UINT64 firstPartSize = (cb->size - cb->tail + 1) * sizeof(void *);
            memcpy(buffer, cb->buffer + cb->tail, firstPartSize * sizeof(void *));
            buffer += firstPartSize;
            memcpy(buffer, cb->buffer, (cb->size - firstPartSize - 1) * sizeof(void *));
        }
        kvfree(cb->buffer);
        cb->buffer = buffer;
        cb->size *= 2;
    }
    cb->buffer[cb->head] = data;
    cb->head++;
    if (cb->head >= cb->size)
        cb->head -= cb->size;
}

void * CircularBufferDelete(struct CircularBuffer * cb)
{
    void * data = cb->buffer[cb->tail];
    cb->tail++;
    if (cb->tail >= cb->size)
        cb->tail -= cb->size;
    return data;
}

int CircularBufferIsEmpty(struct CircularBuffer * cb)
{
    if (cb->head == cb->tail)
        return 1;
    return 0;
}