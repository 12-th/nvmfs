#include "InodeTable.h"
#include "NVMOperations.h"
#include "kutest.h"

TEST(InodeTableTest, formatTest)
{
    NVMInit(1UL << 21);
    struct InodeTable table;
    nvmfs_ino_t ino;
    logic_addr_t inodeAddr = 0;
    logic_addr_t gotAddr;
    InodeTableFormat(&table, 0, 1UL << 21);
    ino = InodeTableGetIno(&table);
    InodeTableUpdateInodeAddr(&table, ino, inodeAddr);
    gotAddr = InodeTableGetInodeAddr(&table, ino);
    EXPECT_EQ(inodeAddr, gotAddr);

    InodeTableUninit(&table);
    NVMUninit();
}

TEST(InodeTableTest, rebuildTest)
{
    NVMInit(1UL << 21);
    struct InodeTable table;
    struct InodeTable table1;
    nvmfs_ino_t ino;
    logic_addr_t inodeAddr = 0;
    logic_addr_t gotAddr;
    int hasNotified;
    InodeTableFormat(&table, 0, 24);
    ino = InodeTableGetIno(&table);
    EXPECT_EQ(ino, 1);
    InodeTableUpdateInodeAddr(&table, ino, inodeAddr);
    gotAddr = InodeTableGetInodeAddr(&table, ino);
    EXPECT_EQ(inodeAddr, gotAddr);
    InodeTableUninit(&table);

    InodeTableRecoveryPreinit(&table1, 0, 24);
    InodeTableRecoveryNotifyInodeInuse(&table1, ino, &hasNotified);
    EXPECT_EQ(hasNotified, 0);
    InodeTableRecoveryNotifyInodeInuse(&table1, ino, &hasNotified);
    EXPECT_EQ(hasNotified, 1);
    InodeTableRecoveryEnd(&table1);
    gotAddr = InodeTableGetInodeAddr(&table1, ino);
    EXPECT_EQ(gotAddr, inodeAddr);
    ino = InodeTableGetIno(&table1);
    EXPECT_EQ(ino, 2);
    ino = InodeTableGetIno(&table1);
    EXPECT_EQ(ino, invalid_nvmfs_ino);
    InodeTableUninit(&table1);
    NVMUninit();
}