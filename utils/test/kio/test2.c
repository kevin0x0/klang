#include "utils/include/kio/kio.h"
#include <stdlib.h>
#include <time.h>

int main(void) {
  FILE* in = fopen("test.txt", "rb");

  FILE* out = fopen("test_output.txt", "wb");
  //int ch = 0;
  clock_t t = clock();
  char buf[4096];
  size_t len = 0;
  while ((len = fread(buf, 1, 4096, in))) {
    fwrite(buf, 1, len, out);
  }
  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  fclose(out);
  fclose(in);

  return 0;
}
