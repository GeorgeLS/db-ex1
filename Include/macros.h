#ifndef DB_EX1_MACROS_H
#define DB_EX1_MACROS_H

#define __MALLOC(size, type) ((type*) malloc((size) * sizeof(type)))
#define __MALLOC_BYTES(bytes) ((void*) malloc((bytes)))
#define BYTE_POINTER(object) ((void*) object)

#define STR_COPY(dest, source, len) \
{ \
  memcpy(dest, source, len); \
  dest[len] = '\0'; \
} \

#endif //DB_EX1_MACROS_H
