#ifndef TYPE_CHECK_H
#define TYPE_CHECK_H

#define typecheck(type,x)               \
    ({	type __dummy;                   \
        typeof(x) __dummy2;             \
        (void)(&__dummy == &__dummy2);  \
        1;                              \
    })

#endif