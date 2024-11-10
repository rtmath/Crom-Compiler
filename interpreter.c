#include <stdio.h>

#include "error.h"
#include "interpreter.h"
#include "symbol_table.h"

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
/* === End Scope Related === */

void Literal(AST_Node *n) {
  n->value = NewValue(n->annotation, n->token);
}

void Identifier(AST_Node *n) {
  Symbol stored_symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);
  n->value = stored_symbol.value;
}

void Assignment(AST_Node *n) {
  Symbol symbol;
  if (IsIn(SYMBOL_TABLE(), n->token)) {
    symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);
  } else {
    symbol = NewSymbol(n->token, n->annotation, DECL_NONE);
  }
  symbol.value = LEFT_NODE(n)->value;

  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), symbol);
  PrintSymbol(stored_symbol);
  printf("\n");
  n->value = symbol.value;
}

Value AddValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("AddValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer + v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger + v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating + v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("AddValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value SubValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("SubValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer - v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger - v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating - v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("SubValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value MulValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("MulValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer * v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger * v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating * v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("MulValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value DivValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("DivValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer / v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger / v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating / v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("DivValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value ModValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("ModValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer % v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger % v2.as.uinteger
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("ModValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

void Binary(AST_Node *n) {
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
      printf("Assignment(): Not implemented yet\n");
    } break;
  }
}

void InterpretRecurse(AST_Node *n) {
  if (LEFT_NODE(n)   != NULL) InterpretRecurse(LEFT_NODE(n));
  if (MIDDLE_NODE(n) != NULL) InterpretRecurse(MIDDLE_NODE(n));
  if (RIGHT_NODE(n)  != NULL) InterpretRecurse(RIGHT_NODE(n));

  switch(n->type) {
    case BINARY_OP_NODE: {
      Binary(n);
    } break;
    case ASSIGNMENT_NODE: {
      Assignment(n);
    } break;
    case LITERAL_NODE: {
      Literal(n);
    } break;
    case IDENTIFIER_NODE: {
      Identifier(n);
    } break;

    default: break;
  }
}

void Interpret(AST_Node *root) {
  Scope.locals[Scope.depth] = NewSymbolTable();

  printf("\n");
  InterpretRecurse(root);
}
