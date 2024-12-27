#include <errno.h>
#include <stdio.h>  // for vprintf
#include <stdlib.h> // for calloc
#include <string.h> // for strncmp

#include "common.h"
#include "error.h"
#include "interpreter.h"
#include "symbol_table.h"

/* === Global === */
static AST_Node *function_definitions[100]; // TODO: Dynamic array?
static int fdi = 0;

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
  if (Scope.depth == 0) COMPILER_ERROR("How'd you end scope at depth 0?");

  DeleteSymbolTable(Scope.locals[Scope.depth]);
  Scope.locals[Scope.depth] = NULL;
  Scope.depth--;
}
/* === End Scope Related === */

static void InterpretPrint(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

#ifdef RUNNING_TESTS
  vprintf(fmt, args);
#else
  Print_VAList(fmt, args);
#endif

  va_end(args);
}

static void Literal(AST_Node *n) {
  n->value = NewValue(n->data_type, n->token);
}

static void Identifier(AST_Node *n) {
  Symbol stored_symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);

  if (TypeIs_String(stored_symbol.value.type)) {
    if (!NodeIs_NULL(n->middle) && NodeIs_ArraySubscript(n->middle)) {
      // Extract char from a "str[i]"-type thing
      int64_t index = TokenToInt64(n->middle->token);
      n->value = NewCharValue(stored_symbol.value.as.string[index]);
    } else {
      n->value = stored_symbol.value;
    }
  } else if (TypeIs_Array(stored_symbol.value.type)) {
    int subscript = TokenToInt64(n->middle->token);
    n->value = stored_symbol.value.as.array[subscript];
  } else {
    n->value = stored_symbol.value;
  }
}

static Value ArrayInitializerList(AST_Node *n) {
  AST_Node **current = &n->left;
  Value *data = calloc(n->value.type.array_size, sizeof(Value));
  int i = 0;

  while (*current != NULL && (*current)->left != NULL) {
    if (NodeIs_Identifier((*current)->left)) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE(), (*current)->left->token);
      data[i] = s.value;
    } else {
      data[i] = NewValue((*current)->left->value.type,
                         (*current)->left->token);
    }

    i++;
    current = &(*current)->right;
  }

  return (Value) {
    .type = (Type) {
      .category = TC_ARRAY,
      .specifier = n->value.type.specifier,
      .array_size = i,
    },

    .as.array = data,
  };
}

static void Assignment(AST_Node *n) {
  Symbol symbol = {0};
  if (IsIn(SYMBOL_TABLE(), n->token)) {
    symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);
  } else {
    symbol = NewSymbol(n->token, n->value.type, DECL_NONE);
  }

  if (TypeIs_Array(n->value.type) && !TypeIs_String(n->value.type)) {
    symbol.value = ArrayInitializerList(n);
  } else if (false /* TODO: Array subscripting */) {

  } else {
    symbol.value = n->left->value;
  }

  Symbol updated_symbol = AddTo(SYMBOL_TABLE(), symbol);
  n->value = updated_symbol.value;
}

static void TerseAssignment(AST_Node *n) {
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

    default: Print("TerseAssignment(): Not implemented yet\n");
  }

  SetSymbolValue(SYMBOL_TABLE(), identifier->token, n->value);
}

static void Unary(AST_Node *n) {
  Type type = n->left->value.type;
  Value value = n->left->value;

  switch(n->token.type) {
    case BITWISE_NOT: {
      uint64_t uint = value.as.uinteger;

      if (TypeIs_U8(type)) {
          uint8_t truncated = (~uint);
          n->value = NewUintValue(truncated);
      } else if (TypeIs_U16(type)) {
          uint16_t truncated = (~uint);
          n->value = NewUintValue(truncated);
      } else if (TypeIs_U32(type)) {
          uint32_t truncated = (~uint);
          n->value = NewUintValue(truncated);
      } else if (TypeIs_U64(type)) {
          uint64_t truncated = (~uint);
          n->value = NewUintValue(truncated);
      }
    } break;
    case LOGICAL_NOT: {
      n->value = NewBoolValue(!(value.as.boolean));
    } break;
    case MINUS: {
      if (TypeIs_Float(type)) {
        n->value = NewFloatValue(-(value.as.floating));
        break;
      }

      if (TypeIs_Int(type)) {
        n->value = NewIntValue(-(value.as.integer));
      } else if (TypeIs_Uint(type)) {
        n->value = NewUintValue(-(value.as.uinteger));
      }
    } break;
    default: {
      Print("Unary(): Not implemented yet\n");
    } break;
  }
}

static void BinaryArithmetic(AST_Node *n) {
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
      Print("BinaryArithmetic(): Not implemented yet\n");
    } break;
  }
}

static void BinaryLogical(AST_Node *n) {
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
      Print("BinaryLogical(): %s not implemented yet\n", TokenTypeTranslation(n->token.type));
    } break;
  }
}

static void BinaryBitwise(AST_Node *n) {
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
      Print("BinaryBitwise(): Not implemented yet\n");
    } break;
  }
}

static void FunctionCall(AST_Node *n) {
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
    INTERPRETER_ERROR_FMTMSG("FunctionCall(): Couldn't find function definition for %.*s()", n->token.length, n->token.position_in_source);
  }

  BeginScope();

  // Create variables for all args
  AST_Node *params = fn_def->middle;
  AST_Node *args   = n->middle;
  while (args != NULL) {
    Symbol s = AddTo(SYMBOL_TABLE(),
                     NewSymbol(params->token,
                               params->value.type,
                               DECL_DEFINED));
    s.value = NewValue(args->value.type, args->token);
    AddTo(SYMBOL_TABLE(), s);

    params = params->left;
    args   = args->right;
  }

  // Evaluate function body
  if (fn_def->right != NULL) {
    InterpretRecurse(fn_def->right);
  }

  EndScope();
}

static void StructDeclaration(AST_Node *struct_identifier) {
  AST_Node **current = &struct_identifier->left;
  while (*current != NULL) {
    if ((*current)->left == NULL) {
      current = &(*current)->right;
      continue;
    }

    Literal((*current)->left);

    /* TODO: Need to store Struct Values somewhere
    SetStructValue(SYMBOL_TABLE(),
                   struct_identifier->token,
                   (*current)->token,
                   (*current)->left->value);
    */
    current = &(*current)->right;
  }
}

static void StructMemberAccess(AST_Node *struct_identifier) {
  if (struct_identifier->right == NULL) {
    if (struct_identifier->left == NULL) {
      return;
    }
    return;
  }

  // TODO: Need to store Struct Values somewhere
  //struct_identifier->value = GetStructValue(SYMBOL_TABLE(), struct_identifier->right->token, struct_identifier->token);
}

static void PrefixIncrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->left->token);
  if (TypeIs_Int(s.value.type)) {
    s.value.as.integer++;
  } else {
    s.value.as.uinteger++;
  }
  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), s);

  n->value = stored_symbol.value;
}

static void PostfixIncrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->token);
  n->value = s.value;

  if (TypeIs_Int(s.value.type)) {
    s.value.as.integer++;
  } else {
    s.value.as.uinteger++;
  }

  AddTo(SYMBOL_TABLE(), s);
}

static void PrefixDecrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->left->token);
  if (TypeIs_Int(s.value.type)) {
    s.value.as.integer--;
  } else {
    s.value.as.uinteger--;
  }
  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), s);

  n->value = stored_symbol.value;
}

static void PostfixDecrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), n->token);
  n->value = s.value;

  if (TypeIs_Int(s.value.type)) {
    s.value.as.integer--;
  } else {
    s.value.as.uinteger--;
  }

  AddTo(SYMBOL_TABLE(), s);
}

static void PrintCall(AST_Node *n) {
  Type type = n->left->value.type;
  Value value = n->left->value;

  if (TypeIs_Int(type)) {
    InterpretPrint("%ld\n", value.as.integer);
    return;
  }

  if (TypeIs_Uint(type)) {
    InterpretPrint("%lu\n", value.as.uinteger);
    return;
  }

  if (TypeIs_Float(type)) {
    InterpretPrint("%f\n", value.as.floating);
    return;
  }

  if (TypeIs_Char(type)) {
    InterpretPrint("%c\n", value.as.character);
    return;
  }

  if (TypeIs_String(type)) {
    InterpretPrint("%s\n", value.as.string);
    return;
  }

  if (TypeIs_Bool(type)) {
    InterpretPrint("%s\n", (value.as.boolean) ? "true" : "false");
    return;
  }

  if (TypeIs_Enum(type)) {
    InterpretPrint("%d\n", value.as.integer);
    return;
  }

  InterpretPrint("PrintCall(): Not implemented yet\n");
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
    case ENUM_ASSIGNMENT_NODE:
    case ASSIGNMENT_NODE: {
      Assignment(n);
    } break;
    case TERSE_ASSIGNMENT_NODE: {
      TerseAssignment(n);
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
    case PRINT_CALL_NODE: {
      PrintCall(n);
    } break;
    case RETURN_NODE: {
    } break;
    default: break;
  }
}

void Interpret(AST_Node *root, SymbolTable *st) {
  Scope.locals[Scope.depth] = st;

  InterpretRecurse(root);
}
