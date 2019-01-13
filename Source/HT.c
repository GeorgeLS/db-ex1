#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <stdio.h>
#include "../Include/HT.h"
#include "../Include/BF.h"
#include "../Include/macros.h"
#include "../Include/bf_macros.h"

typedef struct {
  int overflow_bucket;
  int next_record;
  int free_space;
  unsigned int record_n;
} bucket_info_t;

static __INLINE __NO_DISCARD inline
bucket_info_t create_bucket_info(void) {
  return (bucket_info_t) {
          .overflow_bucket = -1,
          .next_record = sizeof(bucket_info_t),
          .free_space = BLOCK_SIZE - sizeof(bucket_info_t),
          .record_n = 0U
  };
}

static __NON_NULL(1) __NO_DISCARD __INLINE inline
bucket_info_t get_bucket_info(void *block) {
  bucket_info_t bucket_info;
  memcpy(&bucket_info, block, sizeof(bucket_info_t));
  return bucket_info;
}

static __INLINE inline
uint64_t hash_function(const HT_info *restrict ht_info, const void *restrict value) {
  uint64_t hash_value = 0U;
  if (ht_info->attribute_type == 'c') {
    size_t value_len = strlen(value);
    size_t double_words = value_len / sizeof(uint64_t);
    for (size_t i = 0U; i != double_words; ++i) {
      hash_value += *(uint64_t *) value;
      value += sizeof(uint64_t);
    }
    size_t remaining_bytes = value_len % sizeof(uint64_t);
    for (size_t i = 0U; i != remaining_bytes; ++i) {
      hash_value += *(uint8_t *) value;
    }
    hash_value %= ht_info->bucket_n;
  } else {
    hash_value = (*(int *) value) % ht_info->bucket_n;
  }
  return hash_value + 1U;
}

static __INLINE inline
void HT_info_copy_to_block(HT_info *ht_info, void *restrict block) {
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

static __INLINE inline
void copy_block_to_HT_info(HT_info *restrict info, void *restrict block) {
  memcpy(&info->index_descriptor, block, sizeof(int));
  block += sizeof(int);
  memcpy(&info->attribute_type, block, sizeof(char));
  block += sizeof(char);
  memcpy(&info->attribute_length, block, sizeof(size_t));
  block += sizeof(size_t);
  info->attribute_name = __MALLOC(info->attribute_length + 1, char);
  STR_COPY(info->attribute_name, block, info->attribute_length);
  block += info->attribute_length;
  memcpy(&info->bucket_n, block, sizeof(unsigned long int));
}

static __INLINE inline
void initialize_block(void *block) {
  bucket_info_t bucket_info = create_bucket_info();
  memcpy(block, &bucket_info, sizeof(bucket_info_t));
}

int HT_CreateIndex(char *index_name, char attribute_type, char *attribute_name,
                   int attribute_length, int bucket_n) {

  if (sizeof(HT_info) > BLOCK_SIZE) return HT_BLOCK_OVERFLOW;

  int error;
  int index_descriptor = 0;

  CHECKED_BF_CREATE_FILE_GOTO(index_name, __ERROR);
  CHECKED_BF_OPEN_FILE_GOTO(index_name, index_descriptor, __ERROR);

  void *block;
  CHECKED_BF_ALLOCATE_BLOCK_GOTO(index_descriptor, __ERROR);
  CHECKED_BF_READ_BLOCK_GOTO(index_descriptor, 0, block, __ERROR);

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

  CHECKED_BF_WRITE_BLOCK_GOTO(index_descriptor, 0, __ERROR);
  for (size_t i = 1U; i <= bucket_n; ++i) {
    CHECKED_BF_ALLOCATE_BLOCK_GOTO(index_descriptor, __ERROR);
    void *bucket_block;
    CHECKED_BF_READ_BLOCK_GOTO(index_descriptor, (int) i, bucket_block, __ERROR);
    initialize_block(bucket_block);
    CHECKED_BF_WRITE_BLOCK_GOTO(index_descriptor, (int) i, __ERROR);
  }

  CHECKED_BF_CLOSE_FILE_GOTO(index_descriptor, __ERROR);
  return 0;
__ERROR:
  return error;
}

HT_info *HT_OpenIndex(char *index_name) {
  int error;
  HT_info *ht_info = NULL;
  int index_descriptor;
  CHECKED_BF_OPEN_FILE_GOTO(index_name, index_descriptor, __EXIT);

  void *block;
  CHECKED_BF_READ_BLOCK_GOTO(index_descriptor, 0, block, __EXIT);

  size_t identifier_len = strlen(HT_FILE_IDENTIFIER);
  if (memcmp(block, HT_FILE_IDENTIFIER, identifier_len) != 0) goto __EXIT;

  ht_info = __MALLOC(1, HT_info);
  if (ht_info == NULL) goto __EXIT;
  copy_block_to_HT_info(ht_info, block + identifier_len);

__EXIT:
  return ht_info;
}

int HT_CloseIndex(HT_info *header_info) {
  if (header_info == NULL) return -1;
  int error;
  CHECKED_BF_CLOSE_FILE_GOTO(header_info->index_descriptor, __ERROR);
  free(header_info->attribute_name);
  free(header_info);
  return 0;
__ERROR:
  return error;
}

int HT_InsertEntry(HT_info header_info, Record record) {
  int error;
  int bucket = (int) hash_function(&header_info, &record.id);
  void *block;
  int index_descriptor = header_info.index_descriptor;
  CHECKED_BF_READ_BLOCK_GOTO(index_descriptor, bucket, block, __ERROR);
  bucket_info_t bucket_info = *(bucket_info_t *) block;
  int current_bucket = bucket;
  while (bucket_info.free_space < sizeof(Record)) {
    if (bucket_info.overflow_bucket != -1) {
      CHECKED_BF_READ_BLOCK_GOTO(index_descriptor, bucket_info.overflow_bucket, block, __ERROR);
      current_bucket = bucket_info.overflow_bucket;
      bucket_info = *(bucket_info_t *) block;
    } else {
      CHECKED_BF_ALLOCATE_BLOCK_GOTO(index_descriptor, __ERROR);
      CHECKED_BF_GET_BLOCK_COUNTER_GOTO(index_descriptor, bucket_info.overflow_bucket, __ERROR);
      memcpy(block, &bucket_info, sizeof(bucket_info_t));
      CHECKED_BF_WRITE_BLOCK_GOTO(index_descriptor, current_bucket, __ERROR);
      CHECKED_BF_READ_BLOCK_GOTO(index_descriptor, bucket_info.overflow_bucket, block, __ERROR);
      initialize_block(block);
      CHECKED_BF_WRITE_BLOCK_GOTO(header_info.index_descriptor, bucket_info.overflow_bucket, __ERROR);
      bucket_info = *(bucket_info_t *) block;
      break;
    }
  }
  memcpy(block + bucket_info.next_record, &record, sizeof(Record));
  bucket_info.next_record += sizeof(Record);
  bucket_info.free_space -= sizeof(Record);
  ++bucket_info.record_n;
  memcpy(block, &bucket_info, sizeof(bucket_info_t));
  CHECKED_BF_WRITE_BLOCK_GOTO(index_descriptor, bucket, __ERROR);
  return 0;
__ERROR:
  return -1;
}

void HT_Print(HT_info *info) {
  printf("FD = %d\tType = %c\tName = %s\tLength = %zu\tBuckets = %lu\n",
         info->index_descriptor, info->attribute_type, info->attribute_name,
         info->attribute_length, info->bucket_n);
}