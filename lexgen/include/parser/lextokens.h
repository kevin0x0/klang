#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_LEXTOKENS_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_LEXTOKENS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LEX_DEAD    (255)

#define KEV_LTK_DEF (0)
#define KEV_LTK_IMPORT (1)
#define KEV_LTK_ID (2)
#define KEV_LTK_REGEX (3)
#define KEV_LTK_ASSIGN (4)
#define KEV_LTK_COLON (5)
#define KEV_LTK_BLANKS (6)
#define KEV_LTK_OPEN_PAREN (7)
#define KEV_LTK_CLOSE_PAREN (8)
#define KEV_LTK_ENV (9)
#define KEV_LTK_END (10)
#define KEV_LTK_STR (11)
#define KEV_LTK_NUM (12)
#define KEV_LTK_COMMA (13)

#endif

