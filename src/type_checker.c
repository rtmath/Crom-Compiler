#include <errno.h>
#include <float.h>    // for FLT_MIN, FLT_MAX, DBL_MIN, DBL_MAX
#include <inttypes.h> // for INTX_MIN and INTX_MAX
#include <stdlib.h>   // for strtol and friends

#include "common.h"
#include "error.h"
#include "type_checker.h"

#include <stdio.h>

/* === Globals === */
static SymbolTable *SYMBOL_TABLE;
static Type *in_function;

/* === Forward Declarations === */
static void CheckTypesRecurse(AST_Node *node);

/* === Helpers === */
bool Overflow(AST_Node *from, Type target_type) {
  ERROR_FMT(ERR_OVERFLOW, from->token, "Literal value overflows target type '%s'", TypeTranslation(target_type));
  return false; // The return type is unused as ERROR_FMT will call Exit(), but this suppresses -Wreturn-type warnings at the call site
}

bool CanConvertToInt(AST_Node *from, Type target_type) {
  if (Int64Overflow(from->token)) {
    ERROR_FMT(ERR_OVERFLOW, from->token, "Literal value overflows target type '%s'", TypeTranslation(target_type));
  }

  int64_t from_value = TokenToInt64(from->token);

  if (TypeIs_I8(target_type)) {
    if (from_value >= INT8_MIN && from_value <= INT8_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_I16(target_type)) {
    if (from_value >= INT16_MIN && from_value <= INT16_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_I32(target_type)) {
    if (from_value >= INT32_MIN && from_value <= INT32_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_I64(target_type)) {
    if (from_value >= INT64_MIN && from_value <= INT64_MAX) return true;
    return Overflow(from, target_type);

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("CanConverToInt(): Invalid type '%s'", TypeTranslation(target_type));
    return false;
  }
}

bool CanConvertToUint(AST_Node *from, Type target_type) {
  if (Uint64Overflow(from->token)) {
    return Overflow(from, target_type);
  }

  uint64_t from_value = TokenToUint64(from->token);

  if (TypeIs_U8(target_type)) {
    if (from_value <= UINT8_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_U16(target_type)) {
    if (from_value <= UINT16_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_U32(target_type)) {
    if (from_value <= UINT32_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_U64(target_type)) {
    if (from_value <= UINT64_MAX) return true;
    return Overflow(from, target_type);

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("CanConvertToUint(): Invalid type '%s'\n", TypeTranslation(target_type));
    return false;
  }
}

bool CanConvertToFloat(AST_Node *from, Type target_type) {
  if (DoubleOverflow(from->token)) {
    return Overflow(from, target_type);
  }

  double from_value = TokenToDouble(from->token);

  if (TypeIs_F32(target_type)) {
    if (from_value >= -FLT_MAX && from_value <= FLT_MAX) return true;
    return Overflow(from, target_type);

  } else if (TypeIs_F64(target_type)) {
    if (from_value >= -DBL_MAX && from_value <= DBL_MAX) return true;
    return Overflow(from, target_type);

  } else {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR_FMTMSG("CanConvertToFloat(): Invalid type '%s'\n", TypeTranslation(target_type));
    return false;
  }
}

bool TypeIsConvertible(AST_Node *from, Type target_type) {
  bool types_match = TypesMatchExactly(from->data_type, target_type) ||
                     TypesAreInt(from->data_type, target_type)       ||
                     TypesAreUint(from->data_type, target_type)      ||
                     TypesAreFloat(from->data_type, target_type)     ||
                     (TypeIs_Function(from->data_type) && (from->data_type.specifier == target_type.specifier));
  bool types_are_not_numbers = !(TypeIs_Numeric(from->data_type) && TypeIs_Numeric(target_type));

  if (!types_match && types_are_not_numbers) return false;
  if (types_match && types_are_not_numbers) return true;

  if (NodeIs_Identifier(from) ||
      NodeIs_Return(from) ||
      NodeIs_TernaryIf(from)) {
    return types_match;
  }

  if (TypeIs_Void(from->data_type) && TypeIs_Void(target_type)) return true;

  if (TypeIs_Float(target_type)) {
    return CanConvertToFloat(from, target_type);
  }

  if (TypeIs_Float(from->data_type)) {
    // Disallow implicit float->integer conversion
    return false;
  }

  if (!TypeIs_Signed(target_type)) {
    return CanConvertToUint(from, target_type);
  }

  if (TypeIs_Signed(target_type)) {
    return CanConvertToInt(from, target_type);
  }

  return false;
}
/* === End Helpers === */

static void Literal(AST_Node *n) {
  if (TypeIs_Int(n->data_type) && Int64Overflow(n->token)) {
    SetNodeDataType(n, NewType(U64));
  }

  if (TypeIs_Uint(n->data_type) && Uint64Overflow(n->token)) {
    Overflow(n, NewType(U64));
  }
}

static void ArrayInitializerList(AST_Node *list, Type target_type) {
  AST_Node **current = &list;

  int num_literals_in_list = 0;

  while (*current != NULL && (*current)->left != NULL) {
    AST_Node *value = (*current)->left;
    if (!TypeIsConvertible(value, target_type)) {
      ERROR_FMT(ERR_TYPE_DISAGREEMENT, value->token, "Can't convert from %s to %s", TypeTranslation(value->data_type), TypeTranslation(target_type));
    }

    if (!TypeIsConvertible(value, list->data_type)) {
      ERROR_FMT(ERR_TYPE_DISAGREEMENT, value->token, "Can't convert from %s to %s", TypeTranslation(value->data_type), TypeTranslation(list->data_type));
    }

    num_literals_in_list++;
    if (num_literals_in_list > target_type.array_size) {
      ERROR_FMT(ERR_TOO_MANY, value->token, "Too many elements (%d) in initializer list (array size is %d)", num_literals_in_list, target_type.array_size);
    }

    current = &(*current)->right;
  }
}

static void StructInitializerList(AST_Node *list, Type target_type) {
  StructMember **current_member = &target_type.members.next;
  AST_Node **current_value = &list;

  while (*current_value != NULL && (*current_value)->left != NULL) {
    AST_Node *value = (*current_value)->left;
    if (*current_member == NULL) ERROR_MSG(ERR_TOO_MANY, value->token, "Too many elements in initializer list");

    if (!TypeIsConvertible(value, (*current_member)->type)) {
      ERROR_FMT(ERR_TYPE_DISAGREEMENT, value->token, "Can't convert from %s to %s", TypeTranslation(value->data_type), TypeTranslation(target_type));
    }

    current_value = &(*current_value)->right;
    current_member = &(*current_member)->next;
  }
}

static void Assignment(AST_Node *identifier) {
  if (!TypeIs_Array(identifier->data_type) && identifier->middle != NULL) {
    ERROR_FMT(ERR_IMPROPER_ASSIGNMENT, identifier->token, "'%.*s' is not an array", identifier->token.length, identifier->token.position_in_source);
  }

  AST_Node *value = identifier->left;
  if (NodeIs_EnumAssignment(identifier) &&
      (!TypeIs_Int(value->data_type) || NodeIs_Identifier(value))) {
    ERROR_MSG(ERR_IMPROPER_ASSIGNMENT, value->token, "Assignment to enum identifier must be of type INT");
  }

  if (!TypeIs_Uint(identifier->data_type) &&
      (value->token.type == HEX_LITERAL || value->token.type == BINARY_LITERAL))
  {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, value->token, "'%s' cannot be assigned to non-Uint types", TokenTypeTranslation(value->token.type));
  }

  if (NodeIs_TerseAssignment(identifier)) {
    if (!TypeIs_Numeric(value->data_type)) {
      ERROR_MSG(ERR_TYPE_DISAGREEMENT, value->token, "Left hand side must be numeric");
    }

    if (identifier->right == NULL ||
        !TypeIs_Numeric(identifier->right->data_type)) {
      ERROR_MSG(ERR_TYPE_DISAGREEMENT, identifier->right->token, "Right hand side must be numeric");
    }

    SetNodeDataType(identifier, value->data_type);
  }

  if (NodeIs_InitializerList(value)) {
    if (TypeIs_Struct(identifier->data_type)) {
      StructInitializerList(value, identifier->data_type);
    } else {
      ArrayInitializerList(value, identifier->data_type);
    }
    return;
  }

  if (!TypeIsConvertible(value, identifier->data_type)) {
    if (TypeIs_Enum(value->data_type)) {
      ERROR(ERR_IMPROPER_ASSIGNMENT, identifier->token);
    }

    ERROR_FMT(ERR_TYPE_DISAGREEMENT, identifier->token,
              "Type disagreement between '%.*s' (%s) and (%s)",
              identifier->token.length, identifier->token.position_in_source,
              TypeTranslation(identifier->data_type),
              TypeTranslation(identifier->left->data_type));
  }

  if (TypeIs_String(identifier->data_type)) {
    // For strings, propagate the type information from child node
    // to parent in order to get the length of the string
    SetNodeDataType(identifier, value->data_type);
  }

  // Synchronize information between nodes
  bool assignment_to_array_slot = (TypeIs_Array(identifier->data_type) && !TypeIs_Array(value->data_type));
  if (assignment_to_array_slot) {
    value->data_type.category = TC_NONE;
  }

  if (NodeIs_Identifier(value)) {
    Symbol s = RetrieveFrom(SYMBOL_TABLE, value->token);
    SetNodeDataType(identifier, s.data_type);
  }

  SetNodeDataType(value, identifier->data_type);

  return;
}

static void Identifier(AST_Node *identifier) {
  if (!TypeIs_Array(identifier->data_type) && identifier->middle != NULL) {
    ERROR_FMT(ERR_IMPROPER_ACCESS, identifier->token, "'%.*s' is not an array", identifier->token.length, identifier->token.position_in_source);
  }

  Symbol symbol = RetrieveFrom(SYMBOL_TABLE, identifier->token);
  if (symbol.token.type == ERROR && in_function != NULL) {
    FnParam *param = GetFunctionParam(*in_function, identifier->token);
    if (param != NULL) {
      symbol = NewSymbol(param->token, param->type, DECL_DEFINED);
    }
  }

  if (!NodeIs_NULL(identifier->middle) &&
      NodeIs_ArraySubscript(identifier->middle) &&
      TypeIs_String(identifier->data_type)) {
    SetNodeDataType(identifier, NewType(CHAR));
  }
}

static void Return(AST_Node* node) {
  if (TypeIs_Void(node->data_type)) {
    node->data_type.specifier = T_VOID;
    return;
  }

  SetNodeDataType(node, node->left->data_type);
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

  while(!IsDeadEnd(*current)) {
    AST_Node *left = (*current)->left;
    AST_Node *middle = (*current)->middle;
    AST_Node *right = (*current)->right;

    if (!NodeIs_NULL(left) &&
        ((NodeIs_If(left)    ||
          NodeIs_While(left) ||
          NodeIs_For(left)))) {
          TypeCheckNestedReturns(left, return_type);
    }

    if (!NodeIs_NULL(middle) &&
        ((NodeIs_If(middle)    ||
          NodeIs_While(middle) ||
          NodeIs_For(middle)   ||
          NodeIs_Chain(middle)))) {
          TypeCheckNestedReturns(middle, return_type);
    }

    if (!NodeIs_NULL(right) &&
        ((NodeIs_If(right)    ||
          NodeIs_While(right) ||
          NodeIs_For(right)))) {
          TypeCheckNestedReturns(right, return_type);
    }

    if (NodeIs_Return(left)) {
      bool missing_return = left->left == NULL;

      if ((TypeIs_Void(left->data_type) || missing_return) &&
          !TypeIs_Void(return_type->data_type)) {
        ERROR_MSG(ERR_TYPE_DISAGREEMENT, left->token, "Void return in non-void function");
      }

      if (!TypeIs_Void(left->data_type) &&
          TypeIs_Void(return_type->data_type)) {
        ERROR_MSG(ERR_TYPE_DISAGREEMENT, left->token, "Non-void return in void function");
      }

      if (!missing_return &&
          !TypeIsConvertible(left->left, return_type->data_type)) {
        ERROR_FMT(ERR_TYPE_DISAGREEMENT, left->left->token,
                  "Can't convert from %s to %s",
                  TypeTranslation(left->data_type),
                  TypeTranslation(return_type->data_type));
      }
    }

    current = &right;
  }
}

static void Function(AST_Node *node) {
  AST_Node *return_type = node->left;
  AST_Node *body = node->right;
  AST_Node **check = &body;

  while (!IsDeadEnd(*check)) {
    AST_Node *left = (*check)->left;
    AST_Node *right = (*check)->right;
    if (NodeIs_If(left)    ||
        NodeIs_While(left) ||
        NodeIs_For(left)) {
      TypeCheckNestedReturns(left, return_type);
    }

    if (NodeIs_Return(left)) {
      bool void_return = TypeIs_Void(left->data_type);

      if (void_return && TypeIs_Void(return_type->data_type)) {
        /* Do nothing
         *
         * This case occurs when a non-void function has no return in the body.
         * The parser inserts a void return node into the body and this function
         * segfaults without this check. The 'missing return' error will
         * trigger appropriately after the do-while loop finishes. */
      } else if (void_return && !TypeIs_Void(return_type->data_type)) {
        ERROR_MSG(ERR_TYPE_DISAGREEMENT, left->token, "Void return in non-void function");

      } else if (TypeIsConvertible(left, return_type->data_type)) {
        if (!IsDeadEnd(right)) {
          ERROR(ERR_UNREACHABLE_CODE, right->left->token);
        }
        return;

      } else {
        ERROR_FMT(ERR_TYPE_DISAGREEMENT,
                  left->left->token,
                  "%.*s(): Can't convert from return type %s to %s",
                  node->token.length, node->token.position_in_source,
                  TypeTranslation(left->data_type),
                  TypeTranslation(return_type->data_type));
      }
    }

    check = &right;
  };

  if (TypeIs_Void(return_type->data_type)) {
    node->data_type.specifier = T_VOID;
  } else {
    ERROR(ERR_MISSING_RETURN, node->token);
  }
}

static void FunctionCall(AST_Node *node) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE, node->token);
  node->data_type = s.data_type;
}

static void UnaryOp(AST_Node *node) {
  AST_Node *check_node = (node)->left;

  if (node->token.type == LOGICAL_NOT) {
    if (!TypeIs_Bool(check_node->data_type)) {
      ERROR_MSG(ERR_TYPE_DISAGREEMENT, check_node->token, "Operand must be type BOOL");
    }

    SetNodeDataType(node, node->left->data_type);
    return;
  }

  if (node->token.type == BITWISE_NOT) {
    if (!TypeIs_Uint(check_node->data_type)) {
      ERROR_MSG(ERR_TYPE_DISAGREEMENT, check_node->token, "Operand must be of type Uint");
    }

    SetNodeDataType(node, node->left->data_type);
    return;
  }

  if (node->token.type == MINUS) {
    if (check_node->token.type == HEX_LITERAL ||
        check_node->token.type == BINARY_LITERAL) {
      ERROR_FMT(ERR_TYPE_DISAGREEMENT, check_node->token, "'%s' not allowed with unary '-'", TokenTypeTranslation(check_node->token.type));
      return;
    }

    if (TypeIs_Int(check_node->data_type)) {
      SetNodeDataType(node, check_node->data_type);
      return;
    }

    if (TypeIs_Uint(check_node->data_type)) {
      ERROR_MSG(ERR_TYPE_DISAGREEMENT, check_node->token, "Uint not allowed with unary '-'");
      return;
    }

    if (TypeIs_Float(check_node->data_type)) {
      SetNodeDataType(node, check_node->data_type);
      return;
    }

    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->token, "Expected INT or FLOAT, got '%s' instead'", TypeTranslation(check_node->data_type));
  }
}

static void BinaryArithmeticOp(AST_Node *node) {
  SetNodeDataType(node, node->left->data_type);

  if (!TypeIsConvertible(node->right, node->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->right->token, "Can't convert from type %s to %s", TypeTranslation(node->right->data_type), TypeTranslation(node->data_type));
  }

  SetNodeDataType(node->right, node->data_type);
}

static void BinaryLogicalOp(AST_Node *node) {
  switch(node->token.type) {
    case LESS_THAN:
    case GREATER_THAN:
    case LESS_THAN_EQUALS:
    case GREATER_THAN_EQUALS: {
      // Check left node individually for incorrect type
      if (TypeIs_Int(node->left->data_type) &&
          TypeIs_Float(node->left->data_type)) {
        ERROR_FMT(ERR_UNEXPECTED, node->left->token, "Expected BOOL, got '%s'", TypeTranslation(node->left->data_type));
      }

      // Check right node individually for incorrect type
      if (TypeIs_Int(node->right->data_type) &&
          TypeIs_Float(node->right->data_type)) {
        ERROR_FMT(ERR_UNEXPECTED, node->right->token, "Expected BOOL, got '%s'", TypeTranslation(node->right->data_type));
      }
    } /* Intentional fallthrough */
    case LOGICAL_NOT_EQUALS:
    case EQUALITY: {
      // If left node is signed, check if right node can be converted
      if (TypeIs_Int(node->left->data_type) &&
          TypeIs_Uint(node->right->data_type))
      {
        if (!TypeIsConvertible(node->right, node->left->data_type)) {
          ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->right->token, "Can't convert from %s to %s", TypeTranslation(node->right->data_type), TypeTranslation(node->left->data_type));
        }
      }

      // If right node is signed, check if left node can be converted
      if (TypeIs_Uint(node->left->data_type) &&
          TypeIs_Int(node->right->data_type))
      {
        if (!TypeIsConvertible(node->left, node->right->data_type)) {
          ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->token, "Can't convert from type %s to %s",  TypeTranslation(node->right->data_type), TypeTranslation(node->left->data_type));
        }
      }
    } break;

    case LOGICAL_AND:
    case LOGICAL_OR: {
      if (!TypeIs_Bool(node->left->data_type)) {
        ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->token, "Expected BOOL, got '%s'", TypeTranslation(node->left->data_type));
      }

      if (!TypeIs_Bool(node->right->data_type)) {
        ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->right->token, "Expected BOOL, got '%s'", TypeTranslation(node->right->data_type));
      }
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
  if (!TypeIs_Uint(left_value->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, left_value->token, "Expected UINT, got '%s'", TypeTranslation(left_value->data_type));
  }

  if (node->token.type == BITWISE_LEFT_SHIFT ||
      node->token.type == BITWISE_RIGHT_SHIFT) {
    if (!TypeIs_Int(right_value->data_type) && !TypeIs_Uint(right_value->data_type)) {
      ERROR(ERR_TYPE_DISAGREEMENT, right_value->token);
    }

    return;
  }

  // Check right node individually for incorrect type
  if (!TypeIs_Uint(right_value->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, right_value->token, "Expected UINT, got '%s'", TypeTranslation(right_value->data_type));
  }
}

static void EnumListRecurse(AST_Node *node) {
  AST_Node *list_entry = (node)->left;

  if (list_entry == NULL) return;

  if (NodeIs_EnumAssignment(list_entry)) {
    CheckTypesRecurse(list_entry);
    AST_Node *value = (list_entry)->left;
    if ((!TypeIs_Int(value->data_type) && !TypeIs_Uint(value->data_type)) ||
        NodeIs_Identifier(value))
    {
      ERROR_MSG(ERR_IMPROPER_ASSIGNMENT, value->token, "Assignment to enum identifier must be of type INT");
    }
  }

  EnumListRecurse(node->right);
}

static void HandleEnum(AST_Node *node) {
  EnumListRecurse(node);
}

static void StructMemberAccess(AST_Node *struct_identifier) {
  if (struct_identifier->left == NULL) {
    return;
  }
  AST_Node *member_node = struct_identifier->left;

  if (member_node == NULL || NodeIs_StructMember(member_node)) return;

  Symbol struct_symbol = RetrieveFrom(SYMBOL_TABLE, struct_identifier->token);
  StructMember *member = GetStructMember(struct_symbol.data_type, member_node->token);

  SetNodeDataType(struct_identifier, member->type);
}

static void PrefixIncOrDec(AST_Node *node) {
  AST_Node *check_value = node->left;
  if (check_value == NULL) return;
  if (!TypeIs_Int(check_value->data_type) &&
      !TypeIs_Uint(check_value->data_type)) {
    ERROR(ERR_TYPE_DISAGREEMENT, check_value->token);
  }

  SetNodeDataType(node, node->left->data_type);
}

static void PostfixIncOrDec(AST_Node *node) {
  if (!TypeIs_Int(node->data_type) &&
      !TypeIs_Uint(node->data_type)) {
    ERROR(ERR_TYPE_DISAGREEMENT, node->token);
  }
}

static void IfStmt(AST_Node *node) {
  if (!TypeIs_Bool(node->left->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->token, "Predicate must be boolean, got type '%s' instead", TypeTranslation(node->left->data_type));
  }
}

static void TernaryIfStmt(AST_Node *node) {
  if (!TypeIs_Bool(node->left->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->token, "Predicate must be boolean, got type '%s' instead", TypeTranslation(node->left->data_type));
  }

  if (!TypesMatchExactly(node->middle->data_type, node->right->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->right->token, "Branches have different types (If true: '%s', if false: '%s')", TypeTranslation(node->middle->data_type), TypeTranslation(node->right->data_type));
  };

  SetNodeDataType(node, node->middle->data_type);

  PrintNode(node);
}

static void WhileStmt(AST_Node *node) {
  if (!TypeIs_Bool(node->left->data_type)) {
    ERROR_FMT(ERR_TYPE_DISAGREEMENT, node->left->token, "Predicate must be boolean, got type '%s' instead", TypeTranslation(node->left->data_type));
  }
}

static void CheckTypesRecurse(AST_Node *node) {
  if (NodeIs_EnumIdentifier(node)) {
    HandleEnum(node);
    return;
  }

  if (NodeIs_Function(node)) {
    in_function = &node->data_type;
  }

  if (node->left   != NULL) CheckTypesRecurse(node->left);
  if (node->middle != NULL) CheckTypesRecurse(node->middle);
  if (node->right  != NULL) CheckTypesRecurse(node->right);

  if (NodeIs_Function(node)) {
    in_function = NULL;
  }

  switch(node->node_type) {
    case IDENTIFIER_NODE: {
      Identifier(node);
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
    case TERSE_ASSIGNMENT_NODE:
    case ASSIGNMENT_NODE: {
      Assignment(node);
    } break;
    case FUNCTION_NODE: {
      Function(node);
    } break;
    case FUNCTION_ARGUMENT_NODE:
    case FUNCTION_PARAM_NODE:
    case FUNCTION_RETURN_TYPE_NODE: {
      SetNodeDataType(node, node->data_type);
    } break;
    case FUNCTION_CALL_NODE: {
      FunctionCall(node);
    } break;
    case IF_NODE: {
      IfStmt(node);
    } break;
    case TERNARY_IF_NODE: {
      TernaryIfStmt(node);
    } break;
    case WHILE_NODE: {
      WhileStmt(node);
    } break;
    case RETURN_NODE: {
      Return(node);
    } break;
    case STRUCT_IDENTIFIER_NODE: {
      StructMemberAccess(node);
    } break;
    case PREFIX_INCREMENT_NODE:
    case PREFIX_DECREMENT_NODE: {
      PrefixIncOrDec(node);
    } break;
    case POSTFIX_INCREMENT_NODE:
    case POSTFIX_DECREMENT_NODE: {
      PostfixIncOrDec(node);
    } break;
    case LITERAL_NODE: {
      Literal(node);
    } break;
    case ARRAY_SUBSCRIPT_NODE:
    case STRUCT_DECLARATION_NODE:
    case DECLARATION_NODE:
    case PRINT_CALL_NODE:
    default: {
      // Use declared type, no action required
    } break;
  }
}

void CheckTypes(AST_Node *node, SymbolTable *symbol_table) {
  SYMBOL_TABLE = symbol_table;
  CheckTypesRecurse(node);
}
