#ifndef DB_EX1_MACROS_H
#define DB_EX1_MACROS_H

#define __MALLOC(size, type) ((type*) malloc((size) * sizeof(type)))

#define STR_COPY(dest, source, len) \
{ \
  memcpy(dest, source, len); \
  dest[len] = '\0'; \
} \

#define CHECK(res, error_message, on_error) \
do { \
  int ret_val = (res); \
  if (ret_val < 0) { \
    fprintf(stderr, "%s returned %d on file <%s> on line %d\n", #res, ret_val, __FILE__, __LINE__); \
    BF_PrintError(error_message); \
    on_error; \
  } \
} while (0U) \

#endif //DB_EX1_MACROS_H
