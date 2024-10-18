#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

AST_Unary_Node *NewUnaryNode(NodeType t) {
  AST_Unary_Node *n = calloc(1, sizeof(AST_Unary_Node));
  n->node.arity = UNARY_ARITY;
  n->node.type = t;
  return n;
}

AST_Binary_Node *NewBinaryNode(NodeType t) {
  AST_Binary_Node *n = calloc(1, sizeof(AST_Binary_Node));
  n->node.arity = BINARY_ARITY;
  n->node.type = t;
  return n;
}

AST_Ternary_Node *NewTernaryNode(NodeType t) {
  AST_Ternary_Node *n = calloc(1, sizeof(AST_Ternary_Node));
  n->node.arity = TERNARY_ARITY;
  n->node.type = t;
  return n;
}

void SetLeftChild(AST_Node *dest, AST_Node *value) {
  switch(dest->arity) {
    case UNARY_ARITY: {
      AS_UNARY(dest)->left = value;
    } break;
    case BINARY_ARITY: {
      AS_BINARY(dest)->left = value;
    } break;
    case TERNARY_ARITY: {
      AS_TERNARY(dest)->left = value;
    } break;
    default:
      ERROR_AND_CONTINUE_FMTMSG("SetLeftChild(): Unknown arity '%d'\n", dest->arity);
      break;
  }
}

void SetRightChild(AST_Node *dest, AST_Node *value) {
  switch(dest->arity) {
    case UNARY_ARITY: {
      ERROR_AND_CONTINUE_FMTMSG("SetRightChild(): Cannot set right child of a unary node (TokenType %s).", TokenTypeTranslation(dest->token.type));
    } break;
    case BINARY_ARITY: {
      AS_BINARY(dest)->right = value;
    } break;
    case TERNARY_ARITY: {
      AS_TERNARY(dest)->right = value;
    } break;
    default:
      ERROR_AND_CONTINUE_FMTMSG("SetRightChild(): Unknown arity '%d'\n", dest->arity);
      break;
  }
}

void SetMiddleChild(AST_Node *dest, AST_Node *value) {
  switch(dest->arity) {
    case UNARY_ARITY: {
      ERROR_AND_CONTINUE_FMTMSG("SetMiddleChild(): Cannot set middle child of a unary node (TokenType %s).", TokenTypeTranslation(dest->token.type));
    } break;
    case BINARY_ARITY: {
      ERROR_AND_CONTINUE_FMTMSG("SetMiddleChild(): Cannot set middle child of a binary node (TokenType %s).", TokenTypeTranslation(dest->token.type));
    } break;
    case TERNARY_ARITY: {
      AS_TERNARY(dest)->middle = value;
    } break;
    default:
      ERROR_AND_CONTINUE_FMTMSG("SetMiddleChild(): Unknown arity '%d'\n", dest->arity);
      break;
  }
}
