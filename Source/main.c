#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../Include/BF.h"
#include "../Include/HT.h"
#include "../Include/macros.h"

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
  Record record = {.id = 25, .name = "Name_25", .surname = "Surname_25", .address = "Address_25"};
  int res = HT_GetAllEntries(*info, &record.id);
  printf("Res = %d\n", res);
//  res = HT_DeleteEntry(*info, &record.id);
//  res = HT_GetAllEntries(*info, &record.id);
//  printf("Res = %d\n", res);

  char *secondaryIndex = "secondary.index";
  int err;
  err = SHT_CreateSecondaryIndex(secondaryIndex, "name", 4, 5, filename);
  SHT_info *sht_info = SHT_OpenSecondaryIndex(secondaryIndex);
  SecondaryRecord srecord;
  srecord.blockId = 1;
  srecord.record = record;
  SHT_SecondaryInsertEntry(*sht_info, srecord);
  SHT_SecondaryGetAllEntries(*sht_info, *info, "Name_25");
  if (HT_CloseIndex(info) < 0) {
    BF_PrintError("Error closing file");
    exit(EXIT_FAILURE);
  }
  HashStatistics(filename);
}

int main() {
  test1();
  return 0;
}