#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <stdio.h>
#include "../Include/HT.h"
#include "../Include/BF.h"
#include "../Include/macros.h"

typedef struct HT_info_ {
  int index_descriptor;
  char attribute_type;
  size_t attribute_length;
  char *attribute_name;
  unsigned long int bucket_n;
} HT_info;

static __INLINE inline uint64_t hash_function(const HT_info *restrict ht_info, const void *restrict value) {
  uint64_t hash_value = 0U;
  if (ht_info->attribute_type == 'c') {
    size_t value_len = strlen(value);
    size_t i = 0U;
    for (; i <= value_len; i += sizeof(uint64_t)) {
      hash_value += *(uint64_t *) value;
      value += sizeof(uint64_t);
    }
    if (value_len % sizeof(uint64_t) != 0U) {
      hash_value += *(uint64_t *) value;
    }
    hash_value %= ht_info->bucket_n;
  } else {
    hash_value = *(uint64_t *) value % ht_info->bucket_n;
  }
  return hash_value + 1U;
}

static __INLINE inline void HT_info_copy_to_block(HT_info *ht_info, void *restrict block) {
  memcpy(block, &ht_info->index_descriptor, sizeof(int));
  block += sizeof(int);
  memcpy(block, &ht_info->attribute_type, sizeof(char));
  block += sizeof(char);
  memcpy(block, &ht_info->attribute_length, sizeof(size_t));
  block += sizeof(size_t);
  size_t name_len = strlen(ht_info->attribute_name);
  memcpy(block, ht_info->attribute_name, name_len);
  block += name_len;
  memcpy(block, &ht_info->bucket_n, sizeof(unsigned long int));
}

static __INLINE inline void copy_block_to_HT_info(HT_info *restrict info, void *restrict block) {
  memcpy(&info->index_descriptor, block, sizeof(int));
  block += sizeof(int);
  memcpy(&info->attribute_type, block, sizeof(char));
  block += sizeof(char);
  memcpy(&info->attribute_length, block, sizeof(size_t));
  block += sizeof(size_t);
  info->attribute_name = __MALLOC(info->attribute_length + 1, char);
  memcpy(info->attribute_name, block, info->attribute_length);
  info->attribute_name[info->attribute_length] = '\0';
  block += info->attribute_length;
  memcpy(&info->bucket_n, block, sizeof(unsigned long int));
}

int HT_CreateIndex(
        char *index_name,
        char attribute_type,
        char *attribute_name,
        int attribute_length,
        int bucket_n) {

  if (sizeof(HT_info) > BLOCK_SIZE)
    return HT_BLOCK_OVERFLOW;

  int status = 0;
  int index_descriptor = 0;
  int error;

  BF_Init();
  if ((error = BF_CreateFile(index_name)) < 0) { status = error; goto __EXIT; }
  if ((index_descriptor = BF_OpenFile(index_name)) < 0) { status = index_descriptor; goto __EXIT; }

  void *block = __MALLOC_BYTES(BLOCK_SIZE);
  void *original_block = block;
  if (block == NULL) return BFE_NOMEM;

  if ((error = BF_AllocateBlock(index_descriptor)) < 0) { status = error; goto __EXIT; }
  if ((error = BF_ReadBlock(index_descriptor, 0, &block)) < 0) { status = error; goto __EXIT; }

  HT_info ht_info = {
          .index_descriptor = index_descriptor,
          .attribute_type = attribute_type,
          .attribute_name = attribute_name,
          .attribute_length = (size_t) attribute_length,
          .bucket_n = (unsigned long) bucket_n
  };

  size_t identifier_len = strlen(HT_FILE_IDENTIFIER);
  memcpy(block, HT_FILE_IDENTIFIER, identifier_len);

  HT_info_copy_to_block(&ht_info, block + identifier_len);

  if ((error = BF_WriteBlock(index_descriptor, 0)) < 0) { status = error; goto __EXIT; }
  if ((error = BF_CloseFile(index_descriptor)) < 0) { status = error; goto __EXIT; }

__EXIT:
  free(original_block);
  return status;
}

HT_info *HT_OpenIndex(char *index_name) {
  HT_info *ht_info = NULL;
  int index_descriptor;
  if ((index_descriptor = BF_OpenFile(index_name)) < 0) goto __EXIT;

  void *block = malloc(BLOCK_SIZE);
  if (block == NULL) goto __EXIT;

  if ((BF_ReadBlock(index_descriptor, 0, &block)) < 0) goto __EXIT;

  size_t identifier_len = strlen(HT_FILE_IDENTIFIER);
  if (memcmp(block, HT_FILE_IDENTIFIER, identifier_len) != 0) goto __EXIT;
  block += identifier_len;

  ht_info = __MALLOC(1, HT_info);
  if (ht_info == NULL) goto __EXIT;
  copy_block_to_HT_info(ht_info, block);

__EXIT:
  return ht_info;
}

int HT_CloseIndex(HT_info *header_info) {
  if (header_info == NULL) return -1;
  int error;
  if ((error = BF_CloseFile(header_info->index_descriptor)) < 0) return error;
  free(header_info->attribute_name);
  free(header_info);
  return 0;
}

void HT_Print(HT_info *info) {
  printf("FD = %d\tType = %c\tName = %s\tLength = %zu\tBuckets = %lu\n",
         info->index_descriptor, info->attribute_type, info->attribute_name,
         info->attribute_length, info->bucket_n);
}