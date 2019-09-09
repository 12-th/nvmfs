#ifndef NVM_PTR_H
#define NVM_PTR_H

#include "NVMOperations.h"
#include "NVMPtrDefine.h"

#define nvm_ptr_distance(first, second)                                                                                \
    ({                                                                                                                 \
        UINT64 size;                                                                                                   \
        typecheck(typeof(first.typeForCheck), second.typeForCheck);                                                    \
        size = second.addr - first.addr;                                                                               \
        size;                                                                                                          \
    })

#define nvm_ptr_less(first, second)                                                                                    \
    ({                                                                                                                 \
        int ret;                                                                                                       \
        typecheck(typeof(first.typeForCheck), second.typeForCheck);                                                    \
        ret = !!(first.addr < second.addr);                                                                            \
        ret;                                                                                                           \
    })

#define nvm_ptr_cast(ptrType, ptr)                                                                                     \
    ({                                                                                                                 \
        typeof(ptrType) tmp;                                                                                           \
        tmp.addr = (ptr).addr;                                                                                         \
        tmp;                                                                                                           \
    })

#define nvm_field_ptr(ptr, field)                                                                                      \
    ({                                                                                                                 \
        long offset = (long)(&(((typeof((ptr).typeForCheck))(0))->field));                                             \
        nvm_addr_t tmp;                                                                                                \
        tmp.addr = (ptr).addr + offset;                                                                                \
        tmp;                                                                                                           \
    })

#define nvm_ptr_to_addr(ptr)                                                                                           \
    ({                                                                                                                 \
        nvm_addr_t tmp;                                                                                                \
        tmp.addr = (ptr).addr;                                                                                         \
        tmp;                                                                                                           \
    })

#define nvm_ptr_from_ul(ptrType, value)                                                                                \
    ({                                                                                                                 \
        typeof(ptrType) tmp;                                                                                           \
        tmp.addr = (value);                                                                                            \
        tmp;                                                                                                           \
    })

#define nvm_addr_from_ul(value) nvm_ptr_from_ul(nvm_addr_t, value)

#define nvm_ptr_to_ul(ptr) ({ (ptr).addr; })

#define nvm_ptr_is_null(ptr) ((ptr).addr == 0)
#define nvm_ptr_null(type)                                                                                             \
    ({                                                                                                                 \
        typeof(type) tmp;                                                                                              \
        tmp.addr = 0;                                                                                                  \
        tmp;                                                                                                           \
    })

#define nvm_ptr_equal(ptr1, ptr2)                                                                                      \
    ({                                                                                                                 \
        typecheck(typeof(ptr1), ptr2);                                                                                 \
        (ptr1).addr == (ptr2).addr;                                                                                    \
    })

#define nvm_ptr_advance(ptr, delta)                                                                                    \
    ({                                                                                                                 \
        typeof(ptr) tmp;                                                                                               \
        tmp.addr = (ptr).addr + (delta);                                                                               \
        tmp;                                                                                                           \
    })

#define nvm_ptr_read(ptr, value)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        typecheck(typeof(&(value)), (ptr).typeForCheck);                                                               \
        NVMRead(nvm_ptr_to_addr(ptr), sizeof(value), &(value))                                                         \
    } while (0)

#define nvm_ptr_write(ptr, value)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        typecheck(typeof(&(value)), (ptr).typeForCheck);                                                               \
        NVMWrite(nvm_ptr_to_addr(ptr), sizeof(value), &(value));                                                       \
    } while (0)

//*dstPtr = *srcPtr
#define nvm_ptr_assign(dstPtr, srcPtr)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        int size = sizeof(typeof(*((dstPtr).typeForCheck)));                                                           \
        typecheck(typeof(dstPtr), (srcPtr));                                                                           \
        NVMemcpy(dstPtr, srcPtr, size);                                                                                \
    } while (0)

// value = ptr->field
#define nvm_ptr_read_field(ptr, field, value)                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        long offset = (long)(&(((typeof((ptr).typeForCheck))(0))->field));                                             \
        typecheck(typeof((ptr).typeForCheck->field), (value));                                                         \
        NVMRead(nvm_addr_from_ul((ptr).addr + offset), sizeof(value), &(value));                                       \
    } while (0)

// ptr->field = value
#define nvm_ptr_write_field(ptr, field, value)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        long offset = (long)(&(((typeof((ptr).typeForCheck))(0))->field));                                             \
        typecheck(typeof((ptr).typeForCheck->field), (value));                                                         \
        NVMWrite(nvm_addr_from_ul((ptr).addr + offset), sizeof(value), &(value));                                      \
    } while (0)

// dstPtr->field = srcPtr->field
#define nvm_ptr_assign_field(dstPtr, srcPtr, field)                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        int size = sizeof(typeof((srcPtr).typeForCheck->field));                                                       \
        long offset = (long)(&(((typeof((srcPtr).typeForCheck))(0))->field));                                          \
        NVMemcpy(nvm_addr_from_ul((dstPtr).addr + offset), nvm_addr_from_ul((srcPtr).addr + offset), size);            \
    } while (0)

#endif