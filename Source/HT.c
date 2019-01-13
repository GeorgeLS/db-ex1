#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <stdio.h>
#include "../Include/HT.h"
#include "../Include/BF.h"
#include "../Include/macros.h"

#define BF_CREATE_EMSG "Error while creating file"
#define BF_OPEN_EMSG "Error while opening file"
#define BF_ALLOCATE_EMSG "Error while allocating block"
#define BF_READ_BLOCK_EMSG "Error while reading block"
#define BF_WRITE_BLOCK_EMSG "Error while writing block"
#define BF_CLOSE_EMSG "Error while closing file"
#define BF_GET_BLOCK_COUNTER_EMSG "Error while getting block counter"

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
void HT_info_copy_to_block(const HT_info *restrict const ht_info, void *restrict block) {
  memcpy(block, &ht_info->index_descriptor, sizeof(int));
  block += sizeof(int);
  memcpy(block, &ht_info->attribute_type, sizeof(char));
  block += sizeof(char);
  memcpy(block, &ht_info->attribute_length, sizeof(size_t));
  block += sizeof(size_t);
  memcpy(block, ht_info->attribute_name, ht_info->attribute_length);
  block += ht_info->attribute_length;
  memcpy(block, &ht_info->bucket_n, sizeof(unsigned long int));
}

static __INLINE inline
void copy_block_to_HT_info(HT_info *restrict info, const void *restrict block) {
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
void SHT_info_copy_to_block(const SHT_info *restrict const sh_info, void *restrict block) {
  memcpy(block, &sh_info->secondary_index_descriptor, sizeof(int));
  block += sizeof(int);
  memcpy(block, &sh_info->attribute_length, sizeof(size_t));
  block += sizeof(size_t);
  memcpy(block, &sh_info->attribute_name, sh_info->attribute_length);
  block += sh_info->attribute_length;
  memcpy(block, &sh_info->bucket_n, sizeof(unsigned long int));
  block += sizeof(unsigned long int);
  memcpy(block, &sh_info->index_name_length, sizeof(size_t));
  block += sizeof(size_t);
  memcpy(block, &sh_info->index_name, sh_info->index_name_length);
}

static __INLINE inline
void initialize_block(void *block) {
  bucket_info_t bucket_info = create_bucket_info();
  memcpy(block, &bucket_info, sizeof(bucket_info_t));
}

int HT_CreateIndex(char *index_name, char attribute_type, char *attribute_name,
                   int attribute_length, int bucket_n) {

  if (sizeof(HT_info) > BLOCK_SIZE) return HT_BLOCK_OVERFLOW;
  int index_descriptor = 0;
  CHECK(BF_CreateFile(index_name), BF_CREATE_EMSG, return -1);
  CHECK(index_descriptor = BF_OpenFile(index_name), BF_OPEN_EMSG, return -1);

  void *block;
  CHECK(BF_AllocateBlock(index_descriptor), BF_ALLOCATE_EMSG, return -1);
  CHECK(BF_ReadBlock(index_descriptor, 0, &block), BF_READ_BLOCK_EMSG, return -1);

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

  CHECK(BF_WriteBlock(index_descriptor, 0), BF_WRITE_BLOCK_EMSG, return -1);
  for (size_t i = 1U; i <= bucket_n; ++i) {
    CHECK(BF_AllocateBlock(index_descriptor), BF_ALLOCATE_EMSG, return -1);
    void *bucket_block;
    CHECK(BF_ReadBlock(index_descriptor, (int) i, &bucket_block), BF_READ_BLOCK_EMSG, return -1);
    initialize_block(bucket_block);
    CHECK(BF_WriteBlock(index_descriptor, (int) i), BF_WRITE_BLOCK_EMSG, return -1);
  }
  CHECK(BF_CloseFile(index_descriptor), BF_CLOSE_EMSG, return -1);
  return 0;
}

HT_info *HT_OpenIndex(char *index_name) {
  int index_descriptor;
  CHECK(index_descriptor = BF_OpenFile(index_name), BF_OPEN_EMSG, return NULL);

  void *block;
  CHECK(BF_ReadBlock(index_descriptor, 0, &block), BF_READ_BLOCK_EMSG, return NULL);

  size_t identifier_len = strlen(HT_FILE_IDENTIFIER);
  if (memcmp(block, HT_FILE_IDENTIFIER, identifier_len) != 0) return NULL;

  HT_info *ht_info = __MALLOC(1, HT_info);
  if (ht_info == NULL) return NULL;
  copy_block_to_HT_info(ht_info, block + identifier_len);
  return ht_info;
}

int HT_CloseIndex(HT_info *header_info) {
  if (header_info == NULL) return -1;
  CHECK(BF_CloseFile(header_info->index_descriptor), BF_CLOSE_EMSG, return -1);
  free(header_info->attribute_name);
  free(header_info);
  return 0;
}

int HT_InsertEntry(HT_info header_info, Record record) {
  int index_descriptor = header_info.index_descriptor;
  int bucket = (int) hash_function(&header_info, &record.id);
  void *block;
  CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
  bucket_info_t bucket_info = *(bucket_info_t *) block;
  int current_bucket = bucket;
  while (bucket_info.free_space < sizeof(Record)) {
    if (bucket_info.overflow_bucket != -1) {
      CHECK(BF_ReadBlock(index_descriptor, bucket_info.overflow_bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      current_bucket = bucket_info.overflow_bucket;
      bucket_info = *(bucket_info_t *) block;
    } else {
      CHECK(BF_AllocateBlock(index_descriptor), BF_ALLOCATE_EMSG, return -1);
      CHECK(bucket_info.overflow_bucket = BF_GetBlockCounter(index_descriptor) - 1, BF_GET_BLOCK_COUNTER_EMSG,
            return -1);
      memcpy(block, &bucket_info, sizeof(bucket_info_t));
      CHECK(BF_WriteBlock(index_descriptor, current_bucket), BF_WRITE_BLOCK_EMSG, return -1);
      CHECK(BF_ReadBlock(index_descriptor, bucket_info.overflow_bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      current_bucket = bucket_info.overflow_bucket;
      initialize_block(block);
      CHECK(BF_WriteBlock(index_descriptor, bucket_info.overflow_bucket), BF_WRITE_BLOCK_EMSG, return -1);
      bucket_info = *(bucket_info_t *) block;
      break;
    }
  }
  memcpy(block + bucket_info.next_record, &record, sizeof(Record));
  bucket_info.next_record += sizeof(Record);
  bucket_info.free_space -= sizeof(Record);
  ++bucket_info.record_n;
  memcpy(block, &bucket_info, sizeof(bucket_info_t));
  CHECK(BF_WriteBlock(index_descriptor, bucket), BF_WRITE_BLOCK_EMSG, return -1);
  return current_bucket;
}

int HT_DeleteEntry(HT_info header_info, void *value) {
  int id = *(int *) value;
  int index_descriptor = header_info.index_descriptor;
  int bucket = (int) hash_function(&header_info, value);
  size_t i;
  void *block;
  void *block_base;
  bucket_info_t bucket_info;
  while (1U) {
    CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
    bucket_info = *(bucket_info_t *) block;
    block_base = block;
    block += sizeof(bucket_info_t);
    for (i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
      Record record = *(Record *) block;
      if (record.id == id) goto __SEARCH_END;
    }
    if (bucket_info.overflow_bucket == -1) return -1;
    bucket = bucket_info.overflow_bucket;
  }
__SEARCH_END:;
  size_t remaining_records = bucket_info.record_n - i - 1;
  memcpy(block, block + sizeof(Record), remaining_records * sizeof(Record));
  bucket_info.next_record -= sizeof(Record);
  bucket_info.free_space += sizeof(Record);
  --bucket_info.record_n;
  memcpy(block_base, &bucket_info, sizeof(bucket_info_t));
  CHECK(BF_WriteBlock(index_descriptor, bucket), BF_WRITE_BLOCK_EMSG, return -1);
  return 0;
}

int HT_GetAllEntries(HT_info header_info, void *value) {
  int id = *(int *) value;
  int index_descriptor = header_info.index_descriptor;
  int bucket = (int) hash_function(&header_info, value);
  int blocks_read = 0U;
  do {
    void *block;
    CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
    bucket_info_t bucket_info = *(bucket_info_t *) block;
    block += sizeof(bucket_info_t);
    for (size_t i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
      if (((Record *) block)->id == id) print_record(block);
    }
    bucket = bucket_info.overflow_bucket;
    ++blocks_read;
  } while (bucket != -1);
  return blocks_read;
}

int SHT_CreateSecondaryIndex(char *secondary_index_name, char *attribute_name,
                             int attribute_length, int bucket_n, char *index_name) {

  if (sizeof(SHT_info) > BLOCK_SIZE) return HT_BLOCK_OVERFLOW;
  int secondary_index_descriptor = 0;
  CHECK(BF_CreateFile(secondary_index_name), BF_CREATE_EMSG, return -1);
  CHECK(secondary_index_descriptor = BF_OpenFile(secondary_index_name), BF_OPEN_EMSG, return -1);

  void *block;
  CHECK(BF_AllocateBlock(secondary_index_descriptor), BF_ALLOCATE_EMSG, return -1);
  CHECK(BF_ReadBlock(secondary_index_descriptor, 0, &block), BF_READ_BLOCK_EMSG, return -1);

  SHT_info sht_info = {
          .secondary_index_descriptor = secondary_index_descriptor,
          .attribute_name = attribute_name,
          .attribute_length = (size_t) attribute_length,
          .bucket_n = (unsigned long) bucket_n,
          .index_name_length = strlen(index_name),
          .index_name = index_name
  };

  size_t identifier_len = strlen(HT_FILE_IDENTIFIER);
  memcpy(block, HT_FILE_IDENTIFIER, identifier_len);

  SHT_info_copy_to_block(&sht_info, block + identifier_len);

  CHECK(BF_WriteBlock(secondary_index_descriptor, 0), BF_WRITE_BLOCK_EMSG, return -1);
  for (size_t i = 1U; i <= bucket_n; ++i) {
    CHECK(BF_AllocateBlock(secondary_index_descriptor), BF_ALLOCATE_EMSG, return -1);
    void *bucket_block;
    CHECK(BF_ReadBlock(secondary_index_descriptor, (int) i, &bucket_block), BF_READ_BLOCK_EMSG, return -1);
    initialize_block(bucket_block);
    CHECK(BF_WriteBlock(secondary_index_descriptor, (int) i), BF_WRITE_BLOCK_EMSG, return -1);
  }
  CHECK(BF_CloseFile(secondary_index_descriptor), BF_CLOSE_EMSG, return -1);
  return 0;
}
