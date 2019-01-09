#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include "../Include/HT.h"
#include "../Include/BF.h"
#include "../Include/macros.h"

typedef struct HT_info_ {
  int index_descriptor;
  char attribute_type;
  char *attribute_name;
  size_t attribute_length;
  unsigned long int bucket_n;
} HT_info;

static __INLINE inline void HT_info_copy_to_block(HT_info *ht_info, void *block) {
  size_t offset = offsetof(HT_info, attribute_name);
  memcpy(block, &ht_info, offset);
  block += offset;
  memcpy(block, ht_info->attribute_name, (size_t) ht_info->attribute_length);
  block += ht_info->attribute_length;
  memcpy(block, BYTE_POINTER(ht_info) + offset + ht_info->attribute_length,
         sizeof(HT_info) - offsetof(HT_info, attribute_length));
}

int HT_CreateIndex(
        char *index_name,
        char attribute_type,
        char *attribute_name,
        int attribute_length,
        int bucket_n) {
  if (sizeof(HT_info) > BLOCK_SIZE)
    return HT_BLOCK_OVERFLOW;

  int error = 0;
  BF_Init();
  if ((error = BF_CreateFile(index_name)) < 0)
    return error;

  int index_descriptor = 0;
  if ((index_descriptor = BF_OpenFile(index_name)) < 0)
    return index_descriptor;

  if ((error = BF_AllocateBlock(index_descriptor)) < 0)
    return error;

  void *block = __MALLOC_BYTES(BLOCK_SIZE);
  if (block == NULL)
    return BFE_NOMEM;

  if ((error = BF_ReadBlock(index_descriptor, 0, &block)) < 0)
    return error;

  HT_info ht_info = {
          .index_descriptor = index_descriptor,
          .attribute_type = attribute_type,
          .attribute_name = attribute_name,
          .attribute_length = (size_t) attribute_length,
          .bucket_n = (unsigned long) bucket_n
  };

  HT_info_copy_to_block(&ht_info, block);

  if ((error = BF_WriteBlock(index_descriptor, 0)) < 0)
    return error;

  if ((error = BF_CloseFile(index_descriptor)) < 0)
    return error;

  return 0;
}

HT_info *HT_OpenIndex(char *index_name) {
  int index_descriptor;
  if ((index_descriptor = BF_OpenFile(index_name)) < 0)
    return NULL;

  void *block = malloc(BLOCK_SIZE);
  if (block == NULL)
    return NULL;

  if ((BF_ReadBlock(index_descriptor, 0, &block)) < 0)
    return NULL;

  HT_info *ht_info = __MALLOC(1, HT_info);
  memcpy(ht_info, block, sizeof(HT_info));
  // @TODO Ο Γιαννης μου είπε να ελεγχουμε εδώ εαν το αρχείο
  // @TODO είναι αρχείο κατακερματισμού και όχι οποιοδήποτε αρχείο.
  return ht_info;
}

int HT_CloseIndex(HT_info *header_info) {
  if (header_info == NULL) return -1;
  int error;
  if ((error = BF_CloseFile(header_info->index_descriptor)) < 0) {
    return error;
  }
  free(header_info);
}