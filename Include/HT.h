#ifndef HT_H
#define HT_H

#include <stdlib.h>
#include "attributes.h"

#define HT_BLOCK_OVERFLOW -24

typedef struct HT_info_ HT_info;

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
 * @param header_info A pointer to a HT_info object
 * @return On success returns 0
 * On failure returns -1
 */
__NO_DISCARD int HT_CloseIndex(HT_info *header_info);

#endif //HT_H
