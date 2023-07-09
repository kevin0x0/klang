#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
/* options */
#define KEV_LEXGEN_OPT_STAGE          (0)
#define KEV_LEXGEN_OPT_WIDTH          (1)
#define KEV_LEXGEN_OPT_CHARSET        (2)
#define KEV_LEXGEN_OPT_HELP           (3)
#define KEV_LEXGEN_OPT_NO             (4)

/* values */
#define KEV_LEXGEN_OPT_LANG_C_CPP     (0)
#define KEV_LEXGEN_OPT_LANG_RUST      (1)
#define KEV_LEXGEN_OPT_STA_TAB        (0)
#define KEV_LEXGEN_OPT_STA_SRC        (1)
#define KEV_LEXGEN_OPT_STA_ARC        (2)
#define KEV_LEXGEN_OPT_STA_SHA        (3)
#define KEV_LEXGEN_OPT_CHARSET_ASCII  (0)
#define KEV_LEXGEN_OPT_CHARSET_UTF8   (1)
#define KEV_LEXGEN_OPT_FALSE          (0)
#define KEV_LEXGEN_OPT_TRUE           (1)

/* string parameters */
#define KEV_LEXGEN_INPUT_PATH         (0)
#define KEV_LEXGEN_OUT_TAB_PATH       (1)
#define KEV_LEXGEN_LANG_NAME          (2)
#define KEV_LEXGEN_BUILD_TOOL_NAME    (3)
#define KEV_LEXGEN_OUT_SRC_PATH       (4)
#define KEV_LEXGEN_OUT_INC_PATH       (5)
#define KEV_LEXGEN_OUT_ARC_PATH       (6)
#define KEV_LEXGEN_OUT_SHA_PATH       (7)
#define KEV_LEXGEN_STR_NO             (8)

typedef struct tagKevOptions {
  int opts[KEV_LEXGEN_OPT_NO];
  char* strs[KEV_LEXGEN_STR_NO];
} KevOptions;

#endif
