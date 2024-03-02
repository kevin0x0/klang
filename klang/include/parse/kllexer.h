#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLLEXER_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLLEXER_H


#include "klang/include/parse/kltokens.h"
#include "klang/include/value/klvalue.h"
#include "utils/include/kio/kio.h"

#include <stdint.h>



typedef struct tagKlToken {
  size_t begin;
  size_t end;
  union {
    KlInt intval;
    char* string;
    char* id;
  } attr;
  uint8_t kind;
} KlToken;

typedef struct tagKlLex {
  Ki* ki;
  KlToken tok;
  uint8_t* buf;

  size_t currpos;
  size_t err_count;
  const char* stream_name;
} KlLex;

bool kllexer_init(KlLex* klex, Ki* ki);
void kllexer_destroy(KlLex* klex);
KlLex* kllexer_create(Ki* ki);
void kllexer_delete(KlLex* klex);

bool kllexer_next(KlLex* klex);
const char* kllexer_tok_info(KlLex* klex, uint8_t kind);

void kllexer_throw_error(KlLex* klex, int severity, const char** msg, size_t msg_count);

#endif
