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
typedef UINT32 physical_block_t;
typedef UINT32 logical_block_t;
typedef UINT64 logic_addr_t;
typedef UINT64 physical_page_t;
typedef UINT64 logical_page_t;

#define BITS_4K 12
#define SIZE_4K (1UL << BITS_4K)

#define BITS_2M 21
#define SIZE_2M (1UL << BITS_2M)

#define invalid_nvm_addr (-1UL)
#define invalid_block (-1)

#endif