#include <errno.h>
#include <float.h>    // for FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
#include <inttypes.h> // for INTX_MIN and INTX_MAX
#include <stdlib.h>   // for strtol and friends

#include "common.h"
#include "error.h"
#include "parser_annotation.h"
#include "type_checker.h"

#include <stdio.h>

static SymbolTable *SYMBOL_TABLE;
static ErrorCode error_code;

bool TypeIsConvertible(AST_Node *from, AST_Node *target_type);
ParserAnnotation ShrinkToSmallestContainingType(AST_Node *node);
static void CheckTypesRecurse(AST_Node *node);

/* === HELPERS === */
static int BitWidth(AST_Node *node) {
  return node->annotation.bit_width;
}

static bool IsSigned(AST_Node *node) {
  return node->annotation.is_signed;
}

static int GetBase(AST_Node* node) {
  const int BASE_DECIMAL = 10;
  const int BASE_HEX = 16;
  const int BASE_BINARY = 2;

  return (node->token.type == HEX_LITERAL)
           ? BASE_HEX
           : (node->token.type == BINARY_LITERAL)
               ? BASE_BINARY
               : BASE_DECIMAL;
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
                 "VerifyTypeIs(): Type disagreement, expected type '%s', got type '%s'",
                 ActualTypeTranslation(type),
                 ActualTypeTranslation(node->annotation.actual_type));
  SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
}

void ActualizeType(AST_Node *node, ParserAnnotation a) {
  int preserve_dol = node->annotation.declared_on_line;
  node->annotation = a;
  node->annotation.actual_type = (ActualType)a.ostensible_type;
  node->annotation.declared_on_line = preserve_dol;
}

void ShrinkAndActualizeType(AST_Node *node) {
  ActualizeType(node, ShrinkToSmallestContainingType(node));

  if (node->type == ENUM_LIST_ENTRY_NODE)
  {
    Symbol s = RetrieveFrom(SYMBOL_TABLE, node->token);
    int preserve_dol = s.annotation.declared_on_line;
    s.annotation = node->annotation;
    s.annotation.declared_on_line = preserve_dol;
    AddTo(SYMBOL_TABLE, s);
  }
}

bool Overflow(AST_Node *from, AST_Node *target_type) {
  SetErrorCodeIfUnset(&error_code, ERR_OVERFLOW);

  // Actualize the ostensible type for the error message
  ActualizeType(target_type, target_type->annotation);
  ERROR_AT_TOKEN(from->token,
                 "Literal value overflows target type '%s'\n",
                 AnnotationTranslation(target_type->annotation));
  return false;
}

bool CanConvertToInt(AST_Node *from, AST_Node *target_type) {
  const int from_base = GetBase(from);

  if (Int64Overflow(from->token, from_base)) {
    return Overflow(from, target_type);
  }

  int64_t from_value = TokenToInt64(from->token, from_base);

  switch(BitWidth(target_type)) {
    case 8: {
      if (from_value >= INT8_MIN && from_value <= INT8_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 16: {
      if (from_value >= INT16_MIN && from_value <= INT16_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 32: {
      if (from_value >= INT32_MIN && from_value <= INT32_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 64: {
      if (from_value >= INT64_MIN && from_value <= INT64_MAX) return true;
      return Overflow(from, target_type);
    } break;
    default:
      ERROR_AND_EXIT_FMTMSG("CanConverToInt(): Unknown bit width: %d\n", BitWidth(target_type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
      return false;
  }
}

bool CanConvertToUint(AST_Node *from, AST_Node *target_type) {
  const int from_base = GetBase(from);

  if (Uint64Overflow(from->token, from_base)) {
    return Overflow(from, target_type);
  }

  if (IsSigned(from)) {
    int64_t check_negative = TokenToInt64(from->token, from_base);
    if (check_negative < 0) return Overflow(from, target_type);
  }

  uint64_t from_value = TokenToUint64(from->token, from_base);

  switch(BitWidth(target_type)) {
    case 8: {
      if (from_value <= UINT8_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 16: {
      if (from_value <= UINT16_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 32: {
      if (from_value <= UINT32_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 64: {
      if (from_value <= UINT64_MAX) return true;
      return Overflow(from, target_type);
    } break;
    default:
      ERROR_AND_EXIT_FMTMSG("CanConvertToUint(): Unknown bit width: %d\n", BitWidth(target_type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
      return false;
  }
}

bool CanConvertToFloat(AST_Node *from, AST_Node *target_type) {
  if (DoubleOverflow(from->token)) {
    return Overflow(from, target_type);
  }

  double from_value = TokenToDouble(from->token);

  switch(BitWidth(target_type)) {
    case 32: {
      if (from_value >= -FLT_MAX && from_value <= FLT_MAX) return true;
      return Overflow(from, target_type);
    } break;
    case 64: {
      if (from_value >= -DBL_MAX && from_value <= DBL_MAX) return true;
      return Overflow(from, target_type);
    } break;
    default:
      ERROR_AND_EXIT_FMTMSG("CanConvertToFloat(): Unknown bit width: %d\n", BitWidth(target_type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
      return false;
  }
}

bool TypeIsConvertible(AST_Node *from, AST_Node *target_type) {
  bool types_match = NodeOstensibleType(from) == NodeOstensibleType(target_type);
  bool types_are_not_numbers = !(HasNumberOstType(from) && HasNumberOstType(target_type));

  if (!types_match && types_are_not_numbers) return false;
  if (types_match && types_are_not_numbers) return true;

  if (from->type == IDENTIFIER_NODE) {
    if (types_match == true &&
        BitWidth(from) == BitWidth(target_type) &&
        IsSigned(from) == IsSigned(target_type)) {
      return true;
    }

    return false;
  }

  if (NodeOstensibleType(target_type) == OST_FLOAT) {
    return CanConvertToFloat(from, target_type);
  }

  if (!IsSigned(target_type)) {
    return CanConvertToUint(from, target_type);
  }

  if (IsSigned(target_type)) {
    return CanConvertToInt(from, target_type);
  }

  return false;
}

ParserAnnotation ShrinkToSmallestContainingType(AST_Node *node) {
  const bool SIGNED = true;
  const bool UNSIGNED = false;
  const int _ = 0;

  const int base = GetBase(node);

  if (NodeOstensibleType(node) == OST_INT) {
    if (IsSigned(node)) {
      if (Int64Overflow(node->token, base)) {
        SetErrorCodeIfUnset(&error_code, ERR_OVERFLOW);
        return NoAnnotation();
      }

      int64_t value = TokenToInt64(node->token, base);

      if (value >= INT8_MIN  && value <= INT8_MAX)  return Annotation(OST_INT,  8, SIGNED);
      if (value >= INT16_MIN && value <= INT16_MAX) return Annotation(OST_INT, 16, SIGNED);
      if (value >= INT32_MIN && value <= INT32_MAX) return Annotation(OST_INT, 32, SIGNED);
      if (value >= INT64_MIN && value <= INT64_MAX) return Annotation(OST_INT, 64, SIGNED);

    } else {
      if (Uint64Overflow(node->token, base)) {
        SetErrorCodeIfUnset(&error_code, ERR_OVERFLOW);
        return NoAnnotation();
      }

      uint64_t value = TokenToUint64(node->token, base);

      if (value <= UINT8_MAX)  return Annotation(OST_INT,  8, UNSIGNED);
      if (value <= UINT16_MAX) return Annotation(OST_INT, 16, UNSIGNED);
      if (value <= UINT32_MAX) return Annotation(OST_INT, 32, UNSIGNED);
      if (value <= UINT64_MAX) return Annotation(OST_INT, 64, UNSIGNED);
    }
  }

  if (NodeOstensibleType(node) == OST_FLOAT) {
    if (DoubleOverflow(node->token)) {
      SetErrorCodeIfUnset(&error_code, ERR_OVERFLOW);
      return NoAnnotation();
    }

    if (DoubleUnderflow(node->token)) {
      SetErrorCodeIfUnset(&error_code, ERR_UNDERFLOW);
      return NoAnnotation();
    }
    double d = TokenToDouble(node->token);

    // FLT_MIN and DBL_MIN are the minimum POSITIVE values a float/double
    // can have, so I'm comparing to -MAX instead.
    if (d >= -FLT_MAX && d <= FLT_MAX) return Annotation(OST_FLOAT, 32, SIGNED);
    if (d >= -DBL_MAX && d <= DBL_MAX) return Annotation(OST_FLOAT, 64, SIGNED);
  }

  if (NodeOstensibleType(node) == OST_BOOL) {
    return Annotation(OST_BOOL, 8, _);
  }

  if (NodeOstensibleType(node) == OST_CHAR) {
    return Annotation(OST_CHAR, 8, UNSIGNED);
  }

  if (NodeOstensibleType(node) == OST_STRING) {
    return Annotation(OST_STRING, _, _);
  }

  ERROR_AND_EXIT_FMTMSG("ShrinkToSmallestContainingType(): Type %d not implemented yet", NodeOstensibleType(node));
  SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
  return Annotation(OST_UNKNOWN, _, _);
}
/* === END HELPERS === */

static void String(AST_Node *str) {
  ParserAnnotation a = str->annotation;

  a.is_array = true;
  a.array_size = str->token.length;
  ActualizeType(str, a);
}

static void Declaration(AST_Node *identifier) {
  ActualizeType(identifier, identifier->annotation);
}

static void Assignment(AST_Node *identifier) {
  if (!identifier->annotation.is_array && MIDDLE_NODE(identifier) != NULL) {
    ERROR_AT_TOKEN(
      identifier->token,
      "Assignment(): '%.*s' is not an array\n",
      identifier->token.length,
      identifier->token.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
  }

  AST_Node *value = LEFT_NODE(identifier);
  if ((identifier->type == ENUM_ASSIGNMENT_NODE) &&
      ((NodeOstensibleType(value) != OST_INT) ||
       (value->type == IDENTIFIER_NODE))) {
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
    ERROR_AT_TOKEN(value->token,
                   "Assignment(): Assignment to enum identifier must be of type INT", "");
  }

  if ((NodeOstensibleType(identifier) != OST_INT || IsSigned(identifier)) &&
      (value->token.type == HEX_LITERAL || value->token.type == BINARY_LITERAL)) {
    SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
    ERROR_AT_TOKEN(value->token,
      "Assignment(): '%s' cannot be assigned to non-Uint types",
      TokenTypeTranslation(value->token.type));
  }

  if (identifier->type == TERSE_ASSIGNMENT_NODE) {
    // Treat terse assignment node actual type as the identifier's type
    ActualizeType(identifier, value->annotation);
  }

  bool types_are_compatible = TypeIsConvertible(value, identifier);

  if (types_are_compatible) {
    if (NodeOstensibleType(identifier) == OST_STRING) {
      // For strings, propagate the type information from child node
      // to parent in order to get the length of the string
      ActualizeType(identifier, value->annotation);
    } else {
      // Otherwise actualize the ostensible declared type of the identifier
      ActualizeType(identifier, identifier->annotation);
    }

    // Synchronize information between nodes
    bool assignment_to_array_slot = (identifier->annotation.is_array && !value->annotation.is_array);
    ActualizeType(value, identifier->annotation);
    if (assignment_to_array_slot) {
      value->annotation.is_array = false;
    }

    if (value->type == IDENTIFIER_NODE) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE, value->token);
      identifier->value = s.value;
    }

    return;
  }

  if (NodeActualType(value) == ACT_ENUM) {
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
    ERROR_AT_TOKEN(identifier->token,
                   "Assignment(): Can't use Enum name as an identifier", "");
    return;
  }

  // Actualize the ostensible type for the error message
  identifier->annotation.actual_type = (ActualType)NodeOstensibleType(identifier);
  ERROR_AT_TOKEN(identifier->token,
    "Assignment(): Identifier '%.*s' has type %s and child node has type %s",
    identifier->token.length,
    identifier->token.position_in_source,
    AnnotationTranslation(identifier->annotation),
    AnnotationTranslation(LEFT_NODE(identifier)->annotation));
  SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
}

static void Identifier(AST_Node *identifier) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE, identifier->token);

  if (MIDDLE_NODE(identifier) != NULL &&
      MIDDLE_NODE(identifier)->type == ARRAY_SUBSCRIPT_NODE &&
      identifier->annotation.ostensible_type == OST_STRING) {
    symbol.annotation.ostensible_type = OST_CHAR;
  }

  ActualizeType(identifier, symbol.annotation);
}

static void Literal(AST_Node *node) {
  // Change annotation to unsigned if literal is longer than INT64_MAX
  if (node->token.type == INT_LITERAL &&
      node->token.length >= 19) { // 19 is the length of INT64_MAX
    if (Uint64Overflow(node->token, 10)) {
      SetErrorCodeIfUnset(&error_code, ERR_OVERFLOW);
      return;
    }

    if (TokenToUint64(node->token, 10) > (uint64_t)INT64_MAX) {
      node->annotation.is_signed = false;
    }
  }

  if (node->token.type == HEX_LITERAL ||
      node->token.type == BINARY_LITERAL) {
      ShrinkAndActualizeType(node);
      node->value = NewValue(node->annotation, node->token);
    return;
  }

  switch (NodeOstensibleType(node)) {
    case OST_INT:
    case OST_FLOAT:
    case OST_BOOL:
    case OST_CHAR:
    {
      ShrinkAndActualizeType(node);
      node->value = NewValue(node->annotation, node->token);
      return;
    } break;

    case OST_STRING: {
      String(node);
      node->value = NewValue(node->annotation, node->token);
      return;
    } break;

    default: {
      ERROR_AND_EXIT_FMTMSG("Literal(): Case '%s' not implemented yet",
                            OstensibleTypeTranslation(node->annotation.ostensible_type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
    } break;
  }
}

static void IncrementOrDecrement(AST_Node *node) {
  (node->type == PREFIX_INCREMENT_NODE ||
   node->type == PREFIX_DECREMENT_NODE)
  ? ActualizeType(node, LEFT_NODE(node)->annotation)
  : ActualizeType(node, node->annotation);
}

static void Return(AST_Node* node) {
  if (NodeOstensibleType(node) == OST_VOID) {
    node->annotation.actual_type = ACT_VOID;
    return;
  }

  ActualizeType(node, LEFT_NODE(node)->annotation);
}

static bool IsDeadEnd(AST_Node *node) {
  return (node == NULL) ||
         (node->type == CHAIN_NODE    &&
          LEFT_NODE(node)   == NULL &&
          MIDDLE_NODE(node) == NULL &&
          RIGHT_NODE(node)  == NULL);
}

static void TypeCheckNestedReturns(AST_Node *node, AST_Node *return_type) {
  AST_Node **current = &node;

  do {
    if (LEFT_NODE(*current) != NULL) {
      switch(LEFT_NODE(*current)->type) {
        case IF_NODE:
        case WHILE_NODE:
        case FOR_NODE: {
          TypeCheckNestedReturns(LEFT_NODE(*current), return_type);
        } break;
        default: break;
      }
    }

    if (MIDDLE_NODE(*current) != NULL) {
      switch(MIDDLE_NODE(*current)->type) {
        case IF_NODE:
        case WHILE_NODE:
        case FOR_NODE:
        case CHAIN_NODE: {
          TypeCheckNestedReturns(MIDDLE_NODE(*current), return_type);
        } break;
        default: break;
      }
    }

    if (RIGHT_NODE(*current) != NULL) {
      switch(RIGHT_NODE(*current)->type) {
        case IF_NODE:
        case WHILE_NODE:
        case FOR_NODE: {
          TypeCheckNestedReturns(RIGHT_NODE(*current), return_type);
        } break;
        default: break;
      }
    }

    if (LEFT_NODE(*current)->type == RETURN_NODE) {
      if (!TypeIsConvertible(LEFT_NODE(*current)->nodes[LEFT], return_type)) {
        ERROR_AT_TOKEN(
          LEFT_NODE(*current)->nodes[LEFT]->token,
          "TypeCheckNestedReturns(): Can't convert type from %s to %s",
          AnnotationTranslation((*current)->nodes[LEFT]->annotation),
          AnnotationTranslation(return_type->annotation)
        );
        SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
      }
    }

    current = &RIGHT_NODE(*current);

  } while(!IsDeadEnd(*current));
}

static void Function(AST_Node *node) {
  AST_Node *return_type = LEFT_NODE(node);
  AST_Node *body = RIGHT_NODE(node);
  AST_Node **check = &body;

  do {
    if (LEFT_NODE(*check)->type == IF_NODE ||
        LEFT_NODE(*check)->type == WHILE_NODE ||
        LEFT_NODE(*check)->type == FOR_NODE) {
      TypeCheckNestedReturns(LEFT_NODE(*check), return_type);
    }

    if (LEFT_NODE(*check)->type == RETURN_NODE) {
      if (TypeIsConvertible(LEFT_NODE(*check), return_type)) {
        ActualizeType(node, node->annotation);

        if (!IsDeadEnd(RIGHT_NODE(*check))) {
          ERROR_AT_TOKEN(
            RIGHT_NODE(*check)->nodes[LEFT]->token,
            "Function(): Unreachable code in function %.*s",
            node->token.length,
            node->token.position_in_source);
          SetErrorCodeIfUnset(&error_code, ERR_UNREACHABLE_CODE);
        }

        return;
      } else if (NodeActualType(LEFT_NODE(*check)) == ACT_VOID) {
        /* Do nothing
         *
         * This case occurs when a non-void function has no return in the body.
         * The parser inserts a void return node into the body and this function
         * segfaults without this check. The 'missing return' error will
         * trigger appropriately after the do-while loop finishes. */
      } else {
        ERROR_AT_TOKEN(
          LEFT_NODE(*check)->nodes[LEFT]->token,
          "Function(): in function '%.*s': Return type '%s' is not convertible to '%s'",
          node->token.length,
          node->token.position_in_source,
          AnnotationTranslation((*check)->nodes[LEFT]->annotation),
          AnnotationTranslation(return_type->annotation));
        SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
      }
    }

    check = &RIGHT_NODE(*check);
  } while (!IsDeadEnd(*check));

  if (NodeActualType(return_type) == ACT_VOID) {
    node->annotation.actual_type = ACT_VOID;
  } else {
    ERROR_AT_TOKEN(
      node->token,
      "Function(): Missing return statement in non-void function '%.*s'",
      node->token.length,
      node->token.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
  }
}

static void FunctionCall(AST_Node *node) {
  AST_Node **current = &MIDDLE_NODE(node);
  Symbol fn_definition = RetrieveFrom(SYMBOL_TABLE, node->token);

  for (int i = 0; i < fn_definition.fn_param_count; i++) {
    AST_Node *argument = *current;
    if (argument == NULL) {
      ERROR_AT_TOKEN(
        (*current)->token,
        "%.*s(): Missing '%.*s' argument\n",
        node->token.length,
        node->token.position_in_source,
        fn_definition.fn_param_list[i].param_token.length,
        fn_definition.fn_param_list[i].param_token.position_in_source
      );
      SetErrorCodeIfUnset(&error_code, ERR_TOO_FEW);
    }

    // TypeIsConvertible deals with AST_Nodes but fn_params aren't an AST_Node
    AST_Node *wrapped_param = NewNodeFromToken(UNTYPED, NULL, NULL, NULL,
      fn_definition.fn_param_list[i].param_token,
      fn_definition.fn_param_list[i].annotation
    );

    if (!TypeIsConvertible(argument, wrapped_param)) {
      ERROR_AT_TOKEN(
        argument->token,
        "%.*s(): Can't convert type from %s to %s\n",
        node->token.length,
        node->token.position_in_source,
        OstensibleTypeTranslation(argument->annotation.ostensible_type),
        OstensibleTypeTranslation(wrapped_param->annotation.ostensible_type));
      SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
    }

    current = &RIGHT_NODE(*current);
  }

  if ((*current) != NULL) {
    ERROR_AT_TOKEN(
      (*current)->token,
      "%.*s(): Too many arguments",
      node->token.length,
      node->token.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_TOO_MANY);
  }

  ActualizeType(node, node->annotation);
}

static void UnaryOp(AST_Node *node) {
  AST_Node *check_node = LEFT_NODE(node);

  if (node->token.type == LOGICAL_NOT) {
    VerifyTypeIs(ACT_BOOL, check_node);
    node->annotation = LEFT_NODE(node)->annotation;
    node->annotation.actual_type = ACT_BOOL;
    return;
  }

  if (node->token.type == BITWISE_NOT) {
    VerifyTypeIs(ACT_INT, check_node);
    if (check_node->annotation.is_signed) {
      SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
      ERROR_AT_TOKEN(check_node->token,
                     "Operand must be of type Uint", "");
    }
    node->annotation = LEFT_NODE(node)->annotation;
    node->annotation.actual_type = ACT_INT;
    return;
  }

  if (node->token.type == MINUS) {
    if (check_node->token.type == HEX_LITERAL ||
        check_node->token.type == BINARY_LITERAL) {
      SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
      ERROR_AT_TOKEN(
        check_node->token,
        "'%s' not allowed with unary '-'",
        TokenTypeTranslation(check_node->token.type));
      return;
    }

    if (NodeActualType(check_node) == ACT_INT) {
      node->annotation = LEFT_NODE(node)->annotation;
      node->annotation.actual_type = ACT_INT;
      node->annotation.is_signed = !node->annotation.is_signed;

      node->value = NewIntValue(-LEFT_NODE(node)->value.as.integer);
      return;
    }

    if (NodeActualType(check_node) == ACT_FLOAT) {
      node->annotation = LEFT_NODE(node)->annotation;
      node->annotation.actual_type = ACT_FLOAT;
      node->annotation.is_signed = !node->annotation.is_signed;

      node->value = NewIntValue(-LEFT_NODE(node)->value.as.floating);
      return;
    }

    SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
    ERROR_AT_TOKEN(node->token,
                   "UnaryOp(): Type disagreement: expected INT or FLOAT, got '%s'",
                   ActualTypeTranslation(check_node->annotation.actual_type));
  }
}

static void BinaryArithmeticOp(AST_Node *node) {
  node->annotation = LEFT_NODE(node)->annotation;
  node->annotation.actual_type = (ActualType)NodeOstensibleType(node);

  if (!TypeIsConvertible(RIGHT_NODE(node), node)) {
    ERROR_AT_TOKEN(
      RIGHT_NODE(node)->token,
      "BinaryArithmeticOp(): Cannot convert from type '%s' to '%s'\n",
      AnnotationTranslation(RIGHT_NODE(node)->annotation),
      AnnotationTranslation(node->annotation));
    SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
  }

  ActualizeType(RIGHT_NODE(node), node->annotation);
}

static void BinaryLogicalOp(AST_Node *node) {
  switch(node->token.type) {
    case LESS_THAN:
    case GREATER_THAN:
    case LESS_THAN_EQUALS:
    case GREATER_THAN_EQUALS: {
      // Check left node individually for incorrect type
      if (NodeOstensibleType(LEFT_NODE(node)) != OST_INT &&
          NodeOstensibleType(LEFT_NODE(node)) != OST_FLOAT) {
        SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
        ERROR_AT_TOKEN(LEFT_NODE(node)->token,
                       "BinaryLogicalOP(): Invalid operand type '%s', expected Bool",
                       OstensibleTypeTranslation(LEFT_NODE(node)->annotation.ostensible_type));
        return;
      }

      // Check right node individually for incorrect type
      if (NodeOstensibleType(RIGHT_NODE(node)) != OST_INT &&
          NodeOstensibleType(RIGHT_NODE(node)) != OST_FLOAT) {
        SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
        ERROR_AT_TOKEN(RIGHT_NODE(node)->token,
                       "BinaryLogicalOP(): Invalid operand type '%s', expected Bool",
                       OstensibleTypeTranslation(RIGHT_NODE(node)->annotation.ostensible_type));
        return;
      }
    } /* Intentional fallthrough */
    case LOGICAL_NOT_EQUALS:
    case EQUALITY: {
      // Ensure both node types match
      if (NodeOstensibleType(LEFT_NODE(node)) !=
          NodeOstensibleType(RIGHT_NODE(node))) {
        SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
        ERROR_AT_TOKEN(RIGHT_NODE(node)->token,
                       "BinaryLogicalOP(): Operand types don't match", "");
        return;
      }

      // If left node is unsigned, check if right node can be converted
      if (IsSigned(LEFT_NODE(node)) && !IsSigned(RIGHT_NODE(node))) {
        if (TypeIsConvertible(RIGHT_NODE(node), LEFT_NODE(node))) {
          ActualizeType(RIGHT_NODE(node), LEFT_NODE(node)->annotation);
        } else {
          SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
          ERROR_AT_TOKEN(RIGHT_NODE(node)->token,
                         "BinaryLogicalOP(): Type '%s' is not convertible to other operand type '%s'",
                         OstensibleTypeTranslation(RIGHT_NODE(node)->annotation.ostensible_type),
                         OstensibleTypeTranslation(LEFT_NODE(node)->annotation.ostensible_type));
          return;
        }
      }

      // If right node is unsigned, check if left node can be converted
      if (!IsSigned(LEFT_NODE(node)) && IsSigned(RIGHT_NODE(node))) {
        if (TypeIsConvertible(LEFT_NODE(node), RIGHT_NODE(node))) {
          ActualizeType(LEFT_NODE(node), RIGHT_NODE(node)->annotation);
        } else {
          SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
          ERROR_AT_TOKEN(LEFT_NODE(node)->token,
                         "BinaryLogicalOP(): Type '%s' is not convertible to other operand type '%s'",
                         OstensibleTypeTranslation(LEFT_NODE(node)->annotation.ostensible_type),
                         OstensibleTypeTranslation(RIGHT_NODE(node)->annotation.ostensible_type));
          return;
        }
      }

      ActualizeType(node, node->annotation);
    } break;

    case LOGICAL_AND:
    case LOGICAL_OR: {
      if (NodeOstensibleType(LEFT_NODE(node)) != OST_BOOL) {
        SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
        ERROR_AT_TOKEN(LEFT_NODE(node)->token,
                       "BinaryLogicalOP(): Invalid operand type '%s', expected Bool",
                       OstensibleTypeTranslation(LEFT_NODE(node)->annotation.ostensible_type));
        return;
      }

      if (NodeOstensibleType(RIGHT_NODE(node)) != OST_BOOL) {
        SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
        ERROR_AT_TOKEN(RIGHT_NODE(node)->token,
                       "BinaryLogicalOP(): Invalid operand type '%s', expected Bool",
                       OstensibleTypeTranslation(RIGHT_NODE(node)->annotation.ostensible_type));
        return;
      }

      ActualizeType(node, node->annotation);
    } break;

    default: {
      printf("BinaryLogicalOp(): Not implemented yet\n");
    } break;
  }
}

static void BinaryBitwiseOp(AST_Node *node) {
  AST_Node *left_value = LEFT_NODE(node);
  AST_Node *right_value = RIGHT_NODE(node);

  // Check left node individually for incorrect type
  if (NodeOstensibleType(left_value) != OST_INT || IsSigned(left_value)) {
    SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
    ERROR_AT_TOKEN(left_value->token,
                   "BinaryBitwiseOP(): Invalid operand type '%s', expected Uint",
                   OstensibleTypeTranslation(left_value->annotation.ostensible_type));
    return;
  }

  // Check right node individually for incorrect type
  if (NodeOstensibleType(right_value) != OST_INT || IsSigned(right_value)) {
    SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
    ERROR_AT_TOKEN(right_value->token,
                   "BinaryBitwiseOP(): Invalid operand type '%s', expected Bool",
                   OstensibleTypeTranslation(right_value->annotation.ostensible_type));
    return;
  }

  ActualizeType(node, node->annotation);
}

static void InitializerList(AST_Node *node) {
  AST_Node **current = &node;

  int num_literals_in_list = 0;
  do {
    if (!TypeIsConvertible(LEFT_NODE(*current), node)) {
      // Actualize the ostensible type for the error message
      ActualizeType(node, node->annotation);
      ERROR_AT_TOKEN(
        LEFT_NODE(*current)->token,
        "InitializerList(): Cannot convert from type '%s' to '%s'\n",
        AnnotationTranslation(LEFT_NODE(*current)->annotation),
        AnnotationTranslation(node->annotation));
      SetErrorCodeIfUnset(&error_code, ERR_TYPE_DISAGREEMENT);
    }

    num_literals_in_list++;
    if (num_literals_in_list > node->annotation.array_size) {
      ERROR_AT_TOKEN(
        LEFT_NODE(*current)->token,
        "InitializerList(): Too many elements in initializer list (array size is %d)",
        node->annotation.array_size);
      SetErrorCodeIfUnset(&error_code, ERR_TOO_MANY);
    }

    current = &RIGHT_NODE(*current);
  } while (*current != NULL && LEFT_NODE(*current) != NULL);

  ActualizeType(node, LEFT_NODE(node)->annotation);
}

static void EnumListRecurse(AST_Node *node, int implicit_value) {
  AST_Node *list_entry = LEFT_NODE(node);

  if (list_entry == NULL) return;

  if (list_entry->type == ENUM_ASSIGNMENT_NODE) {
    CheckTypesRecurse(list_entry);
    AST_Node *value = LEFT_NODE(list_entry);
    if ((NodeOstensibleType(value) != OST_INT) ||
        (value->type == IDENTIFIER_NODE)) {
      SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
      ERROR_AT_TOKEN(value->token,
                     "Assignment(): Assignment to enum identifier must be of type INT", "");
    }

    ShrinkAndActualizeType(list_entry);
    SetValue(SYMBOL_TABLE, list_entry->token, LEFT_NODE(list_entry)->value);

    implicit_value = (LEFT_NODE(list_entry)->value.as.integer + 1);
  }

  if (list_entry->type == ENUM_LIST_ENTRY_NODE) {
    list_entry->value = NewIntValue(implicit_value);
    ShrinkAndActualizeType(list_entry);
    SetValue(SYMBOL_TABLE, list_entry->token, list_entry->value);

    implicit_value++;
  }

  EnumListRecurse(RIGHT_NODE(node), implicit_value);
}

static void HandleEnum(AST_Node *node) {
  EnumListRecurse(node, 0);
  ActualizeType(node, node->annotation);
}

static void CheckTypesRecurse(AST_Node *node) {
  SymbolTable *remember_st = SYMBOL_TABLE;
  if (node->type == FUNCTION_NODE) {
    Symbol s = RetrieveFrom(SYMBOL_TABLE, node->token);
    SYMBOL_TABLE = s.fn_params;
  }

  if (node->type == ENUM_IDENTIFIER_NODE) {
    HandleEnum(node);
    return;
  }

  if (LEFT_NODE(node)   != NULL) CheckTypesRecurse(LEFT_NODE(node));
  if (MIDDLE_NODE(node) != NULL) CheckTypesRecurse(MIDDLE_NODE(node));
  if (RIGHT_NODE(node)  != NULL) CheckTypesRecurse(RIGHT_NODE(node));

  if (node->type == FUNCTION_NODE) {
    SYMBOL_TABLE = remember_st;
  }

  switch(node->type) {
    case LITERAL_NODE: {
      Literal(node);
    } break;
    case UNARY_OP_NODE: {
      UnaryOp(node);
    } break;
    case BINARY_ARITHMETIC_NODE: {
      BinaryArithmeticOp(node);
    } break;
    case BINARY_LOGICAL_NODE: {
      BinaryLogicalOp(node);
    } break;
    case BINARY_BITWISE_NODE: {
      BinaryBitwiseOp(node);
    } break;
    case IDENTIFIER_NODE: {
      Identifier(node);
    } break;
    case ARRAY_SUBSCRIPT_NODE: {
      ShrinkAndActualizeType(node);
    } break;
    case DECLARATION_NODE: {
      Declaration(node);
    } break;
    case TERSE_ASSIGNMENT_NODE:
    case ASSIGNMENT_NODE: {
      Assignment(node);
    } break;
    case PREFIX_INCREMENT_NODE:
    case PREFIX_DECREMENT_NODE:
    case POSTFIX_INCREMENT_NODE:
    case POSTFIX_DECREMENT_NODE: {
      IncrementOrDecrement(node);
    } break;
    case FUNCTION_ARGUMENT_NODE:
    case FUNCTION_PARAM_NODE:
    case FUNCTION_RETURN_TYPE_NODE: {
      ActualizeType(node, node->annotation);
    } break;
    case FUNCTION_NODE: {
      Function(node);
    } break;
    case FUNCTION_CALL_NODE: {
      FunctionCall(node);
    } break;
    case RETURN_NODE: {
      Return(node);
    } break;
    case STRUCT_FIELD_IDENTIFIER_NODE: {
      ActualizeType(node, node->annotation);
    } break;
    case ARRAY_INITIALIZER_LIST_NODE: {
      InitializerList(node);
    } break;
    default: {
    } break;
  }

  if (node->type != START_NODE &&
      node->type != CHAIN_NODE) {
    //PrintNode(node);
  }
}

void CheckTypes(AST_Node *node, SymbolTable *symbol_table) {
  UnsetErrorCode(&error_code);

  SYMBOL_TABLE = symbol_table;
  CheckTypesRecurse(node);

  if (node->error_code == ERR_UNSET) {
    SetErrorCodeIfUnset(&node->error_code, error_code);
  }
}
