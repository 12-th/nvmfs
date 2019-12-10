#include "DirInode.h"
#include "FileInode.h"
#include "Inode.h"
#include "Log.h"

void InodeRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                   struct NVMAccesser * acc)
{
    UINT8 type;
    LogRecoveryReadReserveData(inodeAddr, &type, sizeof(type), offsetof(struct NvmInode, type), acc);
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        FileInodeRecovery(inodeAddr, ctor, cb, acc);
        break;
    case INODE_TYPE_DIR_FILE:
        DirInodeRecovery(inodeAddr, ctor, cb, acc);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
}

void InodeRebuild(logic_addr_t inodeAddr, struct NVMAccesser * acc)
{
    UINT8 type;

    LogRebuildReadReserveData(inodeAddr, &type, sizeof(type), offsetof(struct NvmInode, type), acc);
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:

        break;
    case INODE_TYPE_DIR_FILE:
        // DirInodeRebuild()
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
}