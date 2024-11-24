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
    if (MIDDLE_NODE(n) != NULL && MIDDLE_NODE(n)->type == ARRAY_SUBSCRIPT_NODE) {
      // Extract char from a "str[i]"-type thing
      int64_t index = TokenToInt64(n->nodes[MIDDLE]->token, 10);
      n->value = NewCharValue(stored_symbol.value.as.string[index]);
    } else {
      n->value = stored_symbol.value;
    }
  } else if (stored_symbol.annotation.is_array) {
    int subscript = TokenToInt64(MIDDLE_NODE(n)->token, 10);
    n->value = stored_symbol.value.as.array[subscript];
  } else {
    n->value = stored_symbol.value;
  }
}

Value ArrayInitializerList(AST_Node *n) {
  AST_Node **current = &LEFT_NODE(n);
  Value *data = calloc(n->annotation.array_size, sizeof(Value));
  int i = 0;

  do {
    if (LEFT_NODE(*current)->type == IDENTIFIER_NODE) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE(), LEFT_NODE(*current)->token);
      data[i] = s.value;
    } else {
      data[i] = NewValue(LEFT_NODE(*current)->annotation,
                         LEFT_NODE(*current)->token);
    }

    i++;
    current = &RIGHT_NODE(*current);
  } while (*current != NULL && LEFT_NODE(*current) != NULL);

  return (Value){
    .type = V_ARRAY,
    .array_type = n->value.type,
    .array_size = i,
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
    symbol.value = LEFT_NODE(n)->value;
  }

  AddTo(SYMBOL_TABLE(), symbol);
  n->value = symbol.value;
}

void Unary(AST_Node *n) {
  switch(n->token.type) {
    case LOGICAL_NOT: {
      n->value = NewBoolValue(!(LEFT_NODE(n)->value.as.boolean));
    } break;
    case MINUS: {
      if (n->annotation.actual_type == ACT_FLOAT) {
        n->value = NewFloatValue(-LEFT_NODE(n)->value.as.floating);
        break;
      }

      if (n->annotation.is_signed) {
        n->value = NewIntValue(-LEFT_NODE(n)->value.as.integer);
      } else {
        n->value = NewUintValue(-(LEFT_NODE(n)->value.as.uinteger));
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
      n->value = AddValues(LEFT_NODE(n)->value,
                           RIGHT_NODE(n)->value);
    } break;
    case MINUS: {
      n->value = SubValues(LEFT_NODE(n)->value,
                           RIGHT_NODE(n)->value);
    } break;
    case ASTERISK: {
      n->value = MulValues(LEFT_NODE(n)->value,
                           RIGHT_NODE(n)->value);
    } break;
    case DIVIDE: {
      n->value = DivValues(LEFT_NODE(n)->value,
                           RIGHT_NODE(n)->value);
    } break;
    case MODULO: {
      n->value = ModValues(LEFT_NODE(n)->value,
                           RIGHT_NODE(n)->value);
    } break;
    default: {
      printf("BinaryArithmetic(): Not implemented yet\n");
    } break;
  }
}

void BinaryLogical(AST_Node *n) {
  switch(n->token.type) {
    case EQUALITY: {
      n->value = Equality(LEFT_NODE(n)->value, RIGHT_NODE(n)->value);
    } break;
    case GREATER_THAN: {
      n->value = GreaterThan(LEFT_NODE(n)->value, RIGHT_NODE(n)->value);
    } break;
    case LESS_THAN: {
      n->value = LessThan(LEFT_NODE(n)->value, RIGHT_NODE(n)->value);
    } break;
    case GREATER_THAN_EQUALS: {
      n->value = Not(GreaterThan(RIGHT_NODE(n)->value, LEFT_NODE(n)->value));
    } break;
    case LESS_THAN_EQUALS: {
      n->value = Not(LessThan(RIGHT_NODE(n)->value, LEFT_NODE(n)->value));
    } break;
    case LOGICAL_NOT_EQUALS: {
      n->value = Not(Equality(LEFT_NODE(n)->value, RIGHT_NODE(n)->value));
    } break;
    case LOGICAL_AND: {
      n->value = LogicalAND(LEFT_NODE(n)->value, RIGHT_NODE(n)->value);
    } break;
    case LOGICAL_OR: {
      n->value = LogicalOR(LEFT_NODE(n)->value, RIGHT_NODE(n)->value);
    } break;
    default: {
      printf("BinaryLogical(): %s not implemented yet\n", TokenTypeTranslation(n->token.type));
    } break;
  }
}

void BinaryBitwise(AST_Node *n) {
  switch(n->token.type) {
    case BITWISE_XOR: {
      n->value = NewUintValue( LEFT_NODE(n)->value.as.uinteger ^
                              RIGHT_NODE(n)->value.as.uinteger);
    } break;
    case BITWISE_OR: {
      n->value = NewUintValue( LEFT_NODE(n)->value.as.uinteger |
                              RIGHT_NODE(n)->value.as.uinteger);
    } break;
    case BITWISE_AND: {
      n->value = NewUintValue( LEFT_NODE(n)->value.as.uinteger &
                              RIGHT_NODE(n)->value.as.uinteger);
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
  AST_Node *params = MIDDLE_NODE(fn_def);
  AST_Node *args   = MIDDLE_NODE(n);
  while (args != NULL) {
    Symbol s = AddTo(SYMBOL_TABLE(),
                     NewSymbol(params->token,
                               params->annotation,
                               DECL_DEFINED));
    s.value = NewValue(args->annotation, args->token);
    AddTo(SYMBOL_TABLE(), s);

    params = LEFT_NODE(params);
    args   = RIGHT_NODE(args);
  }

  // Evaluate function body
  if (RIGHT_NODE(fn_def) != NULL) {
    InterpretRecurse(RIGHT_NODE(fn_def));
    n->value = check_value;
  }

  EndScope();
}

void PrefixIncrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), LEFT_NODE(n)->token);
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
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), LEFT_NODE(n)->token);
  if (s.annotation.is_signed) {
    s.value.as.integer--;
  } else {
    s.value.as.uinteger--;
  }
  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), s);

  n->value = stored_symbol.value;
}

void PostfixDecrement(AST_Node *n) {
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), LEFT_NODE(n)->token);
  n->value = s.value;

  if (s.annotation.is_signed) {
    s.value.as.integer--;
  } else {
    s.value.as.uinteger--;
  }

  AddTo(SYMBOL_TABLE(), s);
}

static void InterpretRecurse(AST_Node *n) {
  if (n->type == FUNCTION_NODE) {
    function_definitions[fdi++] = n;
    return;
  }

  if (LEFT_NODE(n)   != NULL) InterpretRecurse(LEFT_NODE(n));
  if (MIDDLE_NODE(n) != NULL) InterpretRecurse(MIDDLE_NODE(n));
  if (RIGHT_NODE(n)  != NULL) InterpretRecurse(RIGHT_NODE(n));

  switch(n->type) {
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
      check_value = LEFT_NODE(n)->value;
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
