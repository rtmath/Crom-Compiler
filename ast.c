#include <stdio.h>  // for printf
#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

static const char* const _NodeTypeTranslation[] =
{
  [UNTYPED] = "UNTYPED",
  [START_NODE] = "START",
  [CHAIN_NODE] = "CHAIN",
  [IF_NODE] = "IF",
  [FUNCTION_NODE] = "FUNCTION",
  [FUNCTION_RETURN_TYPE_NODE] = "RETURN TYPE",
  [FUNCTION_PARAM_NODE] = "FUNCTION PARAM",
  [FUNCTION_BODY_NODE] = "FUNCTION BODY",
};

const char *NodeTypeTranslation(NodeType t) {
  if (t < 0 || t >= NODE_TYPE_COUNT) { return "Out of bounds"; }
  return _NodeTypeTranslation[t];
}

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->type = type;
  n->annotation = a;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeFromToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = token;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->type = type;
  n->annotation = a;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeFromSymbol(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Symbol symbol) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = symbol.token;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->type = type;
  n->annotation = symbol.annotation;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->arity = arity;
  n->type = type;
  n->annotation = a;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

ParserAnnotation NoAnnotation() {
  ParserAnnotation a = {
    .ostensible_type = OST_UNKNOWN,
    .bit_width = 0,
    .is_signed = 0,
    .declared_on_line = -1,
    .is_array = 0,
    .array_size = 0,
  };

  return a;
}

static void PrintASTRecurse(AST_Node *node, int depth) {
  if (node == NULL) return;
  if (node->type != FUNCTION_RETURN_TYPE_NODE &&
      node->type != LITERAL_NODE &&
      node->type != IDENTIFIER_NODE &&
      node->nodes[LEFT]   == NULL &&
      node->nodes[MIDDLE] == NULL &&
      node->nodes[RIGHT]  == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < depth * 4 && i + node->token.length < 100; i++) {
    buf[i] = ' ';
  }
  buf[i] = '\0';

  if (node->token.type == UNINITIALIZED ||
      node->type == FUNCTION_RETURN_TYPE_NODE ||
      node->type == FUNCTION_BODY_NODE) {
    printf("%s%s ", buf, NodeTypeTranslation(node->type));
    InlinePrintAnnotation(node->annotation);
    printf("\n");
  } else {
    printf("%s%.*s ", buf,
        node->token.length,
        node->token.position_in_source);
    InlinePrintAnnotation(node->annotation);
    printf("\n");
  }

  PrintASTRecurse(node->nodes[LEFT], depth + 1);
  PrintASTRecurse(node->nodes[MIDDLE], depth + 1);
  PrintASTRecurse(node->nodes[RIGHT], depth + 1);
}

void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0);
}
