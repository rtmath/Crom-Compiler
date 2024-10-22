#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "token.h"

#define LEFT 0
#define RIGHT 1
#define MIDDLE 2

typedef enum {
  UNARY_ARITY,
  BINARY_ARITY,
  TERNARY_ARITY,
} Arity;

typedef enum {
  UNTYPED,
  START_NODE,
  CHAIN_NODE,
  IDENTIFIER_NODE,
  IF_NODE,
  TERMINAL_DATA,
  NODE_TYPE_COUNT
} NodeType;

/* Parser Related */
typedef enum {
  OST_UNKNOWN,
  OST_INT,
  OST_FLOAT,
  OST_BOOL,
  OST_CHAR,
  OST_STRING,
  OST_VOID,
  OST_ENUM,
  OST_STRUCT,
} OstensibleType;

typedef struct {
  OstensibleType ostensible_type;
  int bit_width; // for I8, U16, etc
  bool is_signed;
  int declared_on_line;
  bool is_array;
  int array_size;
} ParserAnnotation;

void InlinePrintAnnotation(ParserAnnotation);
/* End Parser Related*/

typedef struct AST_Node {
  Token token;
  Arity arity;
  NodeType type;
  ParserAnnotation annotation;

  struct AST_Node *nodes[3];
} AST_Node;

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation annotation);
AST_Node *NewNodeWithToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation annotation);
AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity, ParserAnnotation annotation);

const char *NodeTypeTranslation(NodeType t);

ParserAnnotation NoAnnotation();

void PrintAST(AST_Node *root);

#endif
