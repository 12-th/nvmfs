#ifndef ATOMIC_FUNCTIONS_H
#define ATOMIC_FUNCTIONS_H

#define BOOL_COMPARE_AND_SWAP(ptr, oldval, newval) __sync_bool_compare_and_swap(ptr, oldval, newval)
#define FETCH_AND_ADD(ptr, delta) __sync_fetch_and_add(ptr, delta)

#endif