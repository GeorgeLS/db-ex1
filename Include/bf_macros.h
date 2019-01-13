#ifndef DB_EX1_BF_MACROS_H
#define DB_EX1_BF_MACROS_H

#define CHECKED_BF_CREATE_FILE(filename) \
{ \
  if ((error = BF_CreateFile(filename)) < 0) { \
    BF_PrintError("Error while creating file"); \
  } \
} \

#define CHECKED_BF_CREATE_FILE_GOTO(filename, label) \
{ \
  if ((error = BF_CreateFile(filename)) < 0) { \
    BF_PrintError("Error while creating file"); \
    goto label; \
  } \
} \

#define CHECKED_BF_OPEN_FILE(filename, fd) \
{ \
  if ((fd = BF_OpenFile(filename)) < 0) { \
    BF_PrintError("Error while opening file"); \
  } \
}\

#define CHECKED_BF_OPEN_FILE_GOTO(filename, fd, label) \
{ \
  if ((fd = BF_OpenFile(filename)) < 0) { \
    BF_PrintError("Error while opening file"); \
    goto label; \
  } \
}\

#define CHECKED_BF_ALLOCATE_BLOCK(fd) \
{ \
  if ((error = BF_AllocateBlock(fd)) < 0) { \
    BF_PrintError("Error while allocating block"); \
  } \
} \

#define CHECKED_BF_ALLOCATE_BLOCK_GOTO(fd, label) \
{ \
  if ((error = BF_AllocateBlock(fd)) < 0) { \
    BF_PrintError("Error while allocating block"); \
    goto label; \
  } \
} \

#define CHECKED_BF_READ_BLOCK(fd, block_n, block) \
{ \
  if ((error = BF_ReadBlock(fd, block_n, &block)) < 0) { \
    BF_PrintError("Error while reading block"); \
  } \
} \

#define CHECKED_BF_READ_BLOCK_GOTO(fd, block_n, block, label) \
{ \
  if ((error = BF_ReadBlock(fd, block_n, &block)) < 0) { \
    BF_PrintError("Error while reading block"); \
    goto label; \
  } \
} \

#define CHECKED_BF_WRITE_BLOCK(fd, block_n) \
{ \
  if ((error = BF_WriteBlock(fd, block_n)) < 0) { \
    BF_PrintError("Error while writing block"); \
  } \
} \

#define CHECKED_BF_WRITE_BLOCK_GOTO(fd, block_n, label) \
{ \
  if ((error = BF_WriteBlock(fd, block_n)) < 0) { \
    BF_PrintError("Error while writing block"); \
    goto label; \
  } \
} \

#define CHECKED_BF_CLOSE_FILE(fd) \
{ \
  if ((error = BF_CloseFile(fd)) < 0) { \
    BF_PrintError("Error while closing file"); \
  } \
} \

#define CHECKED_BF_CLOSE_FILE_GOTO(fd, label) \
{ \
  if ((error = BF_CloseFile(fd)) < 0) { \
    BF_PrintError("Error while closing file"); \
    goto label; \
  } \
} \

#define CHECKED_BF_GET_BLOCK_COUNTER(fd) \
{ \
  if ((error = BF_GetBlockCounter(fd)) < 0) { \
    BF_PrintError("Error while getting block counter"); \
  } \
} \

#define CHECKED_BF_GET_BLOCK_COUNTER_GOTO(fd, block_n, label) \
{ \
  if ((error = BF_GetBlockCounter(fd)) < 0) { \
    BF_PrintError("Error while getting block counter"); \
    goto label; \
  } \
  block_n = error - 1; \
} \

#endif //DB_EX1_BF_MACROS_H
