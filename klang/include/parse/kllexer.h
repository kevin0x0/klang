#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLLEXER_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLLEXER_H


#include "klang/include/parse/kltokens.h"
#include "klang/include/value/klvalue.h"
#include "utils/include/kio/kio.h"
#include "utils/include/kstring/kstring.h"

#include <stdint.h>

#define KLLEXER_SEV_ERROR   (1)
#define KLLEXER_SEV_FATAL   (2)


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

typedef struct tagKlLexer {
  Ki* ki;
  KlToken tok;
  uint8_t* buf;

  size_t currpos;
  size_t err_count;
  const char* stream_name;
} KlLexer;

bool kllexer_init(KlLexer* klex, Ki* ki);
void kllexer_destroy(KlLexer* klex);
KlLexer* kllexer_create(Ki* ki);
void kllexer_delete(KlLexer* klex);

bool kllexer_next(KlLexer* klex);
const char* kllexer_tok_info(KlLexer* klex, uint8_t kind);

void kllexer_throw_error(KlLexer* klex, int severity, const char** msg, size_t msg_count);

#endif
