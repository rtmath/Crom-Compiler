#include <stdio.h>  // for printf
#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

static void _InlinePrintAnnotation(const char *s, int bit_width, int is_signed) {
  printf("[Annotation(%s:%d:%s)]", s, bit_width, (is_signed) ? "SIGNED" : "UNSIGNED");
}

void InlinePrintAnnotation(ParserAnnotation a) {
  switch (a.ostensible_type) {
    case OST_UNKNOWN: {
    } break;
    case OST_INT: {
      _InlinePrintAnnotation("INTEGER", a.bit_width, a.is_signed);
    } break;
    case OST_FLOAT: {
      _InlinePrintAnnotation("FLOAT", a.bit_width, a.is_signed);
    } break;
    case OST_BOOL: {
      _InlinePrintAnnotation("BOOL", a.bit_width, a.is_signed);
    } break;
    case OST_CHAR: {
      _InlinePrintAnnotation("CHAR", a.bit_width, a.is_signed);
    } break;
    case OST_STRING: {
      _InlinePrintAnnotation("STRING", a.bit_width, a.is_signed);
    } break;
    case OST_VOID: {
      _InlinePrintAnnotation("VOID", a.bit_width, a.is_signed);
    } break;
    case OST_ENUM: {
      _InlinePrintAnnotation("ENUM", a.bit_width, a.is_signed);
    } break;
    case OST_STRUCT: {
      _InlinePrintAnnotation("STRUCT", a.bit_width, a.is_signed);
    } break;
  }
}

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

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));
  n->type = type;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->annotation = a;
  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));
  n->token = token;
  n->type = type;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->annotation = a;
  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));
  n->type = type;
  n->arity = arity;
  n->annotation = a;
  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

static void PrintASTRecurse(AST_Node *node, int depth, char label) {
  if (node == NULL) return;
  if (node->type != TERMINAL_DATA &&
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

  if (node->token.type == UNINITIALIZED) {
    printf("%s%c: <%s> ", buf, label, NodeTypeTranslation(node->type));
    InlinePrintAnnotation(node->annotation);
    printf("\n");
  } else {
    printf("%s%c: %.*s ", buf, label,
        node->token.length,
        node->token.position_in_source);
    InlinePrintAnnotation(node->annotation);
    printf("\n");
  }

  PrintASTRecurse(node->nodes[LEFT], depth + 1, 'L');
  PrintASTRecurse(node->nodes[MIDDLE], depth + 1, 'M');
  PrintASTRecurse(node->nodes[RIGHT], depth + 1, 'R');
}

void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0, 'S');
}
