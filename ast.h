#ifndef AST_H
#define AST_H

#include "token.h"

#define AS_NODE(n) ((AST_Node*)n)
#define AS_UNARY(n) ((AST_Unary_Node*)n)
#define AS_BINARY(n) ((AST_Binary_Node*)n)
#define AS_TERNARY(n) ((AST_Ternary_Node*)n)

typedef enum {
  UNARY_ARITY,
  BINARY_ARITY,
  TERNARY_ARITY,
} Arity;

typedef enum {
  UNTYPED,
  START_NODE,
  STATEMENT_NODE,
} NodeType;

typedef struct {
  Token token;
  Arity arity;
  NodeType type;
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

AST_Unary_Node *NewUnaryNode(NodeType t);
AST_Binary_Node *NewBinaryNode(NodeType t);
AST_Ternary_Node *NewTernaryNode(NodeType t);

void SetLeftChild(AST_Node *dest, AST_Node *value);
void SetRightChild(AST_Node *dest, AST_Node *value);
void SetMiddleChild(AST_Node *dest, AST_Node *value);

#endif
