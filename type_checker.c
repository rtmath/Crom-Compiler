#include <errno.h>
#include <float.h>    // for FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
#include <inttypes.h> // for INTX_MIN and INTX_MAX
#include <stdlib.h>   // for strtol and friends

#include "error.h"
#include "parser_annotation.h"
#include "type_checker.h"

#define BASE_DECIMAL 10
#define BASE_HEX 16
#define BASE_BINARY 2

static SymbolTable *SYMBOL_TABLE;

bool TypeIsConvertible(AST_Node *from, AST_Node *target_type);
ParserAnnotation ShrinkToSmallestContainingType(AST_Node *node);

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
                 "VerifyTypeIs(): Type disagreement, expected type '%s', got type '%s'",
                 ActualTypeTranslation(type),
                 ActualTypeTranslation(node->annotation.actual_type));
}

void ActualizeType(AST_Node *node, ParserAnnotation a) {
  int preserve_dol = node->annotation.declared_on_line;
  node->annotation = a;
  node->annotation.actual_type = (ActualType)a.ostensible_type;
  node->annotation.declared_on_line = preserve_dol;
}

void ShrinkAndActualizeType(AST_Node *node) {
  ActualizeType(node, ShrinkToSmallestContainingType(node));
}

long long TokenToLL(Token t, int base) {
  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToLL() underflow or overflow");
  }

  return value;
}

unsigned long long TokenToULL(Token t, int base) {
  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, base);
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
  bool types_match = NodeOstensibleType(from) == NodeOstensibleType(target_type);
  bool both_types_arent_numbers = !(HasNumberOstType(from) && HasNumberOstType(target_type));

  if (from->annotation.is_array &&
      NodeOstensibleType(from) == OST_CHAR &&
      NodeOstensibleType(target_type) == OST_STRING) return true;

  if (!types_match && both_types_arent_numbers) return false;
  if (types_match && both_types_arent_numbers) return true;

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
    return false;
  }

  const int base =
    (from->token.type == HEX_LITERAL)
    ? BASE_HEX
    : (from->token.type == BINARY_LITERAL)
      ? BASE_BINARY
      : BASE_DECIMAL;

  if (NodeOstensibleType(target_type) == OST_FLOAT) {
    double d = TokenToDouble(from->token);
    if (BitWidth(target_type) == 32) return d >= FLT_MIN && d <= FLT_MAX;
    if (BitWidth(target_type) == 64) return d >= DBL_MIN && d <= DBL_MAX;

    ERROR_AND_EXIT_FMTMSG("TypeIsConvertible(): Unknown bitwidth '%d'", BitWidth(target_type));
  }

  if (!IsSigned(target_type)) {
    long long from_value = TokenToULL(from->token, base);

    if (from_value < 0) return false;
    switch(BitWidth(target_type)) {
      case  8: return from_value <= UINT8_MAX;
      case 16: return from_value <= UINT16_MAX;
      case 32: return from_value <= UINT32_MAX;
      case 64: return true;
      default:
        ERROR_AND_EXIT_FMTMSG("TypeIsConvertible(): Unknown bit width: %d\n", BitWidth(target_type));
        return false;
    }
  }

  if (IsSigned(target_type)) {
    long long from_value = TokenToLL(from->token, base);

    switch(BitWidth(target_type)) {
      case  8: return from_value <= INT8_MAX;
      case 16: return from_value <= INT16_MAX;
      case 32: return from_value <= INT32_MAX;
      case 64: return from_value <= INT64_MAX;
      default:
        ERROR_AND_EXIT_FMTMSG("TypeIsConvertible(): Unknown bit width: %d\n", BitWidth(target_type));
        return false;
    }
  }

  return false;
}

ParserAnnotation ShrinkToSmallestContainingType(AST_Node *node) {
  const bool SIGNED = true;
  const bool UNSIGNED = false;
  const int _ = 0;

  const int base =
    (node->token.type == HEX_LITERAL)
    ? BASE_HEX
    : (node->token.type == BINARY_LITERAL)
      ? BASE_BINARY
      : BASE_DECIMAL;

  if (NodeOstensibleType(node) == OST_INT) {
    if (IsSigned(node)) {
      long long value = TokenToLL(node->token, base);

      if (value >= INT8_MIN  && value <= INT8_MAX)  return Annotation(OST_INT,  8, SIGNED);
      if (value >= INT16_MIN && value <= INT16_MAX) return Annotation(OST_INT, 16, SIGNED);
      if (value >= INT32_MIN && value <= INT32_MAX) return Annotation(OST_INT, 32, SIGNED);
      if (value >= INT64_MIN && value <= INT64_MAX) return Annotation(OST_INT, 64, SIGNED);

    } else {
      unsigned long long value = TokenToULL(node->token, base);

      if (value <= UINT8_MAX)  return Annotation(OST_INT,  8, UNSIGNED);
      if (value <= UINT16_MAX) return Annotation(OST_INT, 16, UNSIGNED);
      if (value <= UINT32_MAX) return Annotation(OST_INT, 32, UNSIGNED);
      if (value <= UINT64_MAX) return Annotation(OST_INT, 64, UNSIGNED);
    }
  }

  if (NodeOstensibleType(node) == OST_FLOAT) {
    double d = TokenToDouble(node->token);

    if (d >= 0 && d <= FLT_MAX) return Annotation(OST_FLOAT, 32, SIGNED);
    if (d >= 0 && d <= DBL_MAX) return Annotation(OST_FLOAT, 64, SIGNED);
    ERROR_AND_EXIT_FMTMSG("ShrinkToSmallestContainingType(): Value %e fell through both float and double checks\n", d);
  }

  if (NodeOstensibleType(node) == OST_BOOL) {
    return Annotation(OST_BOOL, 8, _);
  }

  if (NodeOstensibleType(node) == OST_CHAR) {
    return Annotation(OST_CHAR, 8, UNSIGNED);
  }

  if (NodeOstensibleType(node) == OST_STRING) {
    return ArrayAnnotation(CHAR, node->token.length);
  }

  ERROR_AND_EXIT_FMTMSG("ShrinkToSmallestContainingType(): Type %d not implemented yet", NodeOstensibleType(node));
  return Annotation(OST_UNKNOWN, _, _);
}
/* === END HELPERS === */

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
  }

  AST_Node *value = LEFT_NODE(identifier);

  if (identifier->type == TERSE_ASSIGNMENT_NODE) {
    // Treat terse assignment node actual type as the identifier's type
    ActualizeType(identifier, value->annotation);
  }

  bool types_are_same = NodeOstensibleType(identifier) == NodeOstensibleType(value) &&
                        BitWidth(identifier) == BitWidth(value);
  bool types_are_compatible = types_are_same || TypeIsConvertible(value, identifier);

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
}

static void Identifier(AST_Node *identifier) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE, identifier->token);
  ActualizeType(identifier, symbol.annotation);
}

static void Literal(AST_Node *node) {
  if (node->token.type == HEX_LITERAL ||
      node->token.type == BINARY_LITERAL) {
      ShrinkAndActualizeType(node);
    return;
  }

  switch (NodeOstensibleType(node)) {
    case OST_INT:
    case OST_FLOAT:
    case OST_BOOL:
    case OST_CHAR:
    case OST_STRING:
    {
      ShrinkAndActualizeType(node);
      return;
    } break;

    default: {
      ERROR_AND_EXIT_FMTMSG("Literal(): Case '%s' not implemented yet",
                            OstensibleTypeTranslation(node->annotation.ostensible_type));
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
  }
}

static void FunctionCall(AST_Node *node) {
  AST_Node **current = &MIDDLE_NODE(node);
  Symbol fn_definition = RetrieveFrom(SYMBOL_TABLE, node->token);

  for (int i = 0; i < fn_definition.fn_param_count; i++) {
    if (*current == NULL) {
      ERROR_AT_TOKEN(
        node->token,
        "%.*s(): Expected %d arguments, got %d\n",
        node->token.length,
        node->token.position_in_source,
        fn_definition.fn_param_count,
        i);
    }

    AST_Node *argument = *current;
    if (argument == NULL) {
      // This branch might be unreachable as long as the AST is well-formed
      ERROR_AT_TOKEN(
        (*current)->token,
        "%.*s(): Missing '%.*s' argument when calling function\n",
        node->token.length,
        node->token.position_in_source,
        fn_definition.fn_param_list[i].param_token.length,
        fn_definition.fn_param_list[i].param_token.position_in_source
      );
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
    }

    current = &RIGHT_NODE(*current);
  }

  if ((*current) != NULL) {
    ERROR_AT_TOKEN(
      (*current)->token,
      "%.*s(): Too many arguments",
      node->token.length,
      node->token.position_in_source);
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

  if (node->token.type == MINUS) {
    if (NodeActualType(check_node) == ACT_INT) {
      node->annotation = LEFT_NODE(node)->annotation;
      node->annotation.actual_type = ACT_INT;
      node->annotation.is_signed = true;
      return;
    }

    if (NodeActualType(check_node) == ACT_FLOAT) {
      node->annotation = LEFT_NODE(node)->annotation;
      node->annotation.actual_type = ACT_FLOAT;
      node->annotation.is_signed = true;
      return;
    }

    ERROR_AT_TOKEN(node->token,
                   "UnaryOp(): Type disagreement: expected INT or FLOAT, got '%s'",
                   ActualTypeTranslation(check_node->annotation.actual_type));
  }
}

static void BinaryOp(AST_Node *node) {
  node->annotation = LEFT_NODE(node)->annotation;
  node->annotation.actual_type = (ActualType)NodeOstensibleType(node);

  if (!TypeIsConvertible(RIGHT_NODE(node), node)) {
     ERROR_AT_TOKEN(
      RIGHT_NODE(node)->token,
      "BinaryOp(): Cannot convert from type '%s' to '%s'\n",
      AnnotationTranslation(RIGHT_NODE(node)->annotation),
      AnnotationTranslation(node->annotation));
  }

  ActualizeType(RIGHT_NODE(node), node->annotation);
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
    }

    num_literals_in_list++;
    if (num_literals_in_list > node->annotation.array_size) {
      ERROR_AT_TOKEN(
        LEFT_NODE(*current)->token,
        "InitializerList(): Too many elements in initializer list (array size is %d)",
        node->annotation.array_size);
    }

    current = &RIGHT_NODE(*current);
  } while (*current != NULL && LEFT_NODE(*current) != NULL);

  ActualizeType(node, LEFT_NODE(node)->annotation);
}

void CheckTypesRecurse(AST_Node *node) {
  SymbolTable *remember_st = SYMBOL_TABLE;
  if (node->type == FUNCTION_NODE) {
    Symbol s = RetrieveFrom(SYMBOL_TABLE, node->token);
    SYMBOL_TABLE = s.fn_params;
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
    case BINARY_OP_NODE: {
      BinaryOp(node);
    } break;
    case IDENTIFIER_NODE:
    case ENUM_IDENTIFIER_NODE: {
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
  SYMBOL_TABLE = symbol_table;
  CheckTypesRecurse(node);
}
