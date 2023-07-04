
#include "lexgen/include/finite_automaton/finite_automaton.h"
#include "lexgen/include/parser/regex.h"
#include "lexgen/include/table_driven_dfa/table_driven_dfa.h"
#include <stdio.h>
#include <stdlib.h>


void output_table(uint8_t (*table)[256], KevFA* dfa, size_t* acc_mapping);

int main(int argc, char** argv) {
  KevFA* def = kev_regex_parse_ascii("def", NULL);
  KevFA* name = kev_regex_parse_ascii("[A-Za-z_][A-Za-z0-9_]*", NULL);
  KevFA* regex = kev_regex_parse_ascii("$\\ *[^\\n]*", NULL);
  KevFA* assign = kev_regex_parse_ascii("=", NULL);
  KevFA* colon = kev_regex_parse_ascii(":", NULL);
  KevFA* blanks = kev_regex_parse_ascii("([\\ \\t\\n]*#[^\\n]*\\n)*[\\ \\t\\n]+", NULL);
  KevFA* open_paren = kev_regex_parse_ascii("\\(", NULL);
  KevFA* close_paren = kev_regex_parse_ascii("\\)", NULL);
  KevFA* end = kev_regex_parse_ascii("\xFF", NULL);
  KevFA* nfa_array[] = { def, name, regex, assign, colon, blanks, open_paren, close_paren, end, NULL };
  size_t* mapping = NULL;
  KevFA* dfa = kev_nfa_to_dfa(nfa_array, &mapping);
  KevFA* min_dfa = kev_dfa_minimization(dfa, mapping);
  uint8_t (*table)[256] = kev_get_table_256_u8(min_dfa);
  output_table(table, min_dfa, mapping);
  kev_fa_delete(name);
  kev_fa_delete(regex);
  kev_fa_delete(assign);
  kev_fa_delete(colon);
  kev_fa_delete(blanks);
  kev_fa_delete(open_paren);
  kev_fa_delete(close_paren);
  kev_fa_delete(end);
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
  fprintf(cfile, "#include \"lexgen/include/general/global_def.h\"\n");
  /* transition table */
  fprintf(cfile, "static uint8_t table[%d][256] = {\n", (int)state_number);
  for (size_t i = 0; i < state_number; ++i) {
    fprintf(cfile, "  {");
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
