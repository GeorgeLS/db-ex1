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

typedef struct {
  int block_id;
  char *value;
} SHT_insert_info;

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
uint64_t hash_function(char attribute_type, size_t bucket_n, const void *restrict value) {
  uint64_t hash_value = 0U;
  if (attribute_type == 'c') {
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
    hash_value %= bucket_n;
  } else {
    hash_value = (*(int *) value) % bucket_n;
  }
  return hash_value + 1U;
}

void *get_hash_attribute(char attribute_type, const char *attribute_name, size_t len, Record *record) {
  void *hash_attribute;
  if (attribute_type == 'c') {
    if (!strncmp(attribute_name, "name", len)) {
      hash_attribute = record->name;
    } else if (!strncmp(attribute_name, "surname", len)) {
      hash_attribute = record->surname;
    } else if (!strncmp(attribute_name, "address", len)) {
      hash_attribute = record->address;
    } else return NULL;
  } else if (attribute_type == 'i') {
    hash_attribute = &record->id;
  } else return NULL;
  return hash_attribute;
}

size_t get_attribute_offset(const char *attribute_name, size_t len) {
  size_t offset = 0xBADFACE;
  if (!strncmp(attribute_name, "name", len)) {
    offset = offsetof(Record, name);
  } else if (!strncmp(attribute_name, "surname", len)) {
    offset = offsetof(Record, surname);
  } else if (!strncmp(attribute_name, "address", len)) {
    offset = offsetof(Record, address);
  }
  return offset;
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

  size_t identifier_len = strlen(HT_FILE_IDENTIFIER);
  memcpy(block, HT_FILE_IDENTIFIER, identifier_len);
  block += identifier_len;

  HT_info *info = (HT_info *) block;
  info->fileDesc = index_descriptor;
  info->attrType = attribute_type;
  info->attrLength = (size_t) attribute_length;
  info->numBuckets = (unsigned long) bucket_n;
  memcpy(&info->attrName, attribute_name, (size_t) attribute_length);

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
  block += identifier_len;

  HT_info *info = (HT_info *) block;
  HT_info *ht_info = __MALLOC(1, HT_info);
  if (ht_info == NULL) return NULL;
  ht_info->fileDesc = info->fileDesc;
  ht_info->attrType = info->attrType;
  ht_info->numBuckets = info->numBuckets;
  ht_info->attrLength = info->attrLength;
  ht_info->attrName = __MALLOC(info->attrLength + 1, char);
  STR_COPY(ht_info->attrName, &info->attrName, info->attrLength);
  return ht_info;
}

int HT_CloseIndex(HT_info *header_info) {
  if (header_info == NULL) return -1;
  CHECK(BF_CloseFile(header_info->fileDesc), BF_CLOSE_EMSG, return -1);
  free(header_info->attrName);
  free(header_info);
  return 0;
}

int HT_InsertEntry(HT_info header_info, Record record) {
  int index_descriptor = header_info.fileDesc;
  void *hash_attribute = get_hash_attribute(header_info.attrType, header_info.attrName,
                                            header_info.attrLength, &record);
  int bucket = (int) hash_function(header_info.attrType, header_info.numBuckets, hash_attribute);
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
  int index_descriptor = header_info.fileDesc;
  int bucket = (int) hash_function(header_info.attrType, header_info.numBuckets, value);
  void *block;
  void *block_base;
  bucket_info_t bucket_info;
  size_t i;
  if (header_info.attrType == 'c') {
    size_t value_len = strlen(value);
    size_t field_offset = get_attribute_offset(header_info.attrName, header_info.attrLength);
    while (1U) {
      CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      bucket_info = *(bucket_info_t *) block;
      block_base = block;
      block += sizeof(bucket_info_t);
      for (i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
        char *key = ((char *) (block + field_offset));
        if (!strncmp(key, value, value_len)) goto __SEARCH_END;
      }
      if (bucket_info.overflow_bucket == -1) return -1;
      bucket = bucket_info.overflow_bucket;
    }
  } else {
    int id = *(int *) value;
    while (1U) {
      CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      bucket_info = *(bucket_info_t *) block;
      block_base = block;
      block += sizeof(bucket_info_t);
      for (i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
        if (((Record *) block)->id == id) goto __SEARCH_END;
      }
      if (bucket_info.overflow_bucket == -1) return -1;
      bucket = bucket_info.overflow_bucket;
    }
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
  int index_descriptor = header_info.fileDesc;
  int bucket = (int) hash_function(header_info.attrType, header_info.numBuckets, value);
  int blocks_read = 0;
  int found = 0;
  if (header_info.attrType == 'c') {
    size_t value_len = strlen(value);
    size_t field_offset = get_attribute_offset(header_info.attrName, header_info.attrLength);
    do {
      void *block;
      CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      bucket_info_t bucket_info = *(bucket_info_t *) block;
      block += sizeof(bucket_info_t);
      for (size_t i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
        char *key = ((char *) (block + field_offset));
        if (!strncmp(key, value, value_len)) { found = 1; print_record(block); }
      }
      bucket = bucket_info.overflow_bucket;
      ++blocks_read;
    } while (bucket != -1);
  } else {
    int id = *(int *) value;
    do {
      void *block;
      CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      bucket_info_t bucket_info = *(bucket_info_t *) block;
      block += sizeof(bucket_info_t);
      for (size_t i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
        if (((Record *) block)->id == id) { found = 1; print_record(block); }
      }
      bucket = bucket_info.overflow_bucket;
      ++blocks_read;
    } while (bucket != -1);
  }
  return (!found) ? -1 : blocks_read;
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

  size_t identifier_len = strlen(SHT_FILE_IDENTIFIER);
  memcpy(block, SHT_FILE_IDENTIFIER, identifier_len);
  block += identifier_len;
  SHT_info *info = (SHT_info *) block;
  info->fileDesc = secondary_index_descriptor;
  info->attrLength = (size_t) attribute_length;
  info->numBuckets = (unsigned long) bucket_n;
  memcpy(&info->attrName, attribute_name, (size_t) attribute_length);
  memcpy(&info->fileName, index_name, strlen(index_name));

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

SHT_info *SHT_OpenSecondaryIndex(char *sfileName) {
  int sfd;  // Secondary index file decriptor.
  CHECK(sfd = BF_OpenFile(sfileName), BF_OPEN_EMSG, return NULL);

  void *block;
  CHECK(BF_ReadBlock(sfd, 0, &block), BF_READ_BLOCK_EMSG, return NULL);

  size_t identifier_len = strlen(SHT_FILE_IDENTIFIER);
  if (memcmp(block, SHT_FILE_IDENTIFIER, identifier_len) != 0) return NULL;
  block += identifier_len;

  SHT_info *info = (SHT_info *) block;
  SHT_info *sht_info = __MALLOC(1, SHT_info);
  if (sht_info == NULL) return NULL;

  sht_info->fileDesc = info->fileDesc;
  sht_info->fileName = info->fileName;
  sht_info->numBuckets = info->numBuckets;
  sht_info->attrName = __MALLOC(info->attrLength + 1, char);
  // This is going to work because the fileName gets stored last in the struct and the block get initialized with zeros
  // If that was not the case the we could just add a null terminating character when storing the name
  size_t index_name_len = strlen((const char *) &info->fileName);
  sht_info->fileName = __MALLOC(index_name_len + 1, char);
  STR_COPY(sht_info->attrName, &info->attrName, info->attrLength);
  STR_COPY(sht_info->fileName, &info->fileName, index_name_len);
  return sht_info;
}

int SHT_CloseSecondaryIndex(SHT_info *header_info) {
  if (header_info == NULL) return -1;
  CHECK(BF_CloseFile(header_info->fileDesc), BF_CLOSE_EMSG, return -1);
  free(header_info->attrName);
  free(header_info->fileName);
  free(header_info);
  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord sRecord) {
  int sfd = header_info.fileDesc;
  char *hash_attribute = get_hash_attribute('c', header_info.attrName, header_info.attrLength,
                                            &sRecord.record);
  if (hash_attribute == NULL) return -1;
  int bucket = (int) hash_function('c', header_info.numBuckets, hash_attribute);
  void *block;
  CHECK(BF_ReadBlock(sfd, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
  bucket_info_t bucket_info = *(bucket_info_t *) block;
  int current_bucket = bucket;
  while (bucket_info.free_space < sizeof(SHT_insert_info)) {
    if (bucket_info.overflow_bucket != -1) {
      CHECK(BF_ReadBlock(sfd, bucket_info.overflow_bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      current_bucket = bucket_info.overflow_bucket;
      bucket_info = *(bucket_info_t *) block;
    } else {
      CHECK(BF_AllocateBlock(sfd), BF_ALLOCATE_EMSG, return -1);
      CHECK(bucket_info.overflow_bucket = BF_GetBlockCounter(sfd) - 1, BF_GET_BLOCK_COUNTER_EMSG,
            return -1);
      memcpy(block, &bucket_info, sizeof(bucket_info_t));
      CHECK(BF_WriteBlock(sfd, current_bucket), BF_WRITE_BLOCK_EMSG, return -1);
      CHECK(BF_ReadBlock(sfd, bucket_info.overflow_bucket, &block), BF_READ_BLOCK_EMSG, return -1);
      current_bucket = bucket_info.overflow_bucket;
      initialize_block(block);
      CHECK(BF_WriteBlock(sfd, bucket_info.overflow_bucket), BF_WRITE_BLOCK_EMSG, return -1);
      bucket_info = *(bucket_info_t *) block;
      break;
    }
  }
  size_t attribute_length = strlen(hash_attribute);
  SHT_insert_info *insert_info = (SHT_insert_info *) (block + bucket_info.next_record);
  insert_info->block_id = sRecord.blockId;
  memcpy(&insert_info->value, hash_attribute, attribute_length);
  // That's the C we love!
  *(char *) (void *) (&insert_info->value + attribute_length) = '\0';

  size_t insert_info_size = offsetof(SHT_insert_info, value) + attribute_length + 1;
  bucket_info.next_record += insert_info_size;
  bucket_info.free_space -= insert_info_size;
  ++bucket_info.record_n;
  memcpy(block, &bucket_info, sizeof(bucket_info_t));
  CHECK(BF_WriteBlock(sfd, bucket), BF_WRITE_BLOCK_EMSG, return -1);
  return current_bucket;
}

static int HT_PrintAllEntriesFromSHT(const HT_info *ht_info, int bucket, char *value) {
  int index_descriptor = ht_info->fileDesc;
  int blocks_read = 0;
  size_t value_len = strlen(value);
  size_t field_offset = get_attribute_offset(ht_info->attrName, ht_info->attrLength);
  do {
    void *block;
    CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
    bucket_info_t bucket_info = *(bucket_info_t *) block;
    block += sizeof(bucket_info_t);
    for (size_t i = 0U; i != bucket_info.record_n; ++i, block += sizeof(Record)) {
      char *key = (char *) (block + field_offset);
      if (!strncmp(key, value, value_len)) print_record(block);
    }
    bucket = bucket_info.overflow_bucket;
    ++blocks_read;
  } while (bucket != -1);
  return blocks_read;
}

int SHT_SecondaryGetAllEntries(SHT_info sht_info, HT_info ht_info, void *value) {
  int index_descriptor = sht_info.fileDesc;
  int bucket = (int) hash_function('c', sht_info.numBuckets, value);
  int blocks_read = 0;
  ht_info.attrType = 'c';
  ht_info.attrName = sht_info.attrName;
  do {
    void *block;
    CHECK(BF_ReadBlock(index_descriptor, bucket, &block), BF_READ_BLOCK_EMSG, return -1);
    bucket_info_t bucket_info = *(bucket_info_t *) block;
    block += sizeof(bucket_info_t);
    for (size_t i = 0U; i != bucket_info.record_n; ++i) {
      SHT_insert_info *insert_info = (SHT_insert_info *) block;
      char *key = (char *) &insert_info->value;
      if (!strncmp(key, value, strlen(value)))
        blocks_read += HT_PrintAllEntriesFromSHT(&ht_info, insert_info->block_id, value);
      block += offsetof(SHT_insert_info, value) + strlen(key) + 1;
    }
    bucket = bucket_info.overflow_bucket;
    ++blocks_read;
  } while (bucket != -1);
  return blocks_read;
}

int HashStatistics(char *filename) {
  int fd;
  CHECK(fd = BF_OpenFile(filename), BF_OPEN_EMSG, return -1);
  void *block;
  CHECK(BF_ReadBlock(fd, 0, &block), BF_READ_BLOCK_EMSG, return -1);
  size_t ht_file_id_len = strlen(HT_FILE_IDENTIFIER);
  size_t sht_file_id_len = strlen(SHT_FILE_IDENTIFIER);
  int buckets;
  if (!memcmp(block, HT_FILE_IDENTIFIER, ht_file_id_len)) {
    block += ht_file_id_len;
    buckets = (int) ((HT_info *) block)->numBuckets;
  } else if (!memcmp(block, SHT_FILE_IDENTIFIER, sht_file_id_len)) {
    block += sht_file_id_len;
    buckets = (int) ((SHT_info *) block)->numBuckets;
  } else return -1;
  int total_blocks;
  CHECK(total_blocks = BF_GetBlockCounter(fd), BF_GET_BLOCK_COUNTER_EMSG, return -1);
  int total_records = 0;
  int *bucket_overlfow_blocks = __MALLOC(buckets, int);
  memset(bucket_overlfow_blocks, 0, buckets * sizeof(int));
  int min_records = BLOCK_SIZE / sizeof(Record) + 1;
  int max_records = 0;
  int buckets_with_overflow_blocks = 0U;
  for (size_t i = 1U; i <= buckets; ++i) {
    CHECK(BF_ReadBlock(fd, (int) i, &block), BF_READ_BLOCK_EMSG, {
      free(bucket_overlfow_blocks);
      return -1;
    });
    bucket_info_t bucket_info = *(bucket_info_t *) block;
    if (bucket_info.record_n > max_records) max_records = bucket_info.record_n;
    if (bucket_info.record_n < min_records) min_records = bucket_info.record_n;
    total_records += bucket_info.record_n;
    void *overflow_block;
    if (bucket_info.overflow_bucket != -1) ++buckets_with_overflow_blocks;
    while (bucket_info.overflow_bucket != -1) {
      CHECK(BF_ReadBlock(fd, bucket_info.overflow_bucket, &overflow_block), BF_READ_BLOCK_EMSG, {
        free(bucket_overlfow_blocks);
        return -1;
      });
      ++bucket_overlfow_blocks[i - 1U];
      bucket_info = *(bucket_info_t *) overflow_block;
    }
  }
  printf("\n================================= HASH STATISTICS =================================\n");
  printf("\tFile Blocks: %d\n"
         "\tMinimum Records in a bucket: %d\n"
         "\tMaximum Records in a bucket: %d\n"
         "\tAverage Records in a bucket: %.2f\n"
         "\tAverage number of Blocks per bucket: %.2f\n"
         "\tNumber of buckets with overflow blocks: %d\n",
         total_blocks, min_records, max_records,
         (float) total_records / (float) buckets,
         (float) total_blocks / (float) buckets,
         buckets_with_overflow_blocks);
  if (buckets_with_overflow_blocks) {
    printf("Overflow Blocks in buckets:\n");
    for (size_t i = 0U; i != buckets_with_overflow_blocks; ++i) {
      if (bucket_overlfow_blocks[i]) {
        printf("\tBucket[%zu]: %d\n", i + 1, bucket_overlfow_blocks[i]);
      }
    }
  }
  free(bucket_overlfow_blocks);
  return 0;
}