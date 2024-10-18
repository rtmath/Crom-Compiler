#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

static const char* const _NodeTypeTranslation[] =
{
  [UNTYPED] = "UNTYPED",
  [START_NODE] = "START_NODE",
  [CHAIN_NODE] = "CHAIN",
  [IF_NODE] = "IF_NODE",
};

const char *NodeTypeTranslation(NodeType t) {
  if (t < 0 || t >= NODE_TYPE_COUNT) { return "Out of bounds"; }
  return _NodeTypeTranslation[t];
}

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right) {
  AST_Node *n = calloc(1, sizeof(AST_Node));
  n->type = type;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token) {
  AST_Node *n = calloc(1, sizeof(AST_Node));
  n->token = token;
  n->type = type;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity) {
  AST_Node *n = calloc(1, sizeof(AST_Node));
  n->type = type;
  n->arity = arity;
  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}
