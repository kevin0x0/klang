#include "include/parse/klparser.h"
#include "include/code/klcode.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"
#include "deps/k/include/kio/kibuf.h"

typedef struct tagKlBehaviour {
  Ki* input;
  Ko* dumpoutput;
  Ko* textoutput;
} KlBehaviour;


int main(int argc, char** argv) {
  kltodo("demo");
  return 0;
}


