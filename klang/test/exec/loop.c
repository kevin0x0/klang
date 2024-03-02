#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {
  size_t count = strtoull(argv[1], NULL, 10);
  size_t i = 0;
  size_t sum = 0;
  for (i = 0; i < count; ++i)
    sum += i;
  printf("%zu\n", sum);
  return 0;
}
