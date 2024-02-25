#include <stdio.h>

#define BUFSIZE   (100000)
int main(int argc, char *argv[]) {
  char buf[BUFSIZE + 10];
  for (int i = 0; i < BUFSIZE; ++i) {
    buf[i] = 'a';
  }
  for (int i = BUFSIZE; i < BUFSIZE + 10; ++i) {
    buf[i] = 'b';
  }
  buf[BUFSIZE + 9] = '\0';
  printf("%s\n", buf);
  return 0;
}
