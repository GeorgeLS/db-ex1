#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../Include/BF.h"
#include "../Include/HT.h"
#include "../Include/macros.h"

#define RECORD_FILE_15K "../record_examples/records15K.txt"

void test1(void) {
  char filename[] = "test_file";
  BF_Init();
  if (HT_CreateIndex(filename, 'i', "id", 2, 5) < 0) {
    BF_PrintError("Error creating index");
    exit(EXIT_FAILURE);
  }
  HT_info *info;
  if ((info = HT_OpenIndex(filename)) == NULL) {
    BF_PrintError("Error retrieving index");
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0U; i != 100U; ++i) {
    Record record = {.id = (int) i};
    snprintf(record.name, sizeof(record.name), "Name_%zu", i);
    snprintf(record.surname, sizeof(record.surname), "Surname_%zu", i);
    snprintf(record.address, sizeof(record.address), "Address_%zu", i);
    int res = HT_InsertEntry(*info, record);
    printf("Insert res = %d\n", res);
  }
  Record record = {.id = 25, .name = "aa", .surname = "Surname_25", .address = "Address_25"};
  int res = HT_GetAllEntries(*info, &record.id);
  printf("Res = %d\n", res);
//  res = HT_DeleteEntry(*info, &record.id);
//  res = HT_GetAllEntries(*info, &record.id);
//  printf("Res = %d\n", res);

  char *secondaryIndex = "secondary.index";
  int err;
  err = SHT_CreateSecondaryIndex(secondaryIndex, "name", 4, 1, filename);
  SHT_info *sht_info = SHT_OpenSecondaryIndex(secondaryIndex);
  SecondaryRecord srecord;
  srecord.blockId = 1;
  srecord.record = record;
  SHT_SecondaryInsertEntry(*sht_info, srecord);
  SHT_SecondaryGetAllEntries(*sht_info, *info, "a"); // Doesnt find it, correct.
  if (HT_CloseIndex(info) < 0) {
    BF_PrintError("Error closing file");
    exit(EXIT_FAILURE);
  }
  HashStatistics(filename);
}

int main(int argc, char **argv) {
//  test1();
  int max_records = 15000;
  while (--argc){
    max_records = atoi(argv[argc]);
  }
  FILE *record_file = fopen(RECORD_FILE_15K, "r");
  char *line = NULL;
  size_t len = 0;
  int i = 0;
  int id; char* name; char* surname; char* address;
  name = __MALLOC(15, char);
  surname = __MALLOC(20, char);
  address = __MALLOC(40, char);


  // Create HT.
  char filename[] = "test.ht";
  BF_Init();
  if (HT_CreateIndex(filename, 'i', "id", 2, 1000) < 0) {
    BF_PrintError("Error creating index");
    exit(EXIT_FAILURE);
  }
  HT_info *info;
  if ((info = HT_OpenIndex(filename)) == NULL) {
    BF_PrintError("Error retrieving index");
    exit(EXIT_FAILURE);
  }

  while (getline(&line, &len, record_file) != EOF && i++ < max_records) {
    sscanf(line, "{ %lu , %[^,] , %[^,] , %[^}] ", &id, name, surname, address);
    Record record = {.id = id};
    snprintf(record.name, sizeof(record.name), "%s", name);
    snprintf(record.surname, sizeof(record.surname), "%s", surname);
    snprintf(record.address, sizeof(record.address), "%s", address);
    HT_InsertEntry(*info, record);
  }

  free(name); free(surname); free(address);

  printf("Searching for id: 1001\n");
  int search_criteria = 1001;
  HT_GetAllEntries(*info, &search_criteria);

  char *secondaryIndex = "test.sht";
  int err;
  err = SHT_CreateSecondaryIndex(secondaryIndex, "name", 4, 1000, filename);
  SHT_info *sht_info = SHT_OpenSecondaryIndex(secondaryIndex);
  int count = 0;
  Record record = {.id = id};
  snprintf(record.name, sizeof(record.name), "%s", "john");
  snprintf(record.surname, sizeof(record.surname), "%s", "doe");
  snprintf(record.address, sizeof(record.address), "%s", "Athens");
  int block_num = HT_InsertEntry(*info, record);
  SecondaryRecord sr = {.record = record, .blockId = block_num};
  SHT_SecondaryInsertEntry(*sht_info, sr);
  count = SHT_SecondaryGetAllEntries(*sht_info, *info, "john");
  printf("Blocks read until all records with name: %s are found: \t%d\n", "john", count);

  if (SHT_CloseSecondaryIndex(sht_info)){
    BF_PrintError("Error closing file");
    exit(EXIT_FAILURE);
  }
  if (HT_CloseIndex(info) < 0) {
    BF_PrintError("Error closing file");
    exit(EXIT_FAILURE);
  }
  return 0;
}