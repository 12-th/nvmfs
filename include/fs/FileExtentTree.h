#ifndef FILE_EXTENT_TREE_H
#define FILE_EXTENT_TREE_H

#include "NVMAccesser.h"
#include <linux/rbtree.h>

struct FileSpaceNode
{
    logic_addr_t start;
    UINT64 size;
    struct rb_node node;
    // struct rb_root unmapRoot;
};

struct FileExtentNode
{
    logic_addr_t start;
    UINT64 size;
    UINT64 fileStart;
    struct rb_node extentNode;
    struct rb_node spaceNode;
};

struct FileExtentTree
{
    struct rb_root fileExtentRoot;
    struct rb_root spaceRoot;
    UINT64 effectiveSize;
    UINT64 spaceSize;
};

void FileExtentTreeInit(struct FileExtentTree * tree);
int FileExtentTreeAddSpace(struct FileExtentTree * tree, logic_addr_t addr, UINT64 size, gfp_t flags);
int FileExtentTreeAddExtent(struct FileExtentTree * tree, logic_addr_t addr, UINT64 fileStart, UINT64 size,
                            gfp_t flags);
void FileExtentTreeRead(struct FileExtentTree * tree, UINT64 start, UINT64 end, void * buffer,
                        struct NVMAccesser * acc);
void FileExtentTreeTruncate(struct FileExtentTree * tree);
UINT64 FileExtentTreeGetEffectiveSize(struct FileExtentTree * tree);
UINT64 FileExtentTreeGetSpaceSize(struct FileExtentTree * tree);
void FileExtentTreeUninit(struct FileExtentTree * tree);
int FileExtentTreeIsSame(struct FileExtentTree * tree1, struct FileExtentTree * tree2);
void FileExtentTreePrintInfo(struct FileExtentTree * tree);
void FileExtentTreePrintSpaceInfo(struct FileExtentTree * tree);

#define for_each_extent_in_file_extent_tree(extent, tree, node)                                                        \
    for (node = rb_first(&(tree)->fileExtentRoot),                                                                     \
        extent = node ? container_of(node, struct FileExtentNode, extentNode) : NULL;                                  \
         extent != NULL;                                                                                               \
         node = rb_next(node), extent = node ? container_of(node, struct FileExtentNode, extentNode) : NULL)

#define for_each_space_in_file_space_tree(space, tree, curNode)                                                        \
    for (curNode = rb_first(&(tree)->spaceRoot),                                                                       \
        space = curNode ? container_of(curNode, struct FileSpaceNode, node) : NULL;                                    \
         space != NULL;                                                                                                \
         curNode = rb_next(curNode), space = curNode ? container_of(curNode, struct FileSpaceNode, node) : NULL)
#endif