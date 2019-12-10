#ifndef CICULAR_BUFFER_H
#define CICULAR_BUFFER_H

#include "Types.h"

struct CircularBuffer
{
    void ** buffer;
    UINT64 head;
    UINT64 tail;
    UINT64 size;
};

void CircularBufferInit(struct CircularBuffer * cb);
void CircularBufferUninit(struct CircularBuffer * cb);
void CircularBufferAdd(struct CircularBuffer * cb, void * data);
void * CircularBufferDelete(struct CircularBuffer * cb);
int CircularBufferIsEmpty(struct CircularBuffer * cb);

#endif