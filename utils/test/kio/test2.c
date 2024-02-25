#include "utils/include/kio/kio.h"
#include <string.h>
#include <time.h>
#include <stdbool.h>


int main(void) {
  while (true) {
    const char* str = "hello, world!\n";
    char wbuf[10240];
    Ki* ki = kibuf_create(str, strlen(str));
    Ko* ko = kobuf_create(wbuf, sizeof (wbuf));

    int len = 0;
    for (int i = 0; i < 10; ++i) {
      //size_t size = ki_read(ki, buf, sizeof (buf));
      ki_seek(ki, 0);
      len += ko_printf(ko, "%d time %s", i, str);
    }
    wbuf[len] = '\0';
    ki_delete(ki);
    ko_delete(ko);
    printf("%s", wbuf);
  }
  return 0;
}


