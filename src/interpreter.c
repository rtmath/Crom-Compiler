#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // for calloc
#include <string.h> // for strncmp

#include "common.h"
#include "error.h"
#include "interpreter.h"
#include "symbol_table.h"

/* === Global === */
static AST_Node *function_definitions[100]; // TODO: Dynamic array?
static int fdi = 0;

// check_value's purpose is to hoist a value out of the AST for unit tests
static Value check_value;

/* === Forward Declarations === */
static void InterpretRecurse(AST_Node *n);

/* === Scope Related === */
SymbolTable *shadowed_symbol_table;

static struct {
  int depth;
  SymbolTable *locals[10]; // TODO: figure out actual size or make dynamic array
} Scope;

static SymbolTable *SYMBOL_TABLE() {
  return (shadowed_symbol_table != NULL)
           ? shadowed_symbol_table
           : Scope.locals[Scope.depth];
}

static void BeginScope() {
  Scope.depth++;
  Scope.locals[Scope.depth] = NewSymbolTable();
}

static void EndScope() {
  if (Scope.depth == 0) ERROR_AND_EXIT("How'd you end scope at depth 0?");

  DeleteSymbolTable(Scope.locals[Scope.depth]);
  Scope.locals[Scope.depth] = NULL;
  Scope.depth--;
}
/* === End Scope Related === */

void Literal(AST_Node *n) {
  n->value = NewValue(n->annotation, n->token);
}

void Identifier(AST_Node *n) {
  Symbol stored_symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);

  if (stored_symbol.annotation.actual_type == ACT_STRING) {
    if (!NodeIs_NULL(n->middle) && NodeIs_ArraySubscript(n->middle)) {
      // Extract char from a "str[i]"-type thing
      int64_t index = TokenToInt64(n->middle->token, 10);
      n->value = NewCharValue(stored_symbol.value.as.string[index]);
    } else {
      n->value = stored_symbol.value;
    }
  } else if (stored_symbol.annotation.is_array) {
    int subscript = TokenToInt64(n->middle->token, 10);
    n->value = stored_symbol.value.as.array[subscript];
  } else {
    n->value = stored_symbol.value;
  }
}

Value ArrayInitializerList(AST_Node *n) {
  AST_Node **current = &n->left;
  Value *data = calloc(n->annotation.array_size, sizeof(Value));
  int i = 0;

  do {
    if (NodeIs_Identifier((*current)->left)) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE(), (*current)->left->token);
      data[i] = s.value;
    } else {
      data[i] = NewValue((*current)->left->annotation,
                         (*current)->left->token);
    }

    i++;
    current = &(*current)->right;
  } while (*current != NULL && (*current)->left != NULL);

  return (Value) {
    .type = (Type) {
      .category = TC_ARRAY,
      .specifier = n->value.type.specifier,
      .array_size = i,
    },

    .as.array = data,
  };
}

void Assignment(AST_Node *n) {
  Symbol symbol = {0};
  if (IsIn(SYMBOL_TABLE(), n->token)) {
    symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);
  } else {
    symbol = NewSymbol(n->token, n->annotation, DECL_NONE);
  }

  if (n->annotation.is_array && n->annotation.actual_type != ACT_STRING) {
    symbol.value = ArrayInitializerList(n);
  } else if (false /* TODO: Array subscripting */) {

  } else {
    symbol.value = n->left->value;
  }

  AddTo(SYMBOL_TABLE(), symbol);
  n->value = symbol.value;
}

void TerseAssignment(AST_Node *n) {
  AST_Node *identifier = n->left;
  AST_Node *value = n->right;

  switch(n->token.type) {
    case PLUS_EQUALS: {
      n->value = AddValues(identifier->value, value->value);
    } break;
    case MINUS_EQUALS: {
      n->value = SubValues(identifier->value, value->value);
    } break;
    case TIMES_EQUALS: {
      n->value = MulValues(identifier->value, value->value);
    } break;
    case DIVIDE_EQUALS: {
      n->value = DivValues(identifier->value, value->value);
    } break;
    case MODULO_EQUALS: {
      n->value = ModValues(identifier->value, value->value);
    } break;

    case BITWISE_OR_EQUALS: {
      n->value = NewUintValue(identifier->value.as.uinteger | value->value.as.uinteger);
    } break;
    case BITWISE_AND_EQUALS: {
      n->value = NewUintValue(identifier->value.as.uinteger & value->value.as.uinteger);
    } break;
    case BITWISE_XOR_EQUALS: {
      n->value = NewUintValue(identifier->value.as.uinteger ^ value->value.as.uinteger);
    } break;

    default: printf("TerseAssignment(): Not implemented yet\n");
  }

  SetValue(SYMBOL_TABLE(), identifier->token, n->value);
}

void Unary(AST_Node *n) {
  switch(n->token.type) {
    case BITWISE_NOT: {
      uint64_t value = n->left->value.as.uinteger;

      switch(n->left->annotation.bit_width) {
        case 8: {
          uint8_t truncated = (~value);
          n->value = NewUintValue(truncated);
        } break;
        case 16: {
          uint16_t truncated = (~value);
          n->value = NewUintValue(truncated);
        } break;
        case 32: {
          uint32_t truncated = (~value);
          n->value = NewUintValue(truncated);
        } break;
        case 64: {
          uint64_t truncated = (~value);
          n->value = NewUintValue(truncated);
        } break;
        default: return;
      }

    } break;
    case LOGICAL_NOT: {
      n->value = NewBoolValue(!(n->left->value.as.boolean));
    } break;
    case MINUS: {
      if (n->annotation.actual_type == ACT_FLOAT) {
        n->value = NewFloatValue(-(n->left->value.as.floating));
        break;
      }

      if (n->annotation.is_signed) {
        n->value = NewIntValue(-(n->left->value.as.integer));
      } else {
        n->value = NewUintValue(-(n->left->value.as.uinteger));
      }
    } break;
    default: {
      printf("Unary(): Not implemented yet\n");
    } break;
  }
}

void BinaryArithmetic(AST_Node *n) {
  switch(n->token.type) {
    case PLUS: {
      n->value = AddValues(n->left->value,
                           n->right->value);
    } break;
    case MINUS: {
      n->value = SubValues(n->left->value,
                           n->right->value);
    } break;
    case ASTERISK: {
      n->value = MulValues(n->left->value,
                           n->right->value);
    } break;
    case DIVIDE: {
      n->value = DivValues(n->left->value,
                           n->right->value);
    } break;
    case MODULO: {
      n->value = ModValues(n->left->value,
                           n->right->value);
    } break;
    default: {
      printf("BinaryArithmetic(): Not implemented yet\n");
    } break;
  }
}

void BinaryLogical(AST_Node *n) {
  switch(n->token.type) {
    case EQUALITY: {
      n->value = Equality(n->left->value, n->right->value);
    } break;
    case GREATER_THAN: {
      n->value = GreaterThan(n->left->value, n->right->value);
    } break;
    case LESS_THAN: {
      n->value = LessThan(n->left->value, n->right->value);
    } break;
    case GREATER_THAN_EQUALS: {
      n->value = Not(GreaterThan(n->right->value, n->left->value));
    } break;
    case LESS_THAN_EQUALS: {
      n->value = Not(LessThan(n->right->value, n->left->value));
    } break;
    case LOGICAL_NOT_EQUALS: {
      n->value = Not(Equality(n->left->value, n->right->value));
    } break;
    case LOGICAL_AND: {
      n->value = LogicalAND(n->left->value, n->right->value);
    } break;
    case LOGICAL_OR: {
      n->value = LogicalOR(n->left->value, n->right->value);
    } break;
    default: {
      printf("BinaryLogical(): %s not implemented yet\n", TokenTypeTranslation(n->token.type));
    } break;
  }
}

void BinaryBitwise(AST_Node *n) {
  switch(n->token.type) {
    case BITWISE_XOR: {
      n->value = NewUintValue(n->left->value.as.uinteger ^
                              n->right->value.as.uinteger);
    } break;
    case BITWISE_OR: {
      n->value = NewUintValue(n->left->value.as.uinteger |
                              n->right->value.as.uinteger);
    } break;
    case BITWISE_AND: {
      n->value = NewUintValue(n->left->value.as.uinteger &
                              n->right->value.as.uinteger);
    } break;
    case BITWISE_LEFT_SHIFT: {
      n->value = NewUintValue(n->left->value.as.uinteger <<
                              n->right->value.as.uinteger);
    } break;
    case BITWISE_RIGHT_SHIFT: {
      n->value = NewUintValue(n->left->value.as.uinteger >>
                              n->right->value.as.uinteger);
    } break;
    default: {
      printf("BinaryBitwise(): Not implemented yet\n");
    } break;
  }
}

void FunctionCall(AST_Node *n) {
  // Linear search for function definition (TODO: Hashtable)
  AST_Node *fn_def = NULL;
  for (int i = 0; i < fdi; i++) {
    if (strncmp(n->token.position_in_source,
                function_definitions[i]->token.position_in_source,
                n->token.length) == 0)
    {
      fn_def = function_definitions[i];
      break;
    }
  }

  if (fn_def == NULL) {
    ERROR_AND_EXIT_FMTMSG(
      "FunctionCall(): Couldn't find function definition for %.*s()",
      n->token.length, n->token.position_in_source);
  }

  BeginScope();

  // Create variables for all args
  AST_Node *params = fn_def->middle;
  AST_Node *args   = n->middle;
  while (args != NULL) {
    Symbol s = AddTo(SYMBOL_TABLE(),
                     NewSymbol(params->token,
                               params->annotation,
                               DECL_DEFINED));
    s.value = NewValue(args->annotation, args->token);
    AddTo(SYMBOL_TABLE(), s);

    params = params->left;
    args   = args->right;
  }

  // Evaluate function body
  if (fn_def->right != NULL) {
    InterpretRecurse(fn_def->right);
    n->value = check_value;
  }

  EndScope();
}

void StructDeclaration(AST_Node *struct_identifier) {
  AST_Node **current = &struct_identifier->left;
  while (*current != NULL) {
    if ((*current)->left == NULL) {
      current = &(*current)->right;
      continue;
    }

    Literal((*current)->left);

    // TODO: Make a helper function for storing struct members
    Symbol parent_struct = RetrieveFrom(SYMBOL_TABLE(), struct_identifier->token);
    Symbol struct_member = RetrieveFrom(parent_struct.struct_fields, (*current)->token);
    struct_member.value = (*current)->left->value;
    AddTo(parent_struct.struct_fields, struct_member);

    current = &(*current)->right;
  }
}

void StructMemberAccess(AST_Node *struct_identifier) {
  if (struct_identifier->right == NULL) {
    if (struct_identifier->left == NULL) {
      return;
    }
    return;
  }

  Symbol parent_struct = RetrieveFrom(SYMBOL_TABLE(), struct_identifier->right->token);
  Symbol struct_member = RetrieveFrom(parent_struct.struct_fields, struct_identifier->token);
  struct_identifier->value = struct_member.value;
}

void PrefixIncrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->left->token);
  if (s.annotation.is_signed) {
    s.value.as.integer++;
  } else {
    s.value.as.uinteger++;
  }
  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), s);

  n->value = stored_symbol.value;
}

void PostfixIncrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->token);
  n->value = s.value;

  if (s.annotation.is_signed) {
    s.value.as.integer++;
  } else {
    s.value.as.uinteger++;
  }

  AddTo(SYMBOL_TABLE(), s);
}

void PrefixDecrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->left->token);
  if (s.annotation.is_signed) {
    s.value.as.integer--;
  } else {
    s.value.as.uinteger--;
  }
  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), s);

  n->value = stored_symbol.value;
}

void PostfixDecrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->left->token);
  n->value = s.value;

  if (s.annotation.is_signed) {
    s.value.as.integer--;
  } else {
    s.value.as.uinteger--;
  }

  AddTo(SYMBOL_TABLE(), s);
}

static void InterpretRecurse(AST_Node *n) {
  if (NodeIs_Function(n)) {
    function_definitions[fdi++] = n;
    return;
  }

  if (!NodeIs_NULL(n->left))   InterpretRecurse(n->left);
  if (!NodeIs_NULL(n->middle)) InterpretRecurse(n->middle);
  if (!NodeIs_NULL(n->right))  InterpretRecurse(n->right);

  switch(n->node_type) {
    case STRUCT_DECLARATION_NODE: {
      StructDeclaration(n);
    } break;
    case STRUCT_MEMBER_IDENTIFIER_NODE: {
      StructMemberAccess(n);
      check_value = n->value;
    } break;
    case UNARY_OP_NODE: {
      Unary(n);
    } break;
    case BINARY_ARITHMETIC_NODE: {
      BinaryArithmetic(n);
    } break;
    case BINARY_LOGICAL_NODE: {
      BinaryLogical(n);
    } break;
    case BINARY_BITWISE_NODE: {
      BinaryBitwise(n);
    } break;
    case ASSIGNMENT_NODE: {
      Assignment(n);
      check_value = n->value;
    } break;
    case TERSE_ASSIGNMENT_NODE: {
      TerseAssignment(n);
      check_value = n->value;
    } break;
    case LITERAL_NODE: {
      Literal(n);
    } break;
    case IDENTIFIER_NODE: {
      Identifier(n);
    } break;
    case PREFIX_INCREMENT_NODE: {
      PrefixIncrement(n);
    } break;
    case PREFIX_DECREMENT_NODE: {
      PrefixDecrement(n);
    } break;
    case POSTFIX_INCREMENT_NODE: {
      PostfixIncrement(n);
    } break;
    case POSTFIX_DECREMENT_NODE: {
      PostfixDecrement(n);
    } break;
    case FUNCTION_CALL_NODE: {
      FunctionCall(n);
    } break;
    case RETURN_NODE: {
      check_value = n->left->value;
    } break;
    default: break;
  }
}

void Interpret(AST_Node *root, SymbolTable *st) {
  Scope.locals[Scope.depth] = st;

  InterpretRecurse(root);
  if (root->error_code == ERR_UNSET) {
    root->error_code = OK;
  }

  root->value = check_value;
}
