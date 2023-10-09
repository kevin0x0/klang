#include "utils/include/kio/kio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ki_nextbuf(Ki* ki);
static bool ko_nextbuf(Ko* ko, size_t *p_written_size);


static const void* ki_bufreader_buf(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize);
static void* ko_bufwriter_buf(void* data, size_t next_readpos, size_t readpos, void* buf, size_t* p_bufsize);
static const void* ki_bufreader_str(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize);

static const void* ki_bufreader_file(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize);
static void* ko_bufwriter_file(void* data, size_t next_readpos, size_t readpos, void* buf, size_t* p_bufsize);


size_t ki_read(Ki* ki, void* buf, size_t buf_size) {
  void* buf_end = buf + buf_size;
  void* bufpos = buf;
  while (bufpos != buf_end) {
    if (ki->readpos == ki->buf_end) {
      if (!ki_nextbuf(ki)) break;
    }
    size_t read_size = buf_end - bufpos < ki->buf_end - ki->readpos ?
                       buf_end - bufpos : ki->buf_end - ki->readpos;
    memcpy(bufpos, ki->readpos, read_size);
    bufpos += read_size;
    ki->readpos += read_size;
  }
  return bufpos - buf;
}

int ki_fill_buf(Ki* ki) {
  return ki_nextbuf(ki) ? (int)*ki->readpos++ : EOF;
}

static bool ki_nextbuf(Ki* ki) {
  size_t old_bufsize = ki_bufsize(ki);
  size_t buf_size = old_bufsize;
  if (!(ki->buf = ki->reader(ki->data, ki->headpos + ki_bufsize(ki), ki->headpos, ki->buf, &buf_size))) {
    buf_size = 0;
  }
  ki->buf_end = ki->buf + buf_size;
  ki->readpos = ki->buf;
  ki->headpos += old_bufsize;
  return ki->buf != NULL;
}

bool ki_seek(Ki* ki, size_t offset) {
  if (offset >= ki->headpos && offset < ki->headpos + ki_bufsize(ki)) {
    ki->readpos = ki->buf + offset - ki->headpos;
    return true;
  }
  size_t buf_size = ki_bufsize(ki);
  size_t old_offset = ki_tell(ki);
  bool succeed = true;
  if (!(ki->buf = ki->reader(ki->data, offset, old_offset, ki->buf, &buf_size))) {
    if (!(ki->buf = ki->reader(ki->data, old_offset, 0, NULL, &buf_size))) {
      buf_size = 0;
    }
    succeed = false;
  }
  ki->buf_end = ki->buf + buf_size;
  ki->readpos = ki->buf;
  ki->headpos = succeed ? offset : old_offset;
  return succeed;
}

size_t ko_write(Ko* ko, void* buf, size_t buf_size) {
  void* buf_end = buf + buf_size;
  void* bufpos = buf;
  while (bufpos != buf_end) {
    size_t write_size = buf_end - bufpos < ko->buf_end - ko->writepos ?
                        buf_end - bufpos : ko->buf_end - ko->writepos;
    memcpy(ko->writepos, bufpos, write_size);
    bufpos += write_size;
    ko->writepos += write_size;
    if (ko->writepos == ko->buf_end) {
      size_t written_size = 0;
      if (!ko_nextbuf(ko, &written_size)) {
        bufpos = bufpos - write_size + written_size;
        break;
      }
    }
  }
  return bufpos - buf;
}

bool ko_write_buf(Ko* ko, int ch) {
  if (ko_nextbuf(ko, NULL)) {
    *ko->writepos++ = (char)ch;
    return true;
  }
  return false;
}

static bool ko_nextbuf(Ko* ko, size_t *p_written_size) {
  size_t old_bufsize = ko_bufsize(ko);
  size_t buf_size = old_bufsize;
  if (!(ko->buf = ko->writer(ko->data, ko->headpos + ko_bufsize(ko), ko->headpos, ko->buf, &buf_size))) {
    if (p_written_size)
      *p_written_size = buf_size;
    buf_size = 0;
    old_bufsize = buf_size;
  }
  ko->buf_end = ko->buf + buf_size;
  ko->writepos = ko->buf;
  ko->headpos += old_bufsize;
  return ko->buf != NULL;
}

bool ko_seek(Ko* ko, size_t offset) {
  if (offset == ko->headpos + ko->writepos - ko->buf)
    return true;

  /* whenever you actually move the offset, flush the buffer */
  size_t buf_size = ko->writepos - ko->buf;
  size_t old_offset = ko_tell(ko);
  bool succeed = true;
  if (!(ko->buf = ko->writer(ko->data, offset, old_offset, ko->buf, &buf_size))) {
    if (!(ko->buf = ko->writer(ko->data, old_offset, 0, NULL, &buf_size))) {
      buf_size = 0;
    }
    succeed = false;
  }
  ko->buf_end = ko->buf + buf_size;
  ko->writepos = ko->buf;
  ko->headpos = succeed ? offset : old_offset;
  return succeed;
}

typedef struct tagKioReaderBufInfo {
  const void* buf;
  size_t bufsize;
} KioReaderBufInfo;


static const void* ki_bufreader_buf(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize) {
  KioReaderBufInfo* bufinfo = (KioReaderBufInfo*)data;
  if (!buf || next_readpos != readpos) { /* read */
    if (next_readpos >= bufinfo->bufsize) {
      *p_bufsize = 0;
      return NULL;
    } else {
    *p_bufsize = bufinfo->bufsize - next_readpos;
    return bufinfo->buf + next_readpos;
    }
  } else { /* end of read */
    free(data);
    return NULL;
  }
}

typedef struct tagKioWriterBufInfo {
  void* buf;
  size_t bufsize;
} KioWriterBufInfo;


static void* ko_bufwriter_buf(void* data, size_t next_writepos, size_t writepos, void* buf, size_t* p_bufsize) {
  KioWriterBufInfo* bufinfo = (KioWriterBufInfo*)data;
  if (!buf || next_writepos != writepos) { /* write */
    if (next_writepos >= bufinfo->bufsize) {
      *p_bufsize = 0;
      return NULL;
    } else {
    *p_bufsize = bufinfo->bufsize - next_writepos;
    return bufinfo->buf + next_writepos;
    }
  } else { /* end of write */
    free(data);
    return NULL;
  }
}

static const void* ki_bufreader_str(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize) {
  if (!buf || next_readpos != readpos) { /* read */
    /* ignore 'next_readpos' and treat it as 0 */
    return data;
  } else { /* end of read */
    return NULL;
  }
}

typedef struct tagKioReaderFileInfo {
  FILE* file;
  void* buf;
  size_t bufsize;
} KioReaderFileInfo;


static const void* ki_bufreader_file(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize) {
  KioReaderFileInfo* fileinfo = (KioReaderFileInfo*)data;
  if (!buf || readpos != next_readpos) { /* read */
    if (ftell(fileinfo->file) != next_readpos &&
        fseek(fileinfo->file, next_readpos, SEEK_SET) != 0) {
      *p_bufsize = 0;
      return NULL;
    }
    size_t readsize = fread(fileinfo->buf, 1, fileinfo->bufsize, fileinfo->file);
    *p_bufsize = readsize;
    return readsize == 0 ? NULL : fileinfo->buf;
  } else { /* end of read */
    fclose(fileinfo->file);
    free(fileinfo->buf);
    free(fileinfo);
    return NULL;
  }
}

typedef struct tagKioWriterFileInfo {
  FILE* file;
  void* buf;
  size_t bufsize;
} KioWriterFileInfo;


static void* ko_bufwriter_file(void* data, size_t next_writepos, size_t writepos, void* buf, size_t* p_bufsize) {
  KioWriterFileInfo* fileinfo = (KioWriterFileInfo*)data;
  if (!buf) { /* initialize write */
    *p_bufsize = fileinfo->bufsize;
    return fileinfo->buf;
  } else if (writepos != next_writepos) { /* write */
    if (ftell(fileinfo->file) != writepos &&
        fseek(fileinfo->file, writepos, SEEK_SET) != 0) {
      *p_bufsize = 0;
      return NULL;
    }
    size_t writesize = fwrite(fileinfo->buf, 1, *p_bufsize, fileinfo->file);
    fflush(fileinfo->file);
    if (*p_bufsize == writesize) {  /* write successfully */
      *p_bufsize = fileinfo->bufsize;
      return fileinfo->buf;
    } else {  /* failed to write data */
      /* *p_bufsize will tell kio how many data is successfully written
       * when failed to write all */
      *p_bufsize = writesize; 
      return NULL;
    }
  } else { /* end of read */
    /* write data */
    if (ftell(fileinfo->file) == writepos ||
        fseek(fileinfo->file, writepos, SEEK_SET) == 0) {
      fwrite(fileinfo->buf, 1, *p_bufsize, fileinfo->file);
    }
    fclose(fileinfo->file);
    free(fileinfo->buf);
    free(fileinfo);
    return NULL;
  }
}

bool ki_init_open(Ki* ki, const char* filepath, size_t bufsize) {
  FILE* file = fopen(filepath, "rb");
  if (!file) return false;
  if (!ki_init_file(ki, file, bufsize)) {
    fclose(file);
    return false;
  }
  return true;
}

bool ko_init_open(Ko* ko, const char* filepath, size_t bufsize) {
  FILE* file = fopen(filepath, "wb");
  if (!file) return false;
  if (!ko_init_file(ko, file, bufsize)) {
    fclose(file);
    return false;
  }
  return true;
}

bool ki_init_file(Ki* ki, FILE* file, size_t bufsize) {
  KioReaderFileInfo* fileinfo = (KioReaderFileInfo*)malloc(sizeof (KioReaderFileInfo));
  void* buffer = malloc(bufsize);
  if (!fileinfo || !buffer) {
    free(fileinfo);
    free(buffer);
    return false;
  }
  fileinfo->file = file;
  fileinfo->bufsize = bufsize;
  fileinfo->buf = buffer;
  ki_init(ki, fileinfo, ki_bufreader_file);
  return true;
}

bool ko_init_file(Ko* ko, FILE* file, size_t bufsize) {
  KioWriterFileInfo* fileinfo = (KioWriterFileInfo*)malloc(sizeof (KioWriterFileInfo));
  void* buffer = malloc(bufsize);
  if (!fileinfo || !buffer) {
    free(fileinfo);
    free(buffer);
    return false;
  }
  fileinfo->file = file;
  fileinfo->bufsize = bufsize;
  fileinfo->buf = buffer;
  ko_init(ko, fileinfo, ko_bufwriter_file);
  return true;
}

bool ki_init_buf(Ki* ki, const void* buf, size_t bufsize) {
  KioReaderBufInfo* bufinfo = (KioReaderBufInfo*)malloc(sizeof (KioReaderBufInfo));
  if (!bufinfo) return false;
  bufinfo->buf = buf;
  bufinfo->bufsize = bufsize;
  ki_init(ki, bufinfo, ki_bufreader_buf);
  return true;
}

bool ko_init_buf(Ko* ko, void* buf, size_t bufsize) {
  KioWriterBufInfo* bufinfo = (KioWriterBufInfo*)malloc(sizeof (KioWriterBufInfo));
  if (!bufinfo) return false;
  bufinfo->buf = buf;
  bufinfo->bufsize = bufsize;
  ko_init(ko, bufinfo, ko_bufwriter_buf);
  return true;
}

void ki_init_string(Ki* ki, const char* str) {
  ki_init(ki, (void*)str, ki_bufreader_str);
}

