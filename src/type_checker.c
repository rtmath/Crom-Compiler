#include <errno.h>
#include <float.h>    // for FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
#include <inttypes.h> // for INTX_MIN and INTX_MAX
#include <stdlib.h>   // for strtol and friends

#include "common.h"
#include "error.h"
#include "type_checker.h"

#include <stdio.h>

static SymbolTable *SYMBOL_TABLE;
static Type *in_function;

bool TypeIsConvertible(AST_Node *from, AST_Node *target_type);
Type ShrinkToSmallestContainingType(AST_Node *node);
static void CheckTypesRecurse(AST_Node *node);

/* === HELPERS === */
static int GetBase(AST_Node* node) {
  return (node->token.type == HEX_LITERAL)
           ? BASE_16
           : (node->token.type == BINARY_LITERAL)
               ? BASE_2
               : BASE_10;
}

void VerifyTypeIs(Type type, AST_Node *node) {
  if (TypesMatchExactly(node->value.type, type)) return;

  ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->token, "Expected type '%s', got type '%s'", TypeTranslation(type), TypeTranslation(node->value.type));
}

void ActualizeType(AST_Node *node, Type t) {
  node->value.type = t;
}

void ShrinkAndActualizeType(AST_Node *node) {
  ActualizeType(node, ShrinkToSmallestContainingType(node));

  if (NodeIs_EnumEntry(node))
  {
    SetValueType(SYMBOL_TABLE, node->token, node->value.type);
  }
}

bool Overflow(AST_Node *from, AST_Node *target_type) {
  ERROR_FMT(ERR_OVERFLOW, from->token, "Literal value overflows target type '%s'", TypeTranslation(target_type->value.type));
  return false;
}

bool CanConvertToInt(AST_Node *from, AST_Node *target_type) {
  const int from_base = GetBase(from);

  if (Int64Overflow(from->token, from_base)) {
    return Overflow(from, target_type);
  }

  int64_t from_value = TokenToInt64(from->token, from_base);

  if (TypeIs_I8(target_type->value.type)) {
    if (from_value >= INT8_MIN && from_value <= INT8_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_I16(target_type->value.type)) {
    if (from_value >= INT16_MIN && from_value <= INT16_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_I32(target_type->value.type)) {
    if (from_value >= INT32_MIN && from_value <= INT32_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_I64(target_type->value.type)) {
    if (from_value >= INT64_MIN && from_value <= INT64_MAX) return true;
    return Overflow(from, target_type);

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("CanConverToInt(): Invalid type '%s'", TypeTranslation(target_type->value.type));
    return false;
  }
}

bool CanConvertToUint(AST_Node *from, AST_Node *target_type) {
  const int from_base = GetBase(from);

  if (Uint64Overflow(from->token, from_base)) {
    return Overflow(from, target_type);
  }

  if (TypeIs_Signed(from->value.type)) {
    int64_t check_negative = TokenToInt64(from->token, from_base);
    if (check_negative < 0) return Overflow(from, target_type);
  }

  uint64_t from_value = TokenToUint64(from->token, from_base);

  if (TypeIs_U8(target_type->value.type)) {
    if (from_value <= UINT8_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_U16(target_type->value.type)) {
    if (from_value <= UINT16_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_U32(target_type->value.type)) {
    if (from_value <= UINT32_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_U64(target_type->value.type)) {
    if (from_value <= UINT64_MAX) return true;
    return Overflow(from, target_type);

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("CanConvertToUint(): Invalid type '%s'\n", TypeTranslation(target_type->value.type));
    return false;
  }
}

bool CanConvertToFloat(AST_Node *from, AST_Node *target_type) {
  if (DoubleOverflow(from->token)) {
    return Overflow(from, target_type);
  }

  double from_value = TokenToDouble(from->token);

  if (TypeIs_F32(target_type->value.type)) {
    if (from_value >= -FLT_MAX && from_value <= FLT_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_F64(target_type->value.type)) {
    if (from_value >= -DBL_MAX && from_value <= DBL_MAX) return true;
    return Overflow(from, target_type);

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("CanConvertToFloat(): Invalid type '%s'\n", TypeTranslation(target_type->value.type));
    return false;
  }
}

bool TypeIsConvertible(AST_Node *from, AST_Node *target_type) {
  bool types_match = TypesMatchExactly(from->value.type, target_type->value.type) ||
                     TypesAreInt(from->value.type, target_type->value.type)       ||
                     TypesAreUint(from->value.type, target_type->value.type)      ||
                     TypesAreFloat(from->value.type, target_type->value.type);
  bool types_are_not_numbers = !(TypeIs_Numeric(from->value.type) && TypeIs_Numeric(target_type->value.type));

  if (!types_match && types_are_not_numbers) return false;
  if (types_match && types_are_not_numbers) return true;

  if (NodeIs_Identifier(from) ||
      NodeIs_Return(from)) {
    return types_match;
  }

  if (TypeIs_Float(target_type->value.type)) {
    return CanConvertToFloat(from, target_type);
  }

  if (!TypeIs_Signed(target_type->value.type)) {
    return CanConvertToUint(from, target_type);
  }

  if (TypeIs_Signed(target_type->value.type)) {
    return CanConvertToInt(from, target_type);
  }

  return false;
}

Type ShrinkToSmallestContainingType(AST_Node *node) {
  const int base = GetBase(node);

  if (TypeIs_Int(node->value.type)) {
    if (Int64Overflow(node->token, base)) {
      SetErrorCode(ERR_OVERFLOW);
      return NoType();
    }

    int64_t value = TokenToInt64(node->token, base);
    return SmallestContainingIntType(value);

  } else if (TypeIs_Uint(node->value.type)) {
    if (Uint64Overflow(node->token, base)) {
      SetErrorCode(ERR_OVERFLOW);
      return NoType();
    }

    uint64_t value = TokenToUint64(node->token, base);
    return SmallestContainingUintType(value);

  } else if (TypeIs_Float(node->value.type)) {
    if (DoubleOverflow(node->token)) {
      SetErrorCode(ERR_OVERFLOW);
      return NoType();
    }

    if (DoubleUnderflow(node->token)) {
      SetErrorCode(ERR_UNDERFLOW);
      return NoType();
    }
    double d = TokenToDouble(node->token);
    return SmallestContainingFloatType(d);

  }

  if (TypeIs_Bool(node->value.type)) {
    return NewType(BOOL);
  }

  if (TypeIs_Char(node->value.type)) {
    return NewType(CHAR);
  }

  if (TypeIs_String(node->value.type)) {
    return NewType(STRING);
  }

  SetErrorCode(ERR_PEBCAK);
  COMPILER_ERROR_FMTMSG("ShrinkToSmallestContainingType(): Invalid type '%s'", TypeTranslation(node->value.type));
  return NoType();
}
/* === END HELPERS === */

static void String(AST_Node *str) {
  Type t = str->value.type;

  t.category = TC_ARRAY;
  t.array_size = str->token.length;
  ActualizeType(str, t);
}

static void Declaration(AST_Node *identifier) {
  ActualizeType(identifier, identifier->value.type);
}

static void Assignment(AST_Node *identifier) {
  if (!TypeIs_Array(identifier->value.type) && identifier->middle != NULL) {
    ERROR_FMT(ERR_IMPROPER_ASSIGNMENT, identifier->token, "'%.*s' is not an array", identifier->token.length, identifier->token.position_in_source);
  }

  AST_Node *value = identifier->left;
  if (NodeIs_EnumAssignment(identifier) &&
      (!TypeIs_Int(value->value.type) || NodeIs_Identifier(value))) {
    ERROR_MSG(ERR_IMPROPER_ASSIGNMENT, value->token, "Assignment to enum identifier must be of type INT");
  }

  if (!TypeIs_Uint(identifier->value.type) &&
      (value->token.type == HEX_LITERAL || value->token.type == BINARY_LITERAL))
  {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, value->token, "'%s' cannot be assigned to non-Uint types", TokenTypeTranslation(value->token.type));
  }

  if (NodeIs_TerseAssignment(identifier)) {
    // Treat terse assignment node actual type as the identifier's type
    ActualizeType(identifier, value->value.type);
  }

  bool types_are_compatible = TypeIsConvertible(value, identifier);

  if (types_are_compatible) {
    if (TypeIs_String(identifier->value.type)) {
      // For strings, propagate the type information from child node
      // to parent in order to get the length of the string
      ActualizeType(identifier, value->value.type);
    } else {
      // Otherwise actualize the ostensible declared type of the identifier
      ActualizeType(identifier, identifier->value.type);
    }

    // Synchronize information between nodes
    bool assignment_to_array_slot = (TypeIs_Array(identifier->value.type) && !TypeIs_Array(value->value.type));
    ActualizeType(value, identifier->value.type);
    if (assignment_to_array_slot) {
      value->value.type.category = TC_NONE;
    }

    if (NodeIs_Identifier(value)) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE, value->token);
      identifier->value = s.value;
    }

    return;
  }

  if (TypeIs_Enum(value->value.type)) {
    ERROR(ERR_IMPROPER_ASSIGNMENT, identifier->token);
  }

  ERROR_FMT(ERR_TYPE_DISAGREEMENT, identifier->token,
            "Type disagreement between '%.*s' (%s) and (%s)",
            identifier->token.length, identifier->token.position_in_source,
            TypeTranslation(identifier->value.type),
            TypeTranslation(identifier->left->value.type));
}

static void Identifier(AST_Node *identifier) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE, identifier->token);
  if (symbol.token.type == ERROR && in_function != NULL) {
    FnParam *param = GetFunctionParam(*in_function, identifier->token);
    if (param != NULL) {
      symbol = NewSymbol(param->token, param->type, DECL_DEFINED);
    }
  }
  Type t = symbol.value.type;

  if (!NodeIs_NULL(identifier->middle) &&
      NodeIs_ArraySubscript(identifier->middle) &&
      TypeIs_String(identifier->value.type)) {
    t = NewType(CHAR);
  }

  ActualizeType(identifier, t);
}

static void Literal(AST_Node *node) {
  if (node->token.type == HEX_LITERAL ||
      node->token.type == BINARY_LITERAL) {
      node->value = NewValue(node->value.type, node->token);
      ShrinkAndActualizeType(node);
    return;
  }

  if (TypeIs_Int(node->value.type)   ||
      TypeIs_Uint(node->value.type)  ||
      TypeIs_Float(node->value.type) ||
      TypeIs_Bool(node->value.type)  ||
      TypeIs_Char(node->value.type))
  {
    ShrinkAndActualizeType(node);
    node->value = NewValue(node->value.type, node->token);
    return;

  } else if (TypeIs_String(node->value.type)) {
    String(node);
    node->value = NewValue(node->value.type, node->token);
    return;

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("Literal(): Case '%s' not implemented yet", TypeTranslation(node->value.type));
  }
}

static void IncrementOrDecrement(AST_Node *node) {
  (NodeIs_PrefixIncrement(node) || NodeIs_PrefixDecrement(node))
    ? ActualizeType(node, node->left->value.type)
    : ActualizeType(node, node->value.type);
}

static void Return(AST_Node* node) {
  if (TypeIs_Void(node->value.type)) {
    node->value.type.specifier = T_VOID;
    return;
  }

  ActualizeType(node, node->left->value.type);
}

static bool IsDeadEnd(AST_Node *node) {
  return (node == NULL) ||
         (NodeIs_Chain(node)        &&
          NodeIs_NULL(node->left)   &&
          NodeIs_NULL(node->middle) &&
          NodeIs_NULL(node->right));
}

static void TypeCheckNestedReturns(AST_Node *node, AST_Node *return_type) {
  AST_Node **current = &node;

  do {
    if (!NodeIs_NULL((*current)->left)) {
      switch((*current)->left->node_type) {
        case IF_NODE:
        case WHILE_NODE:
        case FOR_NODE: {
          TypeCheckNestedReturns((*current)->left, return_type);
        } break;
        default: break;
      }
    }

    if (!NodeIs_NULL((*current)->middle)) {
      switch((*current)->middle->node_type) {
        case IF_NODE:
        case WHILE_NODE:
        case FOR_NODE:
        case CHAIN_NODE: {
          TypeCheckNestedReturns((*current)->middle, return_type);
        } break;
        default: break;
      }
    }

    if (!NodeIs_NULL((*current)->right)) {
      switch((*current)->right->node_type) {
        case IF_NODE:
        case WHILE_NODE:
        case FOR_NODE: {
          TypeCheckNestedReturns((*current)->right, return_type);
        } break;
        default: break;
      }
    }

    if (NodeIs_Return((*current)->left)) {
      if (!TypeIsConvertible((*current)->left->left, return_type)) {
        ERROR_FMT(ERR_TYPE_DISAGREEMENT, (*current)->left->left->token,
                  "Can't convert from %s to %s",
                  TypeTranslation((*current)->left->value.type),
                  TypeTranslation(return_type->value.type));
      }
    }

    current = &(*current)->right;

  } while(!IsDeadEnd(*current));
}

static void Function(AST_Node *node) {
  AST_Node *return_type = node->left;
  AST_Node *body = node->right;
  AST_Node **check = &body;

  do {
    if (NodeIs_If((*check)->left)    ||
        NodeIs_While((*check)->left) ||
        NodeIs_For((*check)->left)) {
      TypeCheckNestedReturns((*check)->left, return_type);
    }

    if (NodeIs_Return((*check)->left)) {
      if (TypeIsConvertible((*check)->left, return_type)) {
        ActualizeType(node, node->value.type);

        if (!IsDeadEnd((*check)->right)) {
          ERROR(ERR_UNREACHABLE_CODE, (*check)->right->left->token);
        }

        return;
      } else if (TypeIs_Void((*check)->left->value.type)) {
        /* Do nothing
         *
         * This case occurs when a non-void function has no return in the body.
         * The parser inserts a void return node into the body and this function
         * segfaults without this check. The 'missing return' error will
         * trigger appropriately after the do-while loop finishes. */
      } else {
        ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->left->token,
                  "%s(): Can't convert from return type %s to %s",
                  node->token.length, node->token.position_in_source,
                  TypeTranslation((*check)->left->value.type),
                  TypeTranslation(return_type->value.type));
      }
    }

    check = &(*check)->right;
  } while (!IsDeadEnd(*check));

  if (TypeIs_Void(return_type->value.type)) {
    node->value.type.specifier = T_VOID;
  } else {
    ERROR(ERR_MISSING_RETURN, node->token);
  }
}

/*
static void FunctionCall(AST_Node *node) {
  AST_Node **current = &(node)->middle;
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
      SetErrorCode(ERR_TOO_FEW);
    }

    // TypeIsConvertible deals with AST_Nodes but fn_params aren't an AST_Node
    AST_Node *wrapped_param = NewNodeFromToken(UNTYPED_NODE, NULL, NULL, NULL,
      fn_definition.fn_param_list[i].param_token,
      fn_definition.fn_param_list[i].type
    );

    if (!TypeIsConvertible(argument, wrapped_param)) {
      ERROR_AT_TOKEN(argument->token,
                     "%.*s(): Can't convert type from '%s' to '%s'\n",
                     node->token.length,
                     node->token.position_in_source,
                     TypeTranslation(argument->value.type),
                     TypeTranslation(wrapped_param->value.type));
      SetErrorCode(ERR_TYPE_DISAGREEMENT);
    }

    current = &(*current)->right;
  }

  if ((*current) != NULL) {
    ERROR_AT_TOKEN((*current)->token,
                   "%.*s(): Too many arguments",
                   node->token.length,
                   node->token.position_in_source);
    SetErrorCode(ERR_TOO_MANY);
  }

  ActualizeType(node, node->value.type);
}
*/

static void UnaryOp(AST_Node *node) {
  AST_Node *check_node = (node)->left;

  if (node->token.type == LOGICAL_NOT) {
    VerifyTypeIs(NewType(BOOL), check_node);
    node->value.type = node->left->value.type;
    return;
  }

  if (node->token.type == BITWISE_NOT) {
    if (!TypeIs_Uint(check_node->value.type)) {
      ERROR_MSG(ERR_TYPE_DISAGREEMENT, check_node->token, "Operand must be of type Uint");
    }

    node->value.type = node->left->value.type;
    return;
  }

  if (node->token.type == MINUS) {
    if (check_node->token.type == HEX_LITERAL ||
        check_node->token.type == BINARY_LITERAL) {
      ERROR_FMT(ERR_TYPE_DISAGREEMENT, check_node->token, "'%s' not allowed with unary '-'", TokenTypeTranslation(check_node->token.type));
      return;
    }

    if (TypeIs_Int(check_node->value.type)) {
      node->value = NewIntValue(-(node->left->value.as.integer));
      return;
    }

    if (TypeIs_Uint(check_node->value.type)) {
      Value int_conversion = NewIntValue(-(node->left->value.as.uinteger));
      node->value = int_conversion;
      check_node->value.type = int_conversion.type;
      return;
    }

    if (TypeIs_Float(check_node->value.type)) {
      node->value = NewFloatValue(-(node->left->value.as.floating));
      return;
    }

    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->token, "Expected INT or FLOAT, got '%s' instead'", TypeTranslation(check_node->value.type));
  }
}

static void BinaryArithmeticOp(AST_Node *node) {
  node->value.type = node->left->value.type;

  if (!TypeIsConvertible(node->right, node)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->right->token, "Can't convert from type %s to %s", TypeTranslation(node->right->value.type), TypeTranslation(node->value.type));
  }

  ActualizeType(node->right, node->value.type);
}

static void BinaryLogicalOp(AST_Node *node) {
  switch(node->token.type) {
    case LESS_THAN:
    case GREATER_THAN:
    case LESS_THAN_EQUALS:
    case GREATER_THAN_EQUALS: {
      // Check left node individually for incorrect type
      if (TypeIs_Int(node->left->value.type) &&
          TypeIs_Float(node->left->value.type)) {
        ERROR_FMT(ERR_UNEXPECTED, node->left->token, "Expected BOOL, got '%s'", TypeTranslation(node->left->value.type));
      }

      // Check right node individually for incorrect type
      if (TypeIs_Int(node->right->value.type) &&
          TypeIs_Float(node->right->value.type)) {
        ERROR_FMT(ERR_UNEXPECTED, node->right->token, "Expected BOOL, got '%s'", TypeTranslation(node->right->value.type));
      }
    } /* Intentional fallthrough */
    case LOGICAL_NOT_EQUALS:
    case EQUALITY: {
      // If left node is signed, check if right node can be converted
      if (TypeIs_Int(node->left->value.type) &&
          TypeIs_Uint(node->right->value.type))
      {
        if (TypeIsConvertible(node->right, node->left)) {
          ActualizeType(node->right, node->left->value.type);
        } else {
          ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->right->token, "Can't convert from %s to %s", TypeTranslation(node->right->value.type), TypeTranslation(node->left->value.type));
        }
      }

      // If right node is signed, check if left node can be converted
      if (TypeIs_Uint(node->left->value.type) &&
          TypeIs_Int(node->right->value.type))
      {
        if (TypeIsConvertible(node->left, node->right)) {
          ActualizeType(node->left, node->right->value.type);
        } else {
          ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->token, "Can't convert from type %s to %s",  TypeTranslation(node->right->value.type), TypeTranslation(node->left->value.type));
        }
      }

      ActualizeType(node, node->value.type);
    } break;

    case LOGICAL_AND:
    case LOGICAL_OR: {
      if (!TypeIs_Bool(node->left->value.type)) {
        ERROR_FMT(ERR_UNEXPECTED, node->left->token, "Expected BOOL, got '%s'", TypeTranslation(node->left->value.type));
      }

      if (!TypeIs_Bool(node->right->value.type)) {
        ERROR_FMT(ERR_UNEXPECTED, node->right->token, "Expected BOOL, got '%s'", TypeTranslation(node->right->value.type));
      }

      ActualizeType(node, node->value.type);
    } break;

    default: {
      Print("BinaryLogicalOp(): Not implemented yet\n");
    } break;
  }
}

static void BinaryBitwiseOp(AST_Node *node) {
  AST_Node *left_value = node->left;
  AST_Node *right_value = node->right;

  // Check left node individually for incorrect type
  if (!TypeIs_Uint(left_value->value.type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, left_value->token, "Expected UINT, got '%s'", TypeTranslation(left_value->value.type));
  }

  // Check right node individually for incorrect type
  if (!TypeIs_Uint(right_value->value.type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, right_value->token, "Expected UINT, got '%s'", TypeTranslation(right_value->value.type));
  }

  Type largest_containing_type =
    (left_value->value.type.specifier > right_value->value.type.specifier)
      ? left_value->value.type
      : right_value->value.type;

  ActualizeType(node, largest_containing_type);
}

static void InitializerList(AST_Node *node) {
  AST_Node **current = &node;

  int num_literals_in_list = 0;
  do {
    if (!TypeIsConvertible((*current)->left, node)) {
      ERROR_FMT(ERR_TYPE_DISAGREEMENT, (*current)->left->token, "Can't convert from %s to %s", TypeTranslation((*current)->left->value.type), TypeTranslation(node->value.type));
    }

    num_literals_in_list++;
    if (num_literals_in_list > node->value.type.array_size) {
      ERROR_FMT(ERR_TOO_MANY, (*current)->left->token, "Too many elements (%d) in initializer list (array size is %d)", num_literals_in_list, node->value.type.array_size);
    }

    current = &(*current)->right;
  } while (*current != NULL && (*current)->left != NULL);

  ActualizeType(node, (node)->left->value.type);
}

static void EnumListRecurse(AST_Node *node, int implicit_value) {
  AST_Node *list_entry = (node)->left;

  if (list_entry == NULL) return;

  if (NodeIs_EnumAssignment(list_entry)) {
    CheckTypesRecurse(list_entry);
    AST_Node *value = (list_entry)->left;
    if ((!TypeIs_Int(value->value.type) && !TypeIs_Uint(value->value.type)) ||
        NodeIs_Identifier(value))
    {
      ERROR_MSG(ERR_IMPROPER_ASSIGNMENT, value->token, "Assignment to enum identifier must be of type INT");
    }

    ShrinkAndActualizeType(list_entry);
    SetValue(SYMBOL_TABLE, list_entry->token, list_entry->left->value);

    implicit_value = (list_entry->left->value.as.integer + 1);
  }

  if (NodeIs_EnumEntry(list_entry)) {
    list_entry->value = NewIntValue(implicit_value);
    ShrinkAndActualizeType(list_entry);
    SetValue(SYMBOL_TABLE, list_entry->token, list_entry->value);

    implicit_value++;
  }

  EnumListRecurse(node->right, implicit_value);
}

static void HandleEnum(AST_Node *node) {
  EnumListRecurse(node, 0);
  ActualizeType(node, node->value.type);
}

static void StructMemberAccess(AST_Node *struct_identifier) {
  if (struct_identifier->left == NULL) {
    return;
  }
  AST_Node *member_node = struct_identifier->left;

  if (member_node == NULL || NodeIs_StructMember(member_node)) return;

  Symbol struct_symbol = RetrieveFrom(SYMBOL_TABLE, struct_identifier->token);
  StructMember *member = GetStructMember(struct_symbol.value.type, member_node->token);

  ActualizeType(struct_identifier, member->type);
}

static void CheckTypesRecurse(AST_Node *node) {
  if (NodeIs_EnumIdentifier(node)) {
    HandleEnum(node);
    return;
  }

  if (NodeIs_Function(node)) {
    in_function = &node->value.type;
  }

  if (node->left   != NULL) CheckTypesRecurse(node->left);
  if (node->middle != NULL) CheckTypesRecurse(node->middle);
  if (node->right  != NULL) CheckTypesRecurse(node->right);

  if (NodeIs_Function(node)) {
    in_function = NULL;
  }

  switch(node->node_type) {
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
    case STRUCT_DECLARATION_NODE:
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
      ActualizeType(node, node->value.type);
    } break;
    case FUNCTION_NODE: {
      Function(node);
    } break;
    case FUNCTION_CALL_NODE: {
      //FunctionCall(node);
    } break;
    case RETURN_NODE: {
      Return(node);
    } break;
    case STRUCT_IDENTIFIER_NODE: {
      StructMemberAccess(node);
    } break;
    case ARRAY_INITIALIZER_LIST_NODE: {
      InitializerList(node);
    } break;
    default: {
    } break;
  }

  if (!NodeIs_Start(node) &&
      !NodeIs_Chain(node)) {
    //PrintNode(node);
  }
}

void CheckTypes(AST_Node *node, SymbolTable *symbol_table) {
  SYMBOL_TABLE = symbol_table;
  CheckTypesRecurse(node);
}
