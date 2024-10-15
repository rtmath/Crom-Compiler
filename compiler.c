#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdio.h>  // for printf and friends
#include <stdlib.h> // for calloc

#include "common.h"
#include "compiler.h"
#include "error.h"
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

static AST_Node *Type();
static AST_Node *Identifier();
static AST_Node *Number();
static AST_Node *Unary();
static AST_Node *Binary();
static AST_Node *Expression();

ParseRule Rules[] = {
  // Type Keywords
  [I8]             = {   Type,   NULL, NO_PRECEDENCE },
  [I16]            = {   Type,   NULL, NO_PRECEDENCE },
  [I32]            = {   Type,   NULL, NO_PRECEDENCE },
  [I64]            = {   Type,   NULL, NO_PRECEDENCE },

  [U8]             = {   Type,   NULL, NO_PRECEDENCE },
  [U16]            = {   Type,   NULL, NO_PRECEDENCE },
  [U32]            = {   Type,   NULL, NO_PRECEDENCE },
  [U64]            = {   Type,   NULL, NO_PRECEDENCE },

  [F32]            = {   Type,   NULL, NO_PRECEDENCE },
  [F64]            = {   Type,   NULL, NO_PRECEDENCE },

  [CHAR]           = {   Type,   NULL, NO_PRECEDENCE },
  [STRING]         = {   Type,   NULL, NO_PRECEDENCE },

  [BOOL]           = {   Type,   NULL, NO_PRECEDENCE },
  [VOID]           = {   Type,   NULL, NO_PRECEDENCE },
  [ENUM]           = {   Type,   NULL, NO_PRECEDENCE },
  [STRUCT]         = {   Type,   NULL, NO_PRECEDENCE },

  [IDENTIFIER]     = { Identifier, NULL, NO_PRECEDENCE },

  // Constants
  [INT_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [FLOAT_CONSTANT] = { Number,   NULL, NO_PRECEDENCE },

  [STRING_LITERAL] = {   NULL,   NULL, NO_PRECEDENCE },

  // Punctuators
  [PLUS]           = {   NULL, Binary,          TERM },
  [MINUS]          = {  Unary, Binary,          TERM },
  [ASTERISK]       = {   NULL, Binary,        FACTOR },
  [DIVIDE]         = {   NULL, Binary,        FACTOR },

  // Misc
  [TOKEN_EOF]      = {   NULL,   NULL,      PREC_EOF },
};

void Advance() {
  Parser.current = Parser.next;

  do {
    Parser.next = ScanToken();

    if (Parser.next.type != ERROR) break;
    // TODO: Report error
  } while (1);
}

bool Match(TokenType type) {
  if (Parser.next.type != type) return false;

  Advance();

  return true;
}

void Consume(TokenType type, const char *msg, ...) {
  if (Parser.next.type == type) {
    Advance();
    return;
  }

  // va_list necessary for passing '...' to another function
  va_list args;
  va_start(args, msg);

  ERROR_AND_CONTINUE(msg, args);

  va_end(args);
}

AST_Node *Parse(int PrecedenceLevel) {
  if (PrecedenceLevel == PREC_EOF) return NULL;
  Advance();

  AST_Node *return_node = NULL;

  ParseFn prefix_rule = Rules[Parser.current.type].prefix;
  if (prefix_rule == NULL) {
    printf("Prefix Rule for '%s' is NULL.\n",
        TokenTypeTranslation(Parser.current.type));
    return NULL;
  }

  AST_Node *prefix_node = prefix_rule();

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) {
      printf("Infix Rule for '%s' is NULL.\n",
          TokenTypeTranslation(Parser.current.type));
      break;
    }

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

static AST_Node *Type() {
  Token remember_token = Parser.current;
  AST_Node *n = NewNode();

  n->e.value = remember_token;

  // TODO: Expect identifier
  n->left = Parse(NO_PRECEDENCE);

  return n;
}

static AST_Node *Identifier() {
  AST_Node *n = NewNode();
  n->e.value = Parser.current;

  if (Match(EQUALS)) {
    n->left = Expression();
  }

  //Consume(SEMICOLON, "Expect ';' after declaration of '%.*s'.", n->e.value.length + ROOM_FOR_NULL_BYTE, n->e.value.position_in_source);
  ERROR_AND_EXIT("Expect ';' after declaration of '%.*s'.", n->e.value.length, n->e.value.position_in_source);

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

static AST_Node *Expression() {
  return Parse((Precedence)1);
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
