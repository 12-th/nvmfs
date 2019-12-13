#include "InodeTable.h"
#include "NVMOperations.h"
#include <linux/mm.h>
#include <linux/slab.h>

#define INODE_TABLE_LIST_TABLE_SIZE (SIZE_2M)

void InodeTableFormat(struct InodeTable * pTable, nvm_addr_t addr, UINT64 size)
{
    UINT64 maxInodeNum;
    int i;
    UINT64 listTableSize;

    maxInodeNum = size / sizeof(void *);
    listTableSize = maxInodeNum * sizeof(struct InodeTableEntry);
    pTable->addr = addr;
    pTable->maxInodeNum = maxInodeNum;
    pTable->listTable = kvmalloc(listTableSize, GFP_KERNEL);
    for (i = 1; i < maxInodeNum - 1; ++i)
    {
        pTable->listTable[i].next = &(pTable->listTable[i + 1]);
    }
    pTable->listTable[maxInodeNum - 1].next = NULL;
    pTable->head = &pTable->listTable[1];
    pTable->tail = &pTable->listTable[maxInodeNum - 1];
}

void InodeTableUninit(struct InodeTable * pTable)
{
    kvfree(pTable->listTable);
}

void InodeTableRecoveryPreinit(struct InodeTable * pTable, nvm_addr_t addr, UINT64 size)
{
    pTable->maxInodeNum = size / sizeof(void *);
    pTable->addr = addr;
    pTable->listTable = kvmalloc(size, GFP_KERNEL);
    memset(pTable->listTable, -1, pTable->maxInodeNum * sizeof(struct InodeTableEntry));
    pTable->head = pTable->tail = NULL;
}

void InodeTableRecoveryNotifyInodeInuse(struct InodeTable * pTable, nvmfs_ino_t ino, int * hasNotified)
{
    logic_addr_t inode;

    if (pTable->listTable[ino].inode != -1)
    {
        *hasNotified = 1;
    }
    else
    {
        NVMRead(pTable->addr + sizeof(logic_addr_t) * ino, sizeof(logic_addr_t), &inode);
        pTable->listTable[ino].inode = inode;
        *hasNotified = 0;
    }
}

static inline void AppendToList(struct InodeTable * pTable, nvmfs_ino_t ino)
{
    void * ptr;

    ptr = &pTable->listTable[ino];
    if (pTable->head == NULL)
    {
        pTable->head = pTable->tail = ptr;
    }
    else
    {
        pTable->tail->next = ptr;
        pTable->tail = ptr;
    }
}

void InodeTableRecoveryEnd(struct InodeTable * pTable)
{
    UINT64 i;

    for (i = 0; i < pTable->maxInodeNum; ++i)
    {
        if (pTable->listTable[i].inode == -1)
        {
            AppendToList(pTable, i);
        }
    }
}

nvmfs_ino_t InodeTableGetIno(struct InodeTable * pTable)
{
    struct InodeTableEntry * cur;

    cur = pTable->head;
    if (cur == NULL)
        return invalid_nvmfs_ino;

    if (pTable->head == pTable->tail)
    {
        cur = pTable->head;
        pTable->head = pTable->tail = NULL;
    }
    else
    {
        cur = pTable->head;
        pTable->head = pTable->head->next;
    }
    return cur - pTable->listTable;
}

void InodeTablePutIno(struct InodeTable * pTable, nvmfs_ino_t ino)
{
    AppendToList(pTable, ino);
}

void InodeTableUpdateInodeAddr(struct InodeTable * pTable, nvmfs_ino_t ino, logic_addr_t inode)
{
    pTable->listTable[ino].inode = inode;
    NVMWrite(pTable->addr + sizeof(logic_addr_t) * ino, sizeof(logic_addr_t), &inode);
}

logic_addr_t InodeTableGetInodeAddr(struct InodeTable * pTable, nvmfs_ino_t ino)
{
    return pTable->listTable[ino].inode;
}