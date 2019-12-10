#ifndef CONTINOUS_SPACE_H
#define CONTINOUS_SPACE_H

#include "Types.h"

struct FsConstructor;
struct PagePool;
struct NVMAccesser;

#define CONTINOUS_SPACE_INLINE_ARRAY_SIZE 3

struct ContinousSpace
{
    logic_addr_t firstArea;
    UINT64 restSize;
    UINT64 areaNum;
    logic_addr_t inlineArray[CONTINOUS_SPACE_INLINE_ARRAY_SIZE];
};

int ContinousSpaceFormat(struct ContinousSpace * cs, struct PagePool * pool);
void ContinousSpaceUninit(struct ContinousSpace * cs);
void ContinousSpaceDestroy(struct ContinousSpace * cs, struct PagePool * pool, struct NVMAccesser * acc);
int ContinousSpacePreallocSpace(struct ContinousSpace * cs, UINT64 size, struct PagePool * pool,
                                struct NVMAccesser * acc, logic_addr_t start);
int ContinousSpaceEssureEnoughSpace(struct ContinousSpace * cs, logic_addr_t addr, UINT64 size, struct PagePool * pool,
                                    struct NVMAccesser * acc);
void ContinousSpaceNotifyUsedSpace(struct ContinousSpace * cs, UINT64 size);
void ContinousSpaceWrite(void * buffer, UINT64 size, logic_addr_t addr, struct NVMAccesser * acc);
void ContinousSpaceRead(void * buffer, UINT64 size, logic_addr_t start, struct NVMAccesser * acc);
logic_addr_t ContinousSpaceCalculateNextAddr(logic_addr_t start, UINT64 size, struct NVMAccesser * acc);

static inline logic_addr_t ContinousSpaceStart(struct ContinousSpace * cs)
{
    return cs->firstArea + sizeof(logic_addr_t);
}

static inline logic_addr_t ContinousSpaceStartCalculate(logic_addr_t firstArea)
{
    return firstArea + sizeof(logic_addr_t);
}

static inline logic_addr_t ContinousSpaceFirstArea(struct ContinousSpace * cs)
{
    return cs->firstArea;
}

static inline UINT64 ContinousSpaceTotalSizeQuery(struct ContinousSpace * cs)
{
    return cs->areaNum * SIZE_4K;
}

void ContinousSpaceRecoveryInit(struct ContinousSpace * cs, logic_addr_t firstAreaAddr);
void ContinousSpaceRecovery(struct ContinousSpace * cs, logic_addr_t lastAddr, struct FsConstructor * ctor,
                            struct NVMAccesser * acc);
void ContinousSpaceRecoveryUninit(struct ContinousSpace * cs);

void ContinousSpaceRebuildBegin(struct ContinousSpace * cs, logic_addr_t firstArea);
void ContinousSpaceRebuildEnd(struct ContinousSpace * cs, logic_addr_t lastAddr, struct NVMAccesser * acc);

#endif