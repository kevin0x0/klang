#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


#include <ctype.h>

static int token_id = 0;
static float attrval = 0.0;
void lex_next(FILE* stream);


#define ACTION_SHIFT      (1)
#define ACTION_REDUCE     (2)
#define ACTION_ACCEPT     (3)
#define ACTION_ERROR      (0)

typedef float Attr;

typedef struct tagSymbol {
  Attr attr;
  int16_t state;
} Symbol;

typedef struct tagActionEntry {
  uint8_t action;
  uint16_t info;
} ActionEntry;

typedef struct tagRuleInfo {
  uint8_t head_id;
  uint8_t bodylen;
} RuleInfo;

typedef void LRCallback(Symbol* stk);

static ActionEntry action_tbl[18][8];
static int16_t goto_tbl[18][9];
static RuleInfo rules_info[8];
static LRCallback* callbacks[8];
static const char* symbol_name[9];
static int16_t start_state;
static int state_symbol_mapping[18];


Attr parse(FILE* input);
static Symbol* symstk_expand(Symbol* symstk, Symbol* symstk_end);

Attr parse(FILE* input) {
  lex_next(input);

  uint8_t _token = token_id;
  int16_t _state = start_state;

  /* symbol stack definition */
  Symbol* _symstk = (Symbol*)malloc(16 * sizeof (Symbol*));
  if (!_symstk) {
    fprintf(stderr, "out of memory", NULL);
    exit(EXIT_FAILURE);
  }
  /* ensure the cell above the top of stack is always available, so the capacity of
   * the stack is set to the actual capacity minus 1.
   */
  Symbol* _symstk_end = _symstk + 15;
  Symbol* _symstk_curr = _symstk;

  _symstk_curr++->state = _state;

  while (true) {
    ActionEntry action = action_tbl[_state][_token];
    switch (action.action) {
      case ACTION_SHIFT: {
        if (_symstk_curr == _symstk_end) {
          size_t old_capacity = _symstk_end - _symstk + 1;
          _symstk = symstk_expand(_symstk, _symstk_end);
          _symstk_end = _symstk + old_capacity * 2 - 1;
          _symstk_curr = _symstk + old_capacity - 1;
        }
        _state = action.info;
        Attr attr;
        attr = attrval;
        _symstk_curr->attr = attr;
        _symstk_curr++->state = _state;
        lex_next(input);
        _token = token_id;
        break;
      }
      case ACTION_REDUCE: {
        RuleInfo ruleinfo = rules_info[action.info];
        size_t rulelen = ruleinfo.bodylen;
        _symstk_curr -= rulelen;
        if (callbacks[action.info])
          callbacks[action.info](_symstk_curr);
        if (_symstk_curr == _symstk_end) {
          size_t old_capacity = _symstk_end - _symstk + 1;
          _symstk = symstk_expand(_symstk, _symstk_end);
          _symstk_end = _symstk + old_capacity * 2 - 1;
          _symstk_curr = _symstk + old_capacity - 1;
        }
        _state = goto_tbl[(_symstk_curr - 1)->state][ruleinfo.head_id];
        _symstk_curr++->state = _state;
        break;
      }
      case ACTION_ACCEPT: {
        Attr ret = (_symstk_curr - 1)->attr;
        free(_symstk);
        
        return ret;
      }
      case ACTION_ERROR: {
        fprintf(stderr, "error occurred!\n");
        exit(EXIT_FAILURE);
        free(_symstk);
        break;
      }
      default: {
        fprintf(stderr, "impossible state\n");
        exit(EXIT_FAILURE);
        break;
      }
    }
  }
}


static ActionEntry action_tbl[18][8] = {
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 0,   -1 }, { 2,    7 }, { 2,    7 }, { 2,    7 }, { 2,    7 }, { 0,   -1 }, { 2,    7 }, { 2,    7 }, 
  },
  {
    { 0,   -1 }, { 1,   11 }, { 1,   12 }, { 1,   10 }, { 1,    9 }, { 0,   -1 }, { 0,   -1 }, { 3,   -1 }, 
  },
  {
    { 0,   -1 }, { 2,    4 }, { 2,    4 }, { 2,    4 }, { 2,    4 }, { 0,   -1 }, { 2,    4 }, { 2,    4 }, 
  },
  {
    { 0,   -1 }, { 2,    5 }, { 2,    5 }, { 2,    5 }, { 2,    5 }, { 0,   -1 }, { 2,    5 }, { 2,    5 }, 
  },
  {
    { 0,   -1 }, { 1,   11 }, { 1,   12 }, { 1,   10 }, { 1,    9 }, { 0,   -1 }, { 1,   13 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 1,    4 }, { 1,    1 }, { 1,    2 }, { 0,   -1 }, { 0,   -1 }, { 1,    3 }, { 0,   -1 }, { 0,   -1 }, 
  },
  {
    { 0,   -1 }, { 2,    6 }, { 2,    6 }, { 2,    6 }, { 2,    6 }, { 0,   -1 }, { 2,    6 }, { 2,    6 }, 
  },
  {
    { 0,   -1 }, { 2,    3 }, { 2,    3 }, { 2,    3 }, { 2,    3 }, { 0,   -1 }, { 2,    3 }, { 2,    3 }, 
  },
  {
    { 0,   -1 }, { 2,    2 }, { 2,    2 }, { 2,    2 }, { 2,    2 }, { 0,   -1 }, { 2,    2 }, { 2,    2 }, 
  },
  {
    { 0,   -1 }, { 2,    0 }, { 2,    0 }, { 1,   10 }, { 1,    9 }, { 0,   -1 }, { 2,    0 }, { 2,    0 }, 
  },
  {
    { 0,   -1 }, { 2,    1 }, { 2,    1 }, { 1,   10 }, { 1,    9 }, { 0,   -1 }, { 2,    1 }, { 2,    1 }, 
  },
};



static int16_t goto_tbl[18][9] = {
  {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,   5,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,   6,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,   7,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,   8,
  }, {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  13,  -1,  -1,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,  14,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,  15,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,  16,
  }, {
       4,   1,   2,  -1,  -1,   3,  -1,  -1,  17,
  }, {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  }, {
      -1,  11,  12,  10,   9,  -1,  -1,  -1,  -1,
  },
};



static RuleInfo rules_info[8] = {
  {    8,    3 }, {    8,    3 }, {    8,    3 }, {    8,    3 }, {    8,    2 }, {    8,    2 }, {    8,    3 }, {    8,    1 }, 
};



static void _action_callback_0(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(1) == 0 ? 0 : (1) - 1].attr + stk[(3) == 0 ? 0 : (3) - 1].attr; 
}

static void _action_callback_1(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(1) == 0 ? 0 : (1) - 1].attr - stk[(3) == 0 ? 0 : (3) - 1].attr; 
}

static void _action_callback_2(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(1) == 0 ? 0 : (1) - 1].attr * stk[(3) == 0 ? 0 : (3) - 1].attr; 
}

static void _action_callback_3(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(1) == 0 ? 0 : (1) - 1].attr / stk[(3) == 0 ? 0 : (3) - 1].attr; 
}

static void _action_callback_4(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(2) == 0 ? 0 : (2) - 1].attr; 
}

static void _action_callback_5(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = - stk[(2) == 0 ? 0 : (2) - 1].attr; 
}

static void _action_callback_6(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(2) == 0 ? 0 : (2) - 1].attr; 
}

static void _action_callback_7(Symbol* stk) {
 stk[(0) == 0 ? 0 : (0) - 1].attr = stk[(1) == 0 ? 0 : (1) - 1].attr; 
}

static LRCallback* callbacks[8] = {
  _action_callback_0,
  _action_callback_1,
  _action_callback_2,
  _action_callback_3,
  _action_callback_4,
  _action_callback_5,
  _action_callback_6,
  _action_callback_7,
};



static const char* symbol_name[9] = {
  "num",
  "+",
  "-",
  "*",
  "/",
  "(",
  ")",
  "$",
  "expr",
};



static int16_t start_state = 0;



static int state_symbol_mapping[18] = {
    -1,   1,   2,   5,   0,   8,   8,   8,   8,   4,   3,   1,   2,   6,   8,   8,
     8,   8,
};




static Symbol* symstk_expand(Symbol* symstk, Symbol* symstk_end) {
  size_t old_size = symstk_end - symstk + 1;
  size_t new_size = old_size * 2;
  Symbol* newstk = (Symbol*)realloc(symstk, new_size * sizeof (Symbol*));
  if (!newstk) {
    fprintf(stderr, "out of memory", NULL);
    exit(EXIT_FAILURE);
  }
  symstk = newstk;
  return symstk;
}


void lex_next(FILE* stream) {
  int ch = fgetc(stream);
  while (ch == ' ' || ch == '\t')
    ch = fgetc(stream);
  if (isdigit(ch)) {
    ungetc(ch, stream);
    fscanf(stream, "%f", &attrval);
    token_id = 0;
  } else if (ch == '+') {
    token_id = 1;
  } else if (ch == '-') {
    token_id = 2;
  } else if (ch == '*') {
    token_id = 3;
  } else if (ch == '/') {
    token_id = 4;
  } else if (ch == '(') {
    token_id = 5;
  } else if (ch == ')') {
    token_id = 6;
  } else if (ch == EOF || ch == '\n') {
    token_id = 7;
  } else {
    fprintf(stderr, "lexical error\n");
    lex_next(stream);
  }
}

int main(int argc, char** argv) {
  while (true) {
    printf(">>>");
    float result = parse(stdin);
    printf("%f\n", result);
  }
  return 0;
}
