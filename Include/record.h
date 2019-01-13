#ifndef DB_EX1_RECORD_H
#define DB_EX1_RECORD_H

#include "attributes.h"

typedef struct {
  int id;
  char name[15];
  char surname[20];
  char address[40];
} Record;

/**
 * create_record - Creates a new record
 * @param id The id of the record
 * @param name The name of the record
 * @param surname The surname of the record
 * @param address The address of the record
 * @return Returns a new object Record with values the given parameters
 */
__NO_DISCARD Record create_record(int id, const char *restrict name,
                                  const char *restrict surname,
                                  const char *restrict address) __NON_NULL(2, 3, 4);

/**
 * print_record - Prints the information about the given record
 * @param record The record whose information will be printed
 */
void print_record(Record *record) __NON_NULL(1);

#endif //DB_EX1_RECORD_H
