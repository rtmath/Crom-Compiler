#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // for calloc

#include "common.h"
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

  if (stored_symbol.annotation.is_array) {
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
  Symbol symbol;
  if (IsIn(SYMBOL_TABLE(), n->token)) {
    symbol = RetrieveFrom(SYMBOL_TABLE(), n->token);
  } else {
    symbol = NewSymbol(n->token, n->annotation, DECL_NONE);
  }

  if (n->annotation.is_array) {
    symbol.value = ArrayInitializerList(n);
  } else {
    symbol.value = LEFT_NODE(n)->value;
  }

  Symbol stored_symbol = AddTo(SYMBOL_TABLE(), symbol);
  PrintSymbol(stored_symbol);
  printf("\n");
  n->value = symbol.value;
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
