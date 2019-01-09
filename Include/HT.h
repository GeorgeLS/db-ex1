//
// Created by aris on 8/1/2019.
//

#ifndef HT_H
#define HT_H

#include <stdlib.h>

#define HT_BLOCK_OVERFLOW -24
#define HT_FILE_IDENTIFIER "STATIC_HASH_TABLE"

typedef struct {
    const char* file_identifier;
    int index_descriptor;
    char attribute_type;
    char* attribute_name;
    size_t attribute_length;
    unsigned long int bucket_n;
} HT_info;

#endif //HT_H
