#include <stdio.h>  // for printf
#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

static const char* const _NodeTypeTranslation[] =
{
  [UNTYPED] = "UNTYPED",
  [START_NODE] = "Start",
  [CHAIN_NODE] = "Chain",
  [STATEMENT_NODE] = "Statement",
  [DECLARATION_NODE] = "Declaration",
  [IDENTIFIER_NODE] = "Identifier",
  [ENUM_IDENTIFIER_NODE] = "Enum Identifier",
  [ARRAY_SUBSCRIPT_NODE] = "Array Subscript",

  [IF_NODE] = "If",
  [WHILE_NODE] = "While",

  [BREAK_NODE] = "Break",
  [CONTINUE_NODE] = "Continue",
  [RETURN_NODE] = "Return",

  [FUNCTION_NODE] = "Function",
  [FUNCTION_RETURN_TYPE_NODE] = "Return Type",
  [FUNCTION_PARAM_NODE] = "Function Param",
  [FUNCTION_BODY_NODE] = "Function Body",

  [LITERAL_NODE] = "Literal",

  [ASSIGNMENT_NODE] = "Assignment",
  [UNARY_OP_NODE] = "Unary",
  [BINARY_OP_NODE] = "Binary",

  [PREFIX_INCREMENT_NODE] = "++Increment",
  [PREFIX_DECREMENT_NODE] = "--Decrement",
  [POSTFIX_INCREMENT_NODE] = "Increment++",
  [POSTFIX_DECREMENT_NODE] = "Decrement--",
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

static void PrintASTRecurse(AST_Node *node, int depth, int unindent) {
  #define NUM_INDENT_SPACES 2

  if (node == NULL) return;
  if (node->type == CHAIN_NODE &&
      node->nodes[LEFT] == NULL &&
      node->nodes[MIDDLE] == NULL &&
      node->nodes[RIGHT] == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < (depth * NUM_INDENT_SPACES) - unindent && i + node->token.length < 100; i++) {
    buf[i] = (i == 0) ? '|' : ' ';
  }
  buf[i] = '\0';

  printf("%s", buf);
  if (node->token.type != UNINITIALIZED) {
    printf("%.*s ", node->token.length, node->token.position_in_source);
  }

  if (node->annotation.actual_type != ACT_NOT_APPLICABLE) {
    InlinePrintActAnnotation(node->annotation);
    printf(" ");
  }

  if (node->type != UNTYPED &&
      node->type != CHAIN_NODE &&
      node->type != START_NODE) {
    printf("%s", NodeTypeTranslation(node->type));
  }

  printf("\n");

  if (node->type == CHAIN_NODE) unindent += NUM_INDENT_SPACES;
  PrintASTRecurse(node->nodes[LEFT], depth + 1, unindent);
  PrintASTRecurse(node->nodes[MIDDLE], depth + 1, unindent);
  PrintASTRecurse(node->nodes[RIGHT], depth + 1, unindent);

  #undef NUM_INDENT_SPACES
}

void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0, 0);
}

static void InlinePrintNodeSummary(AST_Node *node) {
  printf("%s Node -> %s Token",
         NodeTypeTranslation(node->type),
         TokenTypeTranslation(node->token.type));
  printf(" [OST ");
  InlinePrintOstAnnotation(node->annotation);
  printf(" : ACT ");
  InlinePrintActAnnotation(node->annotation);
  printf("]");
}

void PrintNode(AST_Node *node) {
  printf("%11s Node ", NodeTypeTranslation(node->type));
  printf("'%.*s'", node->token.length, node->token.position_in_source);
  printf(" [OST ");
  InlinePrintOstAnnotation(node->annotation);
  printf(" : ACT ");
  InlinePrintActAnnotation(node->annotation);
  printf("]");
  printf("\n");

  if (node->nodes[LEFT] != NULL) {
    printf("\n  LEFT: ");
    InlinePrintNodeSummary(node->nodes[LEFT]);
    printf("\n");
  }
  if (node->nodes[MIDDLE] != NULL) {
    printf("MIDDLE: ");
    InlinePrintNodeSummary(node->nodes[MIDDLE]);
    printf("\n");
  }
  if (node->nodes[RIGHT] != NULL) {
    printf(" RIGHT: ");
    InlinePrintNodeSummary(node->nodes[RIGHT]);
    printf("\n");
  }
  printf("----------------------------------------------------------\n");
}
