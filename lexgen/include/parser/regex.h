#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_REGEX_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_REGEX_H

#include "lexgen/include/parser/hashmap/strfa_map.h"
#include "kevfa/include/finite_automaton.h"
#include "utils/include/general/global_def.h"

#define KEV_REGEX_ERR_NONE              (0)
#define KEV_REGEX_ERR_INVALID_INPUT     (1)
#define KEV_REGEX_ERR_SYNTAX            (2)
#define KEV_REGEX_ERR_GENERATE          (3)

KevFA* kev_regex_parse(const uint8_t* regex, KevStringFaMap* map);
static inline KevFA* kev_regex_parse_ascii(const char* regex, KevStringFaMap* map);
uint64_t kev_regex_get_error(void);
const char* kev_regex_get_info(void);
size_t kev_regex_get_pos(void);

static inline KevFA* kev_regex_parse_ascii(const char* regex, KevStringFaMap* map) {
  return kev_regex_parse((const uint8_t*)regex, map);
}

#endif
