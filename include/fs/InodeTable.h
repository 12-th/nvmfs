#ifndef INODE_TABLE_H
#define INODE_TABLE_H

#include "Types.h"

struct InodeTableEntry
{
    union {
        void * next;
        logic_addr_t inode;
    };
};

struct InodeTable
{
    nvm_addr_t addr;
    UINT64 maxInodeNum;
    struct InodeTableEntry * listTable;
    struct InodeTableEntry * head;
    struct InodeTableEntry * tail;
};

void InodeTableFormat(struct InodeTable * pTable, nvm_addr_t addr, UINT64 size);
void InodeTableUninit(struct InodeTable * pTable);
void InodeTableRecoveryPreinit(struct InodeTable * pTable, nvm_addr_t addr, UINT64 size);
void InodeTableRecoveryNotifyInodeInuse(struct InodeTable * pTable, nvmfs_ino_t ino, int * hasNotified);
void InodeTableRecoveryEnd(struct InodeTable * pTable);
nvmfs_ino_t InodeTableGetIno(struct InodeTable * pTable);
void InodeTablePutIno(struct InodeTable * pTable, nvmfs_ino_t ino);
void InodeTableUpdateInodeAddr(struct InodeTable * pTable, nvmfs_ino_t ino, logic_addr_t inode);
logic_addr_t InodeTableGetInodeAddr(struct InodeTable * pTable, nvmfs_ino_t ino);

#endif