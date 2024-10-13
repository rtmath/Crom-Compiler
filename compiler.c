#include <stdio.h>  // for printf and friends
#include <stdlib.h> // for calloc

#include "common.h"
#include "compiler.h"
#include "lexer.h"

struct {
  Token current;
  Token next;
} Parser;

typedef struct {
  Token value;
} AST_Entry;

typedef struct AST_Node {
  AST_Entry e;
  struct AST_Node *left;
  struct AST_Node *right;
} AST_Node;

AST_Node *ParseTree;

typedef enum {
  PREC_EOF = -1,
  NO_PRECEDENCE,
  TERM,
  FACTOR,
  UNARY,
} Precedence;

typedef AST_Node* (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static AST_Node *Number();
static AST_Node *Unary();
static AST_Node *Binary();

ParseRule Rules[] = {
  [TOKEN_EOF]      = {   NULL,   NULL,      PREC_EOF },
  [INT_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [FLOAT_CONSTANT] = { Number,   NULL, NO_PRECEDENCE },
  [PLUS]           = {   NULL, Binary,          TERM },
  [MINUS]          = {  Unary, Binary,          TERM },
  [ASTERISK]       = {   NULL, Binary,        FACTOR },
  [DIVIDE]         = {   NULL, Binary,        FACTOR }
};

void Advance() {
  Parser.current = Parser.next;

  do {
    Parser.next = ScanToken();

    if (Parser.next.type != ERROR) break;
    // TODO: Report error
  } while (1);
}

AST_Node *Parse(int PrecedenceLevel) {
  if (PrecedenceLevel == PREC_EOF) return NULL;
  Advance();

  AST_Node *return_node = NULL;

  ParseFn prefix_rule = Rules[Parser.current.type].prefix;
  if (prefix_rule == NULL) /* TODO: Report error */ return NULL;

  AST_Node *prefix_node = prefix_rule();

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) /* TODO: Report error? */ break;

    AST_Node *infix_node = infix_rule();

    if (return_node == NULL) {
      infix_node->left = prefix_node;
    } else {
      infix_node->left = return_node;
      return_node = infix_node;
    }

    return_node = infix_node;
  }

  return (return_node == NULL) ? prefix_node : return_node;
}

static AST_Node *NewNode() {
  return calloc(1, sizeof(AST_Node));
}

static AST_Node *Number() {
  AST_Node *n = NewNode();

  n->e.value = Parser.current;

  return n;
}

static AST_Node *Unary() {
  Token remember_token = Parser.current;

  AST_Node *n = NewNode();
  n->left = Parse(UNARY);

  switch(remember_token.type) {
    case MINUS:
      n->e.value = remember_token;
      return n;
    default:
      printf("Unknown Unary operator '%s'\n",
          TokenTypeTranslation(remember_token.type));
      return n;
  }
}

static AST_Node *Binary() {
  TokenType operator_type = Parser.current.type;
  ParseRule *rule = &Rules[Parser.current.type];
  Token remember_token = Parser.current;

  AST_Node *n = NewNode();
  n->right = Parse(rule->precedence + 1); // This will flush Parser.current before it can be used below

  switch(operator_type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
      n->e.value = remember_token;
      return n;
    default:
      printf("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(operator_type));
      return n;
  }
}

static void PrintASTRecurse(AST_Node *node, int depth, char label) {
  if (node == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < depth * 4 && i + node->e.value.length < 100; i++) {
    buf[i] = ' ';
  }
  buf[i] = '\0';
  printf("%s%c: %.*s\n", buf, label,
      node->e.value.length,
      node->e.value.position_in_source);

  PrintASTRecurse(node->left, depth + 1, 'L');
  PrintASTRecurse(node->right, depth + 1, 'R');
}

static void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0, 'S');
}

void Compile(const char *source) {
  InitLexer(source);

  Advance();
  ParseTree = Parse(NO_PRECEDENCE);
  if (ParseTree == NULL) {
    printf("[%s:%d] Parse() returned NULL. ParseTree could not be created.\n", __FILE__, __LINE__);
    return;
  }

  printf("\n[AST]\n");
  PrintAST(ParseTree);
}
