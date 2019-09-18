#ifndef TYPES_H
#define TYPES_H

typedef unsigned long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef long INT64;
typedef int INT32;
typedef short INT16;
typedef char INT8;

typedef UINT64 nvm_addr_t;
typedef UINT32 block_t;

#define BITS_4K 12
#define SIZE_4K (1UL << BITS_4K)

#define BITS_2M 21
#define SIZE_2M (1UL << BITS_2M)

static inline block_t nvm_addr_to_block(nvm_addr_t addr)
{
    return addr >> BITS_2M;
}

static inline nvm_addr_t block_to_nvm_addr(block_t block)
{
    return (nvm_addr_t)(block) << BITS_2M;
}

#endif