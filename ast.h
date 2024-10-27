#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "parser_annotation.h"
#include "symbol_table.h"
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
  FUNCTION_NODE,
  FUNCTION_RETURN_TYPE_NODE,
  FUNCTION_PARAM_NODE,
  FUNCTION_BODY_NODE,
  LITERAL_NODE,
  ASSIGNMENT_NODE,
  PREFIX_INCREMENT_NODE,
  PREFIX_DECREMENT_NODE,
  POSTFIX_INCREMENT_NODE,
  POSTFIX_DECREMENT_NODE,
  NODE_TYPE_COUNT
} NodeType;

typedef struct AST_Node {
  Token token;
  Arity arity;
  NodeType type;
  ParserAnnotation annotation;

  struct AST_Node *nodes[3];
} AST_Node;

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation annotation);
AST_Node *NewNodeFromToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation annotation);
AST_Node *NewNodeFromSymbol(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Symbol symbol);
AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity, ParserAnnotation annotation);

const char *NodeTypeTranslation(NodeType t);

void PrintAST(AST_Node *root);

#endif
