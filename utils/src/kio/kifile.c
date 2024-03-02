#include "utils/include/kio/kifile.h"
#include "utils/include/os_spec/file.h"

#include <stdlib.h>

#define KIFILE_BUFSIZE  (8192)

typedef struct tagKiFile {
  Ki ki;
  FILE* file;
  size_t default_bufsize;
} KiFile;

size_t kifile_size(KiFile* kifile);
void kifile_reader(KiFile* kifile);
void kifile_close(KiFile* kifile);
void kifile_detach(KiFile* kifile);

static KiVirtualFunc kifile_create_vfunc = { .size = (KiSize)kifile_size, .delete = (KiDelete)kifile_close, .reader = (KiReader)kifile_reader };
static KiVirtualFunc kifile_attach_vfunc = { .size = (KiSize)kifile_size, .delete = (KiDelete)kifile_detach, .reader = (KiReader)kifile_reader };

Ki* kifile_create(const char* filepath) {
  FILE* file = fopen(filepath, "rb");
  if (!file) return NULL;
  KiFile* kifile = (KiFile*)malloc(sizeof (KiFile));
  if (!kifile) {
    fclose(file);
    return NULL;
  }
  kifile->file = file;
  kifile->default_bufsize = KIFILE_BUFSIZE;
  ki_init((Ki*)kifile, &kifile_create_vfunc);
  return (Ki*)kifile;
}

Ki* kifile_attach(FILE* file) {
  KiFile* kifile = (KiFile*)malloc(sizeof (KiFile));
  if (!kifile) return NULL;
  kifile->file = file;
  kifile->default_bufsize = KIFILE_BUFSIZE;
  ki_init((Ki*)kifile, &kifile_attach_vfunc);
  return (Ki*)kifile;
}

void kifile_reader(KiFile* kifile) {
  size_t readpos = ki_tell((Ki*)kifile);
  if (fseek(kifile->file, readpos, SEEK_SET)) {
    ki_setbuf((Ki*)kifile, ki_getbuf((Ki*)kifile), 0, readpos);
    return;
  }
  void* buf = ki_getbuf((Ki*)kifile);
  if (!buf && !(buf = malloc(kifile->default_bufsize)))
      return;
  size_t readsize = fread(buf, 1, kifile->default_bufsize, kifile->file);
  ki_setbuf((Ki*)kifile, buf, readsize, readpos);
}

void kifile_close(KiFile* kifile) {
  fclose(kifile->file);
  free(ki_getbuf((Ki*)kifile));
  free(kifile);
}

void kifile_detach(KiFile* kifile) {
  free(ki_getbuf((Ki*)kifile));
  free(kifile);
}

size_t kifile_size(KiFile* kifile) {
  return kev_file_size(kifile->file);
}
