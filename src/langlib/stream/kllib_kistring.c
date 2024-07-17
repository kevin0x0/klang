#include "include/langlib/stream/kllib_kistring.h"
#include "include/mm/klmm.h"
#include "include/value/klstring.h"
#include <stdlib.h>
#include <string.h>

typedef struct tagKiString {
  Ki ki;
  KlString* str;
} KiString;

static void kistring_delete(KiString* kistring);
static KioFileOffset kistring_size(KiString* kistring);
static void kistring_reader(KiString* kistring);

static const KiVirtualFunc kistring_vfunc = { .reader = (KiReader)kistring_reader, .delete = (KiDelete)kistring_delete, .size = (KiSize)kistring_size };


Ki* kistring_create(KlString* str) {
  KiString* kistring = (KiString*)malloc(sizeof (KiString));
  if (!kistring) return NULL;
  kistring->str = str;
  ki_init((Ki*)kistring, &kistring_vfunc);
  ki_setbuf((Ki*)kistring, klstring_content(str), klstring_length(str), 0);
  return (Ki*)kistring;
}

static void kistring_delete(KiString* kistring) {
  free(kistring);
}

static KioFileOffset kistring_size(KiString* kistring) {
  return klstring_length(kistring->str);
}

static void kistring_reader(KiString* kistring) {
  size_t readpos = ki_tell((Ki*)kistring);
  if (readpos >= klstring_length(kistring->str)) {
    ki_setbuf((Ki*)kistring, ki_getbuf((Ki*)kistring), 0, readpos);
    return;
  }
  ki_setbuf((Ki*)kistring, klstring_content(kistring->str), klstring_length(kistring->str), 0);
  ki_setbufcurr((Ki*)kistring, readpos);
}

KlGCObject* kistring_prop(const KiString* kistring, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  klmm_gcobj_mark(klmm_to_gcobj(kistring->str), gclist);
  return gclist;
}
