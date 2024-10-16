#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdio.h>  // for printf and friends
#include <stdlib.h> // for calloc

#include "common.h"
#include "compiler.h"
#include "error.h"
#include "lexer.h"

#define AS_NODE(n) ((AST_Node*)n)
#define AS_UNARY(n) ((AST_Unary_Node*)n)
#define AS_BINARY(n) ((AST_Binary_Node*)n)
#define AS_TERNARY(n) ((AST_Ternary_Node*)n)

struct {
  Token current;
  Token next;
} Parser;

typedef enum {
  AST_UNARY,
  AST_BINARY,
  AST_TERNARY
} AST_Arity;

typedef struct {
  Token token;
  AST_Arity arity;
} AST_Node;

typedef struct {
  AST_Node node;

  AST_Node *left;
} AST_Unary_Node;

typedef struct {
  AST_Node node;

  AST_Node *left;
  AST_Node *right;
} AST_Binary_Node;

typedef struct {
  AST_Node node;

  AST_Node *left;
  AST_Node *middle;
  AST_Node *right;
} AST_Ternary_Node;

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
static AST_Node *Parens();

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
  [LPAREN]         = { Parens,   NULL, NO_PRECEDENCE },
  [PLUS]           = {   NULL, Binary,          TERM },
  [MINUS]          = {  Unary, Binary,          TERM },
  [ASTERISK]       = {   NULL, Binary,        FACTOR },
  [DIVIDE]         = {   NULL, Binary,        FACTOR },

  // Misc
  [TOKEN_EOF]      = {   NULL,   NULL,      PREC_EOF },
};

static AST_Unary_Node *NewUnaryNode() {
  AST_Unary_Node *n = calloc(1, sizeof(AST_Unary_Node));
  n->node.arity = AST_UNARY;
  return n;
}

static AST_Binary_Node *NewBinaryNode() {
  AST_Binary_Node *n = calloc(1, sizeof(AST_Binary_Node));
  n->node.arity = AST_BINARY;
  return n;
}

static AST_Ternary_Node *NewTernaryNode() {
  AST_Ternary_Node *n = calloc(1, sizeof(AST_Ternary_Node));
  n->node.arity = AST_TERNARY;
  return n;
}

static void SetLeftChild(AST_Node *dest, AST_Node *value) {
  switch(dest->arity) {
    case AST_UNARY: {
      AS_UNARY(dest)->left = value;
    } break;
    case AST_BINARY: {
      AS_BINARY(dest)->left = value;
    } break;
    case AST_TERNARY: {
      AS_TERNARY(dest)->left = value;
    } break;
    default:
      ERROR_AND_CONTINUE("SetLeftChild(): Unknown arity '%d'\n", dest->arity);
      break;
  }
}

static void SetRightChild(AST_Node *dest, AST_Node *value) {
  switch(dest->arity) {
    case AST_UNARY: {
      ERROR_AND_CONTINUE("SetRightChild(): Cannot set right child of a unary node (TokenType %s).", TokenTypeTranslation(dest->token.type));
    } break;
    case AST_BINARY: {
      AS_BINARY(dest)->right = value;
    } break;
    case AST_TERNARY: {
      AS_TERNARY(dest)->right = value;
    } break;
    default:
      ERROR_AND_CONTINUE("SetRightChild(): Unknown arity '%d'\n", dest->arity);
      break;
  }
}

static void SetMiddleChild(AST_Node *dest, AST_Node *value) {
  switch(dest->arity) {
    case AST_UNARY: {
      ERROR_AND_CONTINUE("SetMiddleChild(): Cannot set middle child of a unary node (TokenType %s).", TokenTypeTranslation(dest->token.type));
    } break;
    case AST_BINARY: {
      ERROR_AND_CONTINUE("SetMiddleChild(): Cannot set middle child of a binary node (TokenType %s).", TokenTypeTranslation(dest->token.type));
    } break;
    case AST_TERNARY: {
      AS_TERNARY(dest)->middle = value;
    } break;
    default:
      ERROR_AND_CONTINUE("SetMiddleChild(): Unknown arity '%d'\n", dest->arity);
      break;
  }
}

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

  va_list args;
  va_start(args, msg);

  ERROR_AND_CONTINUE_VALIST(msg, args);

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
      SetLeftChild(infix_node, prefix_node);
    } else {
      SetLeftChild(infix_node, return_node);
      return_node = infix_node;
    }

    return_node = infix_node;
  }

  return (return_node == NULL) ? prefix_node : return_node;
}

static AST_Node *Number() {
  AST_Unary_Node *n = NewUnaryNode();

  n->node.token = Parser.current;

  return (AST_Node*)n;
}

static AST_Node *Type() {
  Token remember_token = Parser.current;
  AST_Unary_Node *n = NewUnaryNode();

  n->node.token = remember_token;

  // TODO: Expect identifier
  AST_Node *parse_result = Parse(NO_PRECEDENCE);
  SetLeftChild(AS_NODE(n), parse_result);

  return (AST_Node*)n;
}

static AST_Node *Identifier() {
  AST_Unary_Node *n = NewUnaryNode();
  n->node.token = Parser.current;

  if (Match(EQUALS)) {
    SetLeftChild(AS_NODE(n), Expression());
  }

  Consume(SEMICOLON, "Expect ';' after declaration of '%.*s'.", n->node.token.length, n->node.token.position_in_source);

  return (AST_Node*)n;
}

static AST_Node *Unary() {
  Token remember_token = Parser.current;

  AST_Unary_Node *n = NewUnaryNode();
  AST_Node *parse_result = Parse(UNARY);
  SetLeftChild(AS_NODE(n), parse_result);

  switch(remember_token.type) {
    case MINUS:
      n->node.token = remember_token;
      return (AST_Node*)n;
    default:
      printf("Unknown Unary operator '%s'\n",
          TokenTypeTranslation(remember_token.type));
      return (AST_Node*)n;
  }
}

static AST_Node *Binary() {
  Precedence precedence = Rules[Parser.current.type].precedence;
  Token remember_token = Parser.current;
  AST_Binary_Node *n = NewBinaryNode();

  AST_Node *parse_result = Parse(precedence + 1);
  SetRightChild(AS_NODE(n), parse_result);

  switch(remember_token.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
      n->node.token = remember_token;
      return (AST_Node*)n;
    default:
      printf("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
      return (AST_Node*)n;
  }
}

static AST_Node *Expression() {
  return Parse((Precedence)1);
}

static AST_Node *Parens() {
  AST_Node *n = Expression();
  Consume(RPAREN, "Missing ')' after expression");

  return n;
}

static void PrintASTRecurse(AST_Node *node, int depth, char label) {
  if (node == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < depth * 4 && i + node->token.length < 100; i++) {
    buf[i] = ' ';
  }
  buf[i] = '\0';
  printf("%s%c: %.*s\n", buf, label,
      node->token.length,
      node->token.position_in_source);

  PrintASTRecurse(AS_UNARY(node)->left, depth + 1, 'L');
  if (node->arity == AST_TERNARY) PrintASTRecurse(AS_TERNARY(node)->middle, depth + 1, 'M');
  if (node->arity == AST_BINARY) PrintASTRecurse(AS_BINARY(node)->right, depth + 1, 'R');
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
