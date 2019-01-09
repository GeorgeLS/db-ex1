
#include <memory.h>
#include <stddef.h>
#include "../Include/HT.h"
#include "../Include/BF.h"
#include "../Include/macros.h"

/**
 * HT_CreateIndex - Creates an index file
 * implementing static hashing techniques.
 *
 * @param index_name  A string of the index name.
 * @param attribute_type  A character indicating key type.
 * @param attribute_name  A string of the key name.
 * @param attribute_length  The length of the key type in bytes.
 * @param bucket_n  The number of buckets for the hash index.
 * @return  On success returns 0.
 * On failure returns the error values defined in BF.h
 */
int HT_CreateIndex (
        char* index_name,
        char attribute_type,
        char* attribute_name,
        int attribute_length,
        int bucket_n)
{
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

    void* block = __MALLOC_BYTES(BLOCK_SIZE);
    if (block == NULL)
        return BFE_NOMEM;

    if ((error = BF_ReadBlock(index_descriptor, 0, &block)) < 0)
        return error;

    HT_info ht_info = {
            .file_identifier = HT_FILE_IDENTIFIER,
            .index_descriptor = index_descriptor,
            .attribute_type = attribute_type,
            .attribute_name = attribute_name,
            .attribute_length = (size_t) attribute_length,
            .bucket_n = (unsigned long) bucket_n
    };

    memcpy(block, &ht_info, sizeof(HT_info));
    if ((error = BF_WriteBlock(index_descriptor, 0)) < 0)
        return error;

    int j;
    for (j = 0; j < bucket_n; j++)
    {
        if ((error = BF_AllocateBlock(index_descriptor)) < 0)
            return error;
    }

    if ((error = BF_CloseFile(index_descriptor)) < 0)
        return error;

    return 0;
}


/**
 * HT_OpenIndex - Opens an index file and reads the appropriate
 * info into an HT_info object.
 *
 * @param  index_name The name of the index file.
 * @return  On success returns a pointer to an HT_info object.
 * On failure returns NULL.
 */
HT_info* HT_OpenIndex(char* index_name)
{
    int index_descriptor;
    if ((index_descriptor = BF_OpenFile(index_name)) < 0)
        return NULL;

    void* block = __MALLOC_BYTES(BLOCK_SIZE);
    if (block == NULL)
        return NULL;

    if ((BF_ReadBlock(index_descriptor, 0, &block)) < 0)
        return NULL;

    HT_info* ht_info = __MALLOC(1, HT_info);
    memcpy(ht_info, block, sizeof(HT_info));
    if (ht_info->file_identifier != HT_FILE_IDENTIFIER)
        return NULL;

    return ht_info;
}


/**
 * HT_CloseIndex - Closes the index file.
 *
 * @param header_info The object that specifies the index file.
 * @return On success returns 0, otherwise -1.
 */
int HT_CloseIndex(HT_info* header_info)
{
    if (BF_CloseFile(header_info->index_descriptor) < 0)
        return -1;

    free(header_info);
    return 0;
}


int HT_InsertEntry(HT_info header_info, Record record)
{

}


void set_value_to_struct_field(void* object, void* value, const char* field_name)
{
    int offsetf = offsetof(object, field_name);
}