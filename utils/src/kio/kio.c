#include "utils/include/kio/kio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ki_nextbuf(Ki* ki);
static bool ko_nextbuf(Ko* ko, size_t *p_written_size);


static const void* ki_bufhandler_buf(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize);
static void ki_closestream_buf(void* data, size_t readpos, const void* buf, size_t bufsize);
static void* ko_bufhandler_buf(void* data, size_t next_readpos, size_t readpos, void* buf, size_t* p_bufsize);
static void ko_closestream_buf(void* data, size_t readpos, void* buf, size_t bufsize);
static const void* ki_bufhandler_str(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize);
static void ki_closestream_str(void* data, size_t readpos, const void* buf, size_t bufsize);

static const void* ki_bufhandler_file(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize);
static void ki_closestream_file(void* data, size_t readpos, const void* buf, size_t bufsize);
static void* ko_bufhandler_file(void* data, size_t next_readpos, size_t readpos, void* buf, size_t* p_bufsize);
static void ko_closestream_file(void* data, size_t readpos, void* buf, size_t bufsize);


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
  if (!(ki->buf = ki->handler(ki->data, ki->headpos + ki_bufsize(ki), ki->headpos, ki->buf, &buf_size))) {
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
  if (!(ki->buf = ki->handler(ki->data, offset, old_offset, ki->buf, &buf_size))) {
    if (!(ki->buf = ki->handler(ki->data, old_offset, 0, NULL, &buf_size))) {
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
  if (!(ko->buf = ko->handler(ko->data, ko->headpos + ko_bufsize(ko), ko->headpos, ko->buf, &buf_size))) {
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
  if (!(ko->buf = ko->handler(ko->data, offset, old_offset, ko->buf, &buf_size))) {
    if (!(ko->buf = ko->handler(ko->data, old_offset, 0, NULL, &buf_size))) {
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


static const void* ki_bufhandler_buf(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize) {
  KioReaderBufInfo* bufinfo = (KioReaderBufInfo*)data;
  if (next_readpos >= bufinfo->bufsize) {
    *p_bufsize = 0;
    return NULL;
  } else {
    *p_bufsize = bufinfo->bufsize - next_readpos;
    return bufinfo->buf + next_readpos;
  }
}

static void ki_closestream_buf(void* data, size_t streampos, const void* buf, size_t bufsize) {
  free(data);
}

typedef struct tagKioWriterBufInfo {
  void* buf;
  size_t bufsize;
} KioWriterBufInfo;


static void* ko_bufhandler_buf(void* data, size_t next_writepos, size_t writepos, void* buf, size_t* p_bufsize) {
  KioWriterBufInfo* bufinfo = (KioWriterBufInfo*)data;
  if (next_writepos >= bufinfo->bufsize) {
    *p_bufsize = 0;
    return NULL;
  } else {
    *p_bufsize = bufinfo->bufsize - next_writepos;
    return bufinfo->buf + next_writepos;
  }
}

static void ko_closestream_buf(void* data, size_t streampos, void* buf, size_t bufsize) {
  free(data);
}

static const void* ki_bufhandler_str(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize) {
  /* ignore 'next_readpos' and treat it as 0 */
  return data;
}

static void ki_closestream_str(void* data, size_t streampos, const void* buf, size_t bufsize) {
  free(data);
}

typedef struct tagKioReaderFileInfo {
  FILE* file;
  void* buf;
  size_t bufsize;
} KioReaderFileInfo;


static const void* ki_bufhandler_file(void* data, size_t next_readpos, size_t readpos, const void* buf, size_t* p_bufsize) {
  KioReaderFileInfo* fileinfo = (KioReaderFileInfo*)data;
  if (ftell(fileinfo->file) != next_readpos &&
      fseek(fileinfo->file, next_readpos, SEEK_SET) != 0) {
    *p_bufsize = 0;
    return NULL;
  }
  size_t readsize = fread(fileinfo->buf, 1, fileinfo->bufsize, fileinfo->file);
  *p_bufsize = readsize;
  return readsize == 0 ? NULL : fileinfo->buf;
}

static void ki_closestream_file(void* data, size_t streampos, const void* buf, size_t bufsize) {
  KioReaderFileInfo* fileinfo = (KioReaderFileInfo*)data;
  fclose(fileinfo->file);
  free(fileinfo->buf);
  free(fileinfo);
}

typedef struct tagKioWriterFileInfo {
  FILE* file;
  void* buf;
  size_t bufsize;
} KioWriterFileInfo;


static void* ko_bufhandler_file(void* data, size_t next_writepos, size_t writepos, void* buf, size_t* p_bufsize) {
  KioWriterFileInfo* fileinfo = (KioWriterFileInfo*)data;
  if (buf) { /* write */
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
  } else { /* initialize write */
    *p_bufsize = fileinfo->bufsize;
    return fileinfo->buf;
  }
}

static void ko_closestream_file(void* data, size_t streampos, void* buf, size_t bufsize) {
  KioWriterFileInfo* fileinfo = (KioWriterFileInfo*)data;
  /* write data */
  if (buf && (ftell(fileinfo->file) == streampos ||
        fseek(fileinfo->file, streampos, SEEK_SET) == 0)) {
    fwrite(buf, 1, bufsize, fileinfo->file);
  }
  fclose(fileinfo->file);
  free(fileinfo->buf);
  free(fileinfo);
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
  ki_init(ki, fileinfo, ki_bufhandler_file, ki_closestream_file);
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
  ko_init(ko, fileinfo, ko_bufhandler_file, ko_closestream_file);
  return true;
}

bool ki_init_buf(Ki* ki, const void* buf, size_t bufsize) {
  KioReaderBufInfo* bufinfo = (KioReaderBufInfo*)malloc(sizeof (KioReaderBufInfo));
  if (!bufinfo) return false;
  bufinfo->buf = buf;
  bufinfo->bufsize = bufsize;
  ki_init(ki, bufinfo, ki_bufhandler_buf, ki_closestream_buf);
  return true;
}

bool ko_init_buf(Ko* ko, void* buf, size_t bufsize) {
  KioWriterBufInfo* bufinfo = (KioWriterBufInfo*)malloc(sizeof (KioWriterBufInfo));
  if (!bufinfo) return false;
  bufinfo->buf = buf;
  bufinfo->bufsize = bufsize;
  ko_init(ko, bufinfo, ko_bufhandler_buf, ko_closestream_buf);
  return true;
}

void ki_init_string(Ki* ki, const char* str) {
  ki_init(ki, (void*)str, ki_bufhandler_str, ki_closestream_str);
}

