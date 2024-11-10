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
  symbol.value  = LEFT_NODE(n)->value;

  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), symbol);
  PrintSymbol(stored_symbol);
  printf("\n");
  n->value = symbol.value;
}

void Binary(AST_Node *n) {
  switch(n->token.type) {
    case PLUS: {
      n->value = (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = LEFT_NODE(n)->value.as.integer +
                      RIGHT_NODE(n)->value.as.integer
      };
    } break;
    default: {
      printf("Not implemented yet\n");
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
