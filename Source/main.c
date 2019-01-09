#include <stdio.h>
#include <stdlib.h>
#include "../Include/BF.h"
#include "../Include/HT.h"

void test1(void) {
  char filename[] = "test_file";
  if (HT_CreateIndex(filename, 'c', "id", 2, 5) < 0) {
    BF_PrintError("Error creating index");
    exit(EXIT_FAILURE);
  }
  HT_info *info;
  if ((info = HT_OpenIndex(filename)) == NULL) {
    BF_PrintError("Error retrieving index");
    exit(EXIT_FAILURE);
  }
  HT_Print(info);
  if (HT_CloseIndex(info) < 0) {
    BF_PrintError("Error closing file");
    exit(EXIT_FAILURE);
  }
}

int main() {
  test1();
  return 0;
}