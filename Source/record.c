#include <memory.h>
#include <stdio.h>
#include "../Include/record.h"
#include "../Include/macros.h"

Record create_record(int id, const char *restrict name, const char *restrict const surname,
                     const char *restrict const address) {
  Record record = {.id = id};
  STR_COPY(record.name, name, strlen(name));
  STR_COPY(record.surname, surname, strlen(surname));
  STR_COPY(record.address, address, strlen(address));
  return record;
}

void print_record(Record *record) {
  printf("ID: %d, Name: %s, Surname: %s, Address: %s\n",
         record->id, record->name, record->surname, record->address);
}