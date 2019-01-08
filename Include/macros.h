#ifndef DB_EX1_MACROS_H
#define DB_EX1_MACROS_H

#define __MALLOC(size, type) ((type*) malloc((size) * sizeof(type)))
#define __MALLOC_BYTES(bytes) ((void*) malloc((bytes)))

#endif //DB_EX1_MACROS_H
