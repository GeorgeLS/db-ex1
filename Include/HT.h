#ifndef HT_H
#define HT_H

#include <stdlib.h>
#include "attributes.h"
#include "record.h"

#define HT_BLOCK_OVERFLOW -24
#define HT_FILE_IDENTIFIER "STATIC_HASH_TABLE"
#define SHT_FILE_IDENTIFIER "SECONDARY_STATIC_HASH_TABLE"

typedef struct {
  int index_descriptor;
  char attribute_type;
  size_t attribute_length;
  char *attribute_name;
  unsigned long int bucket_n;
} HT_info;

typedef struct {
  int secondary_index_descriptor;
  size_t attribute_length;
  char *attribute_name;
  unsigned long int bucket_n;
  char *index_name;
} SHT_info;

/**
 * HT_CreateIndex - Creates an index file
 * implementing static hashing techniques.
 *
 * @param index_name  A string of the index name.
 * @param attribute_type  A character indicating key type.
 * @param attribute_name  A string of the key name.
 * @param attribute_length  The length of the key type in bytes.
 * @param buckets  The number of buckets for the hash index.
 * @return  On success returns 0.
 * On failure returns the error values defined in BF.h
 */
__NO_DISCARD int HT_CreateIndex(char *index_name, char attribute_type, char *attribute_name,
                                int attribute_length, int bucket_n) __NON_NULL(1, 3);

/**
 * HT_OpenIndex - Opens an index file and reads the appropriate
 * info into an HT_info object.
 *
 * @param  index_name The name of the index file.
 * @return  On success returns a pointer to an HT_info object.
 * On failure returns NULL.
 */
__NO_DISCARD HT_info *HT_OpenIndex(char *index_name) __NON_NULL(1);

/**
 * HT_CloseIndex - Closes the index file associated with the file descriptor
 * in the HT_info object
 * @param header_info A pointer to an HT_info object
 * @return On success returns 0
 * On failure returns -1
 */
__NO_DISCARD int HT_CloseIndex(HT_info *header_info);

/**
 * HT_InsertEntry - Inserts a new entry to the index file associated with header info
 * @param header_info The header info
 * @param record The record to insert
 * @return On success returns the block number that the record got inserted
 * On failure returns -1
 */
__NO_DISCARD int HT_InsertEntry(HT_info header_info, Record record);

/**
 * HT_DeleteEntry - Deletes the entry with id equal to value
 * @param header_info The header info from which we take the static hashing file information
 * @param value The value of the id of the Record to delete
 * @return On success returns 0
 * On failure returns -1
 */
__NO_DISCARD int HT_DeleteEntry(HT_info header_info, void *value) __NON_NULL(2);

/**
 * HT_GetAllEntries - Prints all the records whose primary key is equal to value
 * @param header_info The header info from which we take the static hashing file information
 * @param value The value of the id of the Records to print
 * @return On success returns the number of blocks read until we found all the records
 * On failure returns -1
 */
__NO_DISCARD int HT_GetAllEntries(HT_info header_info, void *value) __NON_NULL(2);

/**
 * SHT_CreateSecondaryIndex - Creates a secondary index file for the primary index file
 * implementing static hashing techniques.
 *
 * @param index_name  A string of the secondary index name.
 * @param attribute_name  A string of the key name.
 * @param attribute_length  The length of the key type in bytes.
 * @param buckets  The number of buckets for the hash index.
 * @param index_name The string of the primary index name.
 * @return  On success returns 0.
 * On failure returns the error values defined in BF.h
 */
__NO_DISCARD int SHT_CreateSecondaryIndex(char *secondary_index_name, char *attribute_name,
                                          int attribute_length, int bucket_n, char *index_name) __NON_NULL(1, 2, 5);

/**
 * SHT_OpenSecondaryIndex - Opens the secondary index file and reads it's info
 * into a SHT_info object.
 *
 * @param sfileName A string of the secondary index file name.
 * @return On Success returns a pointer to a SHT_info object, otherwise NULL.
 */
__NO_DISCARD SHT_info *SHT_OpenSecondaryIndex(char *sfileName) __NON_NULL(1);

/**
 * SHT_CloseIndex - Closes the index file associated with the file descriptor
 * in the SHT_info object
 * @param header_info A pointer to an SHT_info object
 * @return On success returns 0
 * On failure returns -1
 */
__NO_DISCARD int SHT_CloseSecondaryIndex(SHT_info *header_info) __NON_NULL(1);

/**
 * SHT_SecondaryInsertEntry - Inserts a new entry to the secondary index.
 * the record must exist in the primary index.
 *
 * @param header_info  Info about the secondary index.
 * @param record  The SecondaryRecord to be inserted.
 * @return On success returns 0, otherwise -1.
 */
__NO_DISCARD int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record);

/**
 * SHT_SecondaryGetAllEntries -
 * @param sht_info The secondary header info
 * @param ht_info The primary header info
 * @return On success returns the number of blocks read until we found all the records
 * On failure returns -1
 */
__NO_DISCARD int SHT_SecondaryGetAllEntries(SHT_info sht_info, HT_info ht_info);

#endif //HT_H
