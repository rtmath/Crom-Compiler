#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

AST_Unary_Node *NewUnaryNode() {
  AST_Unary_Node *n = calloc(1, sizeof(AST_Unary_Node));
  n->node.arity = AST_UNARY;
  return n;
}

AST_Binary_Node *NewBinaryNode() {
  AST_Binary_Node *n = calloc(1, sizeof(AST_Binary_Node));
  n->node.arity = AST_BINARY;
  return n;
}

AST_Ternary_Node *NewTernaryNode() {
  AST_Ternary_Node *n = calloc(1, sizeof(AST_Ternary_Node));
  n->node.arity = AST_TERNARY;
  return n;
}

void SetLeftChild(AST_Node *dest, AST_Node *value) {
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

void SetRightChild(AST_Node *dest, AST_Node *value) {
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

void SetMiddleChild(AST_Node *dest, AST_Node *value) {
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
