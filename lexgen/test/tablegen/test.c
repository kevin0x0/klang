#include "lexgen/include/parser/regex.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t (*kev_lexgen_output_get_trans_256_u8(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
  uint8_t (*table)[256] = (uint8_t(*)[256])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      table[i][j] = 255;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

void output_table(uint8_t (*table)[256], KevFA* dfa, size_t* acc_mapping);

int main(int argc, char** argv) {
  KevFA* def = kev_regex_parse_ascii("def", NULL);
  KevFA* import = kev_regex_parse_ascii("import", NULL);
  KevFA* id = kev_regex_parse_ascii("[A-Za-z_\\-][A-Za-z0-9_\\-]* | \'([^\'\\\\] | \\.)+\'", NULL);
  KevFA* regex = kev_regex_parse_ascii("$\\ *[^\\n\\xFF]*", NULL);
  KevFA* assign = kev_regex_parse_ascii("=", NULL);
  KevFA* colon = kev_regex_parse_ascii(":", NULL);
  KevFA* blanks = kev_regex_parse_ascii("([\\ \\t\\n]*#[^\\n\\xFF]*)+[\\ \\t\\n]* | [\\ \\t\\n]+", NULL);
  KevFA* open_paren = kev_regex_parse_ascii("\\(", NULL);
  KevFA* close_paren = kev_regex_parse_ascii("\\)", NULL);
  KevFA* env_var_def = kev_regex_parse_ascii("%", NULL);
  KevFA* end = kev_regex_parse_ascii("\\xFF", NULL);
  KevFA* long_str = kev_regex_parse_ascii("!([^\\n\\xFF] | \\n[^\\n\\xFF])*(\\n\\n)?", NULL);
  KevFA* str = kev_regex_parse_ascii("\"[^\\n\\xFF]*", NULL);
  KevFA* number = kev_regex_parse_ascii("[0-9]+", NULL);
  KevFA* comma = kev_regex_parse_ascii(",", NULL);
  KevFA* nfa_array[] = { def, import, id, regex, assign, colon, blanks, open_paren, close_paren, env_var_def, end, long_str, str, number, comma, NULL };
  size_t* mapping = NULL;
  KevFA* dfa = kev_nfa_to_dfa(nfa_array, &mapping);
  KevFA* min_dfa = kev_dfa_minimization(dfa, mapping);
  uint8_t (*table)[256] = kev_lexgen_output_get_trans_256_u8(min_dfa);
  output_table(table, min_dfa, mapping);
  kev_fa_delete(id);
  kev_fa_delete(import);
  kev_fa_delete(regex);
  kev_fa_delete(assign);
  kev_fa_delete(colon);
  kev_fa_delete(blanks);
  kev_fa_delete(open_paren);
  kev_fa_delete(close_paren);
  kev_fa_delete(env_var_def);
  kev_fa_delete(end);
  kev_fa_delete(long_str);
  kev_fa_delete(str);
  kev_fa_delete(number);
  kev_fa_delete(comma);
  kev_fa_delete(dfa);
  kev_fa_delete(min_dfa);
  free(table);
  free(mapping);
  return 0;
}

void output_table(uint8_t (*table)[256], KevFA* dfa, size_t* acc_mapping) {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  
  FILE* cfile = fopen("lexgenlexer_table.c", "w");
  fprintf(cfile, "#include \"utils/include/general/global_def.h\"\n");
  /* transition table */
  fprintf(cfile, "static uint8_t table[%d][256] = {\n", (int)state_number);
  for (size_t i = 0; i < state_number; ++i) {
    fprintf(cfile, "  { /* %d */", (int)i);
    for (size_t j = 0; j < 256; ++j) {
      if (j % 16 == 0) fprintf(cfile, "\n    ");
      fprintf(cfile, "%4d,", table[i][j]);
    }
    fprintf(cfile, "\n  },\n");
  }
  fprintf(cfile, "\n};\n");
  /* accepting state mapping array */
  fprintf(cfile, "static int acc_array[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fprintf(cfile, "\n  ");
    fprintf(cfile, "  -1,");
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fprintf(cfile, "\n  ");
    fprintf(cfile, "%4d,", (int)acc_mapping[i - non_acc_number]);
  }
  fprintf(cfile, "\n};\n");
  /* start state */
  fprintf(cfile, "static size_t start = %d;\n", dfa->start_state->id);
  /* interface function */
  fprintf(cfile, "uint8_t (*kev_lexgenlexer_get_table(void))[256] {\n");
  fprintf(cfile, "  return table;\n");
  fprintf(cfile, "}\n");
  fprintf(cfile, "int* kev_lexgenlexer_get_acc_array(void) {\n");
  fprintf(cfile, "  return acc_array;\n");
  fprintf(cfile, "}\n");
  fprintf(cfile, "size_t kev_lexgenlexer_get_start_state(void) {\n");
  fprintf(cfile, "  return start;\n");
  fprintf(cfile, "}\n");
  fclose(cfile);
}
