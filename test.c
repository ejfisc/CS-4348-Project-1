#include <stdio.h>
#include <stdlib.h>

int main() {
  char line[] = "1000";
  char *ptr;
  long thousand = strtol(line, &ptr, 10);
  printf("%ld\n", thousand);
}
