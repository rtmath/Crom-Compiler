#ifndef AST_H
#define AST_H

#include "token.h"

#define AS_NODE(n) ((AST_Node*)n)
#define AS_UNARY(n) ((AST_Unary_Node*)n)
#define AS_BINARY(n) ((AST_Binary_Node*)n)
#define AS_TERNARY(n) ((AST_Ternary_Node*)n)

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

AST_Unary_Node *NewUnaryNode();
AST_Binary_Node *NewBinaryNode();
AST_Ternary_Node *NewTernaryNode();

void SetLeftChild(AST_Node *dest, AST_Node *value);
void SetRightChild(AST_Node *dest, AST_Node *value);
void SetMiddleChild(AST_Node *dest, AST_Node *value);

#endif
