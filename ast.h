#ifndef AST_H
#define AST_H

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

typedef struct AST_Node {
  Token token;
  Arity arity;
  NodeType type;

  struct AST_Node *nodes[3];
} AST_Node;

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right);
AST_Node *NewNodeWithToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token);
AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity);

const char *NodeTypeTranslation(NodeType t);

void PrintAST(AST_Node *root);

#endif
