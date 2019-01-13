#include <stdio.h>
#include <stdlib.h>
#include "../Include/BF.h"
#include "../Include/HT.h"

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
  HT_Print(info);
  for (size_t i = 0U; i != 100U; ++i) {
    Record record = {.id = (int) i};
    snprintf(record.name, sizeof(record.name), "Name_%zu", i);
    snprintf(record.surname, sizeof(record.surname), "Surname_%zu", i);
    snprintf(record.address, sizeof(record.address), "Address_%zu", i);
    int res = HT_InsertEntry(*info, record);
  }
  if (HT_CloseIndex(info) < 0) {
    BF_PrintError("Error closing file");
    exit(EXIT_FAILURE);
  }
}

int main() {
  test1();
  return 0;
}