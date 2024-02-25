#include "utils/include/kio/kio.h"
#include <stdlib.h>
#include <time.h>

#define KBUFSIZE  (BUFSIZ)
#define MBUFSIZE  (BUFSIZ)

int main(void) {
//  Ki ki;
//  if (!ki_init_open(&ki, "test.txt", KBUFSIZE)) {
//    fprintf(stderr, "failed to open file\n");
//    exit(EXIT_FAILURE);
//  }
//  Ko ko;
//  if (!ko_init_open(&ko, "test_output.txt", KBUFSIZE)) {
//    fprintf(stderr, "failed to open file\n");
//    exit(EXIT_FAILURE);
//  }
//
//  clock_t t = clock();
//  int ch = 0;
//  while ((ch = ki_getc(&ki)) != EOF) {
//    ko_putc(&ko, ch);
//  }
//  //char buf[MBUFSIZE];
//  //size_t len = 0;
//  //while ((len = ki_read(&ki, buf, MBUFSIZE))) {
//  //  ko_write(&ko, buf, len);
//  //}
//  fprintf(stderr, "%f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
//  ko_destroy(&ko);
//  ki_destroy(&ki);
//

  Ki* ki = kifile_create("test.txt");
  Ko* ko = kofile_create("out.txt");
  clock_t t = clock();
  int ch = ki_getc(ki);
  while (ch != EOF) {
    ko_putc(ko, ch);
    ch = ki_getc(ki);
  }
  fprintf(stderr, "done. duration = %f\n", (clock() - t) / (float)CLOCKS_PER_SEC);
  ki_delete(ki);
  ko_delete(ko);
  return 0;
}
