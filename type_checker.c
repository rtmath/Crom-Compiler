#include <errno.h>
#include <float.h>    // for FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
#include <inttypes.h> // for INTX_MIN and INTX_MAX
#include <stdio.h>
#include <stdlib.h>   // for strtol and friends

#include "error.h"
#include "parser_annotation.h"
#include "type_checker.h"

/* === HELPERS === */
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

long long StringToLL(const char *s) {
  errno = 0;
  long long value = strtoll(s, NULL, 10);
  if (errno != 0) {
    ERROR_AND_EXIT("StringToLL() error");
  }

  return value;
}

unsigned long long StringToULL(const char *s) {
  errno = 0;
  unsigned long long value = strtoull(s, NULL, 10);
  if (errno != 0) {
    ERROR_AND_EXIT("StringToLL() error");
  }

  return value;
}

double StringToDouble(const char *s) {
  errno = 0;
  long long value = strtoull(s, NULL, 10);
  if (errno != 0) {
    ERROR_AND_EXIT("StringToLL() error");
  }

  return value;
}

bool TypesAreConvertible(AST_Node *from, AST_Node *to) {
  if (from->annotation.ostensible_type != to->annotation.ostensible_type) return false;

  if (from->annotation.is_signed && !to->annotation.is_signed) {
    long long from_value = StringToLL(from->token.position_in_source);

    if (from_value < 0) return false;
    switch(to->annotation.bit_width) {
      case  8: return from_value < UINT8_MAX;
      case 16: return from_value < UINT16_MAX;
      case 32: return from_value < UINT32_MAX;
      case 64: return true;
      default:
        ERROR_AND_EXIT_FMTMSG("Unknown bit width: %d\n", to->annotation.bit_width);
        return false;
    }
  }

  if (!from->annotation.is_signed && to->annotation.is_signed) {
    unsigned long long from_value = StringToULL(from->token.position_in_source);

    switch(to->annotation.bit_width) {
      case  8: return from_value < INT8_MAX;
      case 16: return from_value < INT16_MAX;
      case 32: return from_value < INT32_MAX;
      case 64: return from_value < INT64_MAX;
      default:
        ERROR_AND_EXIT_FMTMSG("Unknown bit width: %d\n", to->annotation.bit_width);
        return false;
    }
  }

  return false;
}

/* === END HELPERS === */
/* === RANGE VALIDATION === */
ParserAnnotation ShrinkToSmallestContainingType(AST_Node *node) {
  const bool SIGNED = true;
  const bool UNSIGNED = false;

  if (node->annotation.ostensible_type == OST_INT) {
    if (node->annotation.is_signed) {
      long long value = StringToLL(node->token.position_in_source);

      if (value >= INT8_MIN  && value <= INT8_MAX)  return Annotation(OST_INT,  8, SIGNED);
      if (value >= INT16_MIN && value <= INT16_MAX) return Annotation(OST_INT, 16, SIGNED);
      if (value >= INT32_MIN && value <= INT32_MAX) return Annotation(OST_INT, 32, SIGNED);
      if (value >= INT64_MIN && value <= INT64_MAX) return Annotation(OST_INT, 64, SIGNED);

    } else {
      unsigned long long value = StringToULL(node->token.position_in_source);

      if (value <= UINT8_MAX)  return Annotation(OST_INT,  8, UNSIGNED);
      if (value <= UINT16_MAX) return Annotation(OST_INT, 16, UNSIGNED);
      if (value <= UINT32_MAX) return Annotation(OST_INT, 32, UNSIGNED);
      if (value <= UINT64_MAX) return Annotation(OST_INT, 64, UNSIGNED);
    }
  }

  if (node->annotation.ostensible_type == OST_FLOAT) {
    double d = StringToDouble(node->token.position_in_source);

    if (d >= FLT_MIN && d <= FLT_MAX) return Annotation(OST_FLOAT, 32, SIGNED);
    if (d >= DBL_MIN && d <= DBL_MAX) return Annotation(OST_FLOAT, 64, SIGNED);
  }

  ERROR_AND_EXIT("ShrinkToSmallestContainingType(): Unknown Ostensible type");
  return Annotation(OST_UNKNOWN, 0, 0);
}

bool ValidateIntLiteral(AST_Node *node) {
  if (node->annotation.is_signed) {
    long long value = StringToLL(node->token.position_in_source);

    switch (node->annotation.bit_width) {
      case  8: return (value >= INT8_MIN  && value <= INT8_MAX);
      case 16: return (value >= INT16_MIN && value <= INT16_MAX);
      case 32: return (value >= INT32_MIN && value <= INT32_MAX);
      case 64: return (value >= INT64_MIN && value <= INT64_MAX);
      default:
        ERROR_AND_EXIT_FMTMSG(
            "ValidateIntLiteral(): Invalid bit_width %d\n",
            node->annotation.bit_width);
    }
  } else {
    unsigned long long value = StringToULL(node->token.position_in_source);

    switch (node->annotation.bit_width) {
      case  8: return (value <= UINT8_MAX);
      case 16: return (value <= UINT16_MAX);
      case 32: return (value <= UINT32_MAX);
      case 64: return (value <= UINT64_MAX);
      default:
        ERROR_AND_EXIT_FMTMSG(
          "ValidateIntLiteral(): Invalid bit_width %d\n",
          node->annotation.bit_width);
    }
  }

  return false;
}

bool ValidateFloatLiteral(AST_Node *node) {
  double d = StringToDouble(node->token.position_in_source);

  switch (node->annotation.bit_width) {
    case 32: return (d >= FLT_MIN && d <= FLT_MAX);
    case 64: return (d >= DBL_MIN && d <= DBL_MAX);
    default: {
      ERROR_AND_EXIT_FMTMSG(
        "ValidateFloatLiteral(): Invalid bit_width %d\n",
        node->annotation.bit_width);
    } break;
  }

  return false;
}
/* === END RANGE VALIDATION === */

static void Identifier(AST_Node *node) {
  AST_Node *child = node->nodes[LEFT];
  bool types_match = ((ActualType)node->annotation.ostensible_type ==
                      child->annotation.actual_type);

  if (types_match) {
    if (node->annotation.ostensible_type == OST_INT) {
      if ((node->annotation.is_signed == child->annotation.is_signed &&
           child->annotation.bit_width <= node->annotation.bit_width &&
           child->annotation.bit_width != 0) ||
          TypesAreConvertible(node, child)) {
        node->annotation.actual_type = (ActualType)node->annotation.ostensible_type;
        child->annotation.bit_width = node->annotation.bit_width;
        child->annotation.is_signed = node->annotation.is_signed;
        return;
      }
    }
  }

  // Hack to improve the ERROR_AT_TOKEN error message
  node->annotation.actual_type = (ActualType)node->annotation.ostensible_type;

  ERROR_AT_TOKEN(node->token,
    "Identifier(): Identifier '%.*s' has type %s and child node has type %s",
    node->token.length,
    node->token.position_in_source,
    AnnotationTranslation(node->annotation),
    AnnotationTranslation(node->nodes[LEFT]->annotation));
}

static void Literal(AST_Node *node) {
  switch (node->annotation.ostensible_type) {
    case OST_INT: {
      if (!ValidateIntLiteral(node)) {
        ERROR_AND_EXIT_FMTMSG(
          "Invalid literal '%.*s' for type %s",
          node->token.length,
          node->token.position_in_source,
          OstensibleTypeTranslation(node->annotation.ostensible_type));
        return;
      }

      ParserAnnotation type_anno = ShrinkToSmallestContainingType(node);
      node->annotation.is_signed = type_anno.is_signed;
      node->annotation.bit_width = type_anno.bit_width;
      node->annotation.actual_type = (ActualType)type_anno.ostensible_type;
      return;
    } break;

    default: {
      // TODO
      ERROR_AND_EXIT_FMTMSG("Literal(): Case '%s' not implemented yet",
                            OstensibleTypeTranslation(node->annotation.ostensible_type));
    } break;
  }

  node->annotation.actual_type =
    (ActualType)node->annotation.ostensible_type;
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
    if (TypeIs(ACT_INT, check_node)) {
      node->annotation = node->nodes[LEFT]->annotation;
      node->annotation.actual_type = ACT_INT;
      node->annotation.is_signed = true;
      return;
    }

    if (TypeIs(ACT_FLOAT, check_node)) {
      node->annotation = node->nodes[LEFT]->annotation;
      node->annotation.actual_type = ACT_FLOAT;
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
    case IDENTIFIER_NODE: {
      Identifier(node);
    } break;
    default: {
    } break;
  }

  PrintNode(node);
}
