#include <stdio.h>

#include "error.h"
#include "parser_annotation.h"
#include "type_checker.h"

static bool TypeIs(ActualType type, AST_Node *node) {
  if (node == NULL) return false;

  return node->annotation.actual_type == type;
}

void VerifyTypeIs(ActualType type, AST_Node *node) {
  if (TypeIs(type, node)) return;

  ERROR_AT_TOKEN(node->token,
                 "Type disagreement, expected type '%s', got type '%s'",
                 ActualTypeTranslation(type),
                 ActualTypeTranslation(node->annotation.actual_type));
}

void CheckTypeDisagreement(AST_Node *a, AST_Node *b) {
  if (a->annotation.actual_type == b->annotation.actual_type) return;

  ERROR_AT_TOKEN(b->token,
                 "Type disagreement between: %s and %s",
                 ActualTypeTranslation(a->annotation.actual_type),
                 ActualTypeTranslation(b->annotation.actual_type));
}

static void Literal(AST_Node *node) {
  // TODO: Verify ranges for integers et al
  node->annotation.actual_type = (ActualType)node->annotation.ostensible_type;
}

static void UnaryOp(AST_Node *node) {
  AST_Node *check_node = node->nodes[LEFT];
  if (node->token.type == LOGICAL_NOT) {
    VerifyTypeIs(ACT_BOOL, check_node);
    node->annotation = node->nodes[LEFT]->annotation;
    node->annotation.actual_type = ACT_BOOL;
    return;
  }

  if (node->token.type == MINUS) {
    if (TypeIs(ACT_INT, check_node) || TypeIs(ACT_FLOAT, check_node)) {
      node->annotation = node->nodes[LEFT]->annotation;
      node->annotation.actual_type = ACT_INT;
      return;
    }

    ERROR_AT_TOKEN(node->token,
                   "Type disagreement: expected INT or FLOAT, got '%s'",
                   ActualTypeTranslation(check_node->annotation.actual_type));
  }
}

static void BinaryOp(AST_Node *node) {
  CheckTypeDisagreement(node->nodes[LEFT],
                        node->nodes[RIGHT]);

  node->annotation = node->nodes[LEFT]->annotation;
  node->annotation.actual_type =
    (ActualType)node->annotation.ostensible_type;
}

void CheckTypes(AST_Node *node) {
  if (node->nodes[LEFT]   != NULL) CheckTypes(node->nodes[LEFT]);
  if (node->nodes[MIDDLE] != NULL) CheckTypes(node->nodes[MIDDLE]);
  if (node->nodes[RIGHT]  != NULL) CheckTypes(node->nodes[RIGHT]);

  switch(node->type) {
    case LITERAL_NODE: {
      Literal(node);
    } break;
    case UNARY_OP_NODE: {
      UnaryOp(node);
    } break;
    case BINARY_OP_NODE: {
      BinaryOp(node);
    } break;
    default: {
    } break;
  }

  PrintNode(node);
}
