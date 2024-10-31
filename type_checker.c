#include <errno.h>
#include <float.h>    // for FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
#include <inttypes.h> // for INTX_MIN and INTX_MAX
#include <stdio.h>
#include <stdlib.h>   // for strtol and friends

#include "error.h"
#include "parser_annotation.h"
#include "type_checker.h"

static SymbolTable *SYMBOL_TABLE;

/* === HELPERS === */
static int BitWidth(AST_Node *node) {
  return node->annotation.bit_width;
}

static bool IsSigned(AST_Node *node) {
  return node->annotation.is_signed;
}

static OstensibleType NodeOstensibleType(AST_Node *node) {
  return node->annotation.ostensible_type;
}

static ActualType NodeActualType(AST_Node *node) {
  return node->annotation.actual_type;
}

static bool HasNumberOstType(AST_Node *node) {
  return NodeOstensibleType(node) == OST_INT ||
         NodeOstensibleType(node) == OST_FLOAT;
}

void VerifyTypeIs(ActualType type, AST_Node *node) {
  if (NodeActualType(node) == type) return;

  ERROR_AT_TOKEN(node->token,
                 "Type disagreement, expected type '%s', got type '%s'",
                 ActualTypeTranslation(type),
                 ActualTypeTranslation(node->annotation.actual_type));
}

void CheckTypeDisagreement(AST_Node *a, AST_Node *b) {
  if (NodeActualType(a) == NodeActualType(b)) return;

  ERROR_AT_TOKEN(b->token,
                 "Type disagreement between %s and %s",
                 ActualTypeTranslation(a->annotation.actual_type),
                 ActualTypeTranslation(b->annotation.actual_type));
}

long long TokenToLL(Token t) {
  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, 10);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToLL() underflow or overflow");
  }

  return value;
}

unsigned long long TokenToULL(Token t) {
  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, 10);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToULL() underflow or overflow");
  }

  return value;
}

double TokenToDouble(Token t) {
  errno = 0;
  double value = strtod(t.position_in_source, NULL);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToDouble() underflow or overflow");
  }

  return value;
}

bool TypeIsConvertible(AST_Node *from, AST_Node *target_type) {
  bool types_dont_match = NodeOstensibleType(from) != NodeOstensibleType(target_type);
  bool both_types_arent_numbers = !(HasNumberOstType(from) && HasNumberOstType(target_type));
  if (types_dont_match && both_types_arent_numbers) return false;

  if (IsSigned(from) == IsSigned(target_type) &&
      BitWidth(from) <= BitWidth(target_type) &&
      BitWidth(from) > 0) {
    return true;
  }

  if (from->type == IDENTIFIER_NODE) {
    // If the From node is an Identifier, its token
    // will be the identifier name (not a literal value)
    // so the TokenTo*() functions won't work properly.
    //
    // Ergo, range validation is not performed on identifiers
    // if their type is not obviously convertible.
    //
    // TODO: Store symbol value in table and use it here for range
    // validation
    return false;
  }

  if (NodeOstensibleType(target_type) == OST_FLOAT) {
    if (BitWidth(target_type) == 32) {
      double d = TokenToDouble(from->token);
      return d >= FLT_MIN && d <= FLT_MAX;
    }

    if (BitWidth(target_type) == 64) {
      double d = TokenToDouble(from->token);
      return d >= DBL_MIN && d <= DBL_MAX;
    }

    ERROR_AND_EXIT_FMTMSG("TypeIsConvertible(): Unknown bitwidth '%d'", BitWidth(target_type));
  }

  if (IsSigned(from) && !IsSigned(target_type)) {
    long long from_value = TokenToLL(from->token);

    if (from_value < 0) return false;
    switch(BitWidth(target_type)) {
      case  8: return from_value < UINT8_MAX;
      case 16: return from_value < UINT16_MAX;
      case 32: return from_value < UINT32_MAX;
      case 64: return true;
      default:
        ERROR_AND_EXIT_FMTMSG("Unknown bit width: %d\n", BitWidth(target_type));
        return false;
    }
  }

  if (!IsSigned(from) && IsSigned(target_type)) {
    unsigned long long from_value = TokenToULL(from->token);

    switch(BitWidth(target_type)) {
      case  8: return from_value < INT8_MAX;
      case 16: return from_value < INT16_MAX;
      case 32: return from_value < INT32_MAX;
      case 64: return from_value < INT64_MAX;
      default:
        ERROR_AND_EXIT_FMTMSG("Unknown bit width: %d\n", BitWidth(target_type));
        return false;
    }
  }

  return false;
}

ParserAnnotation ShrinkToSmallestContainingType(AST_Node *node) {
  const bool SIGNED = true;
  const bool UNSIGNED = false;

  if (NodeOstensibleType(node) == OST_INT) {
    if (IsSigned(node)) {
      long long value = TokenToLL(node->token);

      if (value >= INT8_MIN  && value <= INT8_MAX)  return Annotation(OST_INT,  8, SIGNED);
      if (value >= INT16_MIN && value <= INT16_MAX) return Annotation(OST_INT, 16, SIGNED);
      if (value >= INT32_MIN && value <= INT32_MAX) return Annotation(OST_INT, 32, SIGNED);
      if (value >= INT64_MIN && value <= INT64_MAX) return Annotation(OST_INT, 64, SIGNED);

    } else {
      unsigned long long value = TokenToULL(node->token);

      if (value <= UINT8_MAX)  return Annotation(OST_INT,  8, UNSIGNED);
      if (value <= UINT16_MAX) return Annotation(OST_INT, 16, UNSIGNED);
      if (value <= UINT32_MAX) return Annotation(OST_INT, 32, UNSIGNED);
      if (value <= UINT64_MAX) return Annotation(OST_INT, 64, UNSIGNED);
    }
  }

  if (NodeOstensibleType(node) == OST_FLOAT) {
    double d = TokenToDouble(node->token);

    if (d >= FLT_MIN && d <= FLT_MAX) return Annotation(OST_FLOAT, 32, SIGNED);
    if (d >= DBL_MIN && d <= DBL_MAX) return Annotation(OST_FLOAT, 64, SIGNED);
  }

  if (NodeOstensibleType(node) == OST_BOOL) {
    return Annotation(OST_BOOL, 0, 0);
  }

  ERROR_AND_EXIT("ShrinkToSmallestContainingType(): Unknown Ostensible type");
  return Annotation(OST_UNKNOWN, 0, 0);
}
/* === END HELPERS === */

static void Declaration(AST_Node *identifier) {
  identifier->annotation.actual_type = (ActualType)NodeOstensibleType(identifier);
}

static void Assignment(AST_Node *identifier) {
  AST_Node *value = identifier->nodes[LEFT];

  bool types_are_same = NodeOstensibleType(identifier) == NodeOstensibleType(value);
  bool types_are_compatible = types_are_same || TypeIsConvertible(value, identifier);

  if (types_are_compatible) {
    identifier->annotation.actual_type = (ActualType)NodeOstensibleType(identifier);
    value->annotation.actual_type = identifier->annotation.actual_type;
    value->annotation.bit_width = BitWidth(identifier);
    value->annotation.is_signed = IsSigned(identifier);
    return;
  }

  // Actualize the ostensible type for the error message
  identifier->annotation.actual_type = (ActualType)NodeOstensibleType(identifier);
  ERROR_AT_TOKEN(identifier->token,
    "Assignment(): Identifier '%.*s' has type %s and child node has type %s",
    identifier->token.length,
    identifier->token.position_in_source,
    AnnotationTranslation(identifier->annotation),
    AnnotationTranslation(identifier->nodes[LEFT]->annotation));
}

static void Identifier(AST_Node *identifier) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE, identifier->token);
  identifier->annotation.actual_type = (ActualType)symbol.annotation.ostensible_type;
}

static void Literal(AST_Node *node) {
  switch (NodeOstensibleType(node)) {
    case OST_INT:
    case OST_FLOAT:
    case OST_BOOL:
    {
      // Actualize type from its smallest containing ostensible type
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

  node->annotation.actual_type = (ActualType)NodeOstensibleType(node);
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
    if (NodeActualType(check_node) == ACT_INT) {
      node->annotation = node->nodes[LEFT]->annotation;
      node->annotation.actual_type = ACT_INT;
      node->annotation.is_signed = true;
      return;
    }

    if (NodeActualType(check_node) == ACT_FLOAT) {
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
  node->annotation.actual_type = (ActualType)NodeOstensibleType(node);
}

void CheckTypesRecurse(AST_Node *node) {
  if (node->nodes[LEFT]   != NULL) CheckTypesRecurse(node->nodes[LEFT]);
  if (node->nodes[MIDDLE] != NULL) CheckTypesRecurse(node->nodes[MIDDLE]);
  if (node->nodes[RIGHT]  != NULL) CheckTypesRecurse(node->nodes[RIGHT]);

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
    case DECLARATION_NODE: {
      Declaration(node);
    } break;
    case ASSIGNMENT_NODE: {
      Assignment(node);
    } break;
    default: {
    } break;
  }

  if (node->type != CHAIN_NODE &&
      node->type != START_NODE) {
    PrintNode(node);
  }
}

void CheckTypes(AST_Node *node, SymbolTable *symbol_table) {
  SYMBOL_TABLE = symbol_table;
  CheckTypesRecurse(node);
}
