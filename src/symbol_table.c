#include <stdlib.h>

#include "common.h"
#include "error.h"
#include "symbol_table.h"

#include <stdio.h>

static int symbol_guid = 0;
static Symbol NOT_FOUND = {
  .symbol_id = -1,
  .declaration_state = DECL_NONE,
  .token = {
    .type = ERROR,
    .position_in_source = "No symbol found in Symbol Table",
    .length = 31,
    .on_line = -1,
  },
  .value = {
    .type = {0},
    .as.uinteger = 0,
  },
};

USE_DYNAMIC_ARRAY(Symbol)

struct SymbolTable {
  int count;
  DA(Symbol) symbols;
};

SymbolTable *NewSymbolTable() {
  SymbolTable *st = calloc(sizeof(SymbolTable), 1);
  DA_INIT(Symbol, st->symbols);

  return st;
}

void DeleteSymbolTable(SymbolTable *st) {
  DA_FREE(Symbol, st->symbols);
  free(st);
}

Symbol NewSymbol(Token token, Type type, enum DeclarationState d) {
  Symbol s = {
    .symbol_id = -1,
    .declaration_state = d,
    .token = token,
    .value = (Value){
      .type = type,
      .as.uinteger = 0,
    },
  };

  return s;
}

static Symbol GetSymbol(SymbolTable *st, int symbol_id) {
  if (symbol_id < 0 || symbol_id >= st->count) return NOT_FOUND;
  return DA_GET(st->symbols, symbol_id);
}

static Symbol SetSymbol(SymbolTable *st, Symbol s) {
  DA_SET(Symbol, st->symbols, s.symbol_id, s);
  return DA_GET(st->symbols, s.symbol_id);
}

static Symbol AddSymbol(SymbolTable *st, Symbol s) {
  s.declared_on_line = s.token.on_line;
  DA_ADD(Symbol, st->symbols, s);
  return DA_GET(st->symbols, st->symbols.count - 1);
}

Symbol AddTo(SymbolTable *st, Symbol s) {
  if (s.token.type == ERROR) ERROR_AND_EXIT("Tried adding an ERROR token to Symbol Table");

  Symbol existing_symbol = RetrieveFrom(st, s.token);
  if (existing_symbol.token.type != ERROR) {
    existing_symbol.declaration_state = s.declaration_state;
    existing_symbol.value.type = s.value.type;
    existing_symbol.token = s.token;

    Symbol updated_symbol = SetSymbol(st, existing_symbol);
    return updated_symbol;
  }

  s.symbol_id = symbol_guid++;
  Symbol stored_symbol = AddSymbol(st, s);
  st->count++;

  return stored_symbol;
}

Symbol RetrieveFrom(SymbolTable *st, Token t) {
  for (int i = 0; i < st->count; i++) {
    Symbol check = GetSymbol(st, i);
    if (TokenValuesMatch(check.token, t)) {
      return check;
    }
  }

  return NOT_FOUND;
}

bool IsIn(SymbolTable *st, Token t) {
  for (int i = 0; i < st->count; i++) {
    Symbol check = GetSymbol(st, i);
    if (TokenValuesMatch(check.token, t)) {
      return true;
    }
  }

  return false;
}

void AddParams(SymbolTable *st, Symbol function_symbol) {
  FnParam *next = function_symbol.value.type.params.next;

  while (next != NULL) {
    AddTo(st, NewSymbol(next->token, next->type, DECL_DEFINED));

    next = next->next;
  }
}

Symbol SetDecl(SymbolTable *st, Token t, enum DeclarationState ds) {
  Symbol s = RetrieveFrom(st, t);
  if (s.token.type == ERROR) {
    Print("SetDecl(): Token '%.*s' not found in symbol table", t.length, t.position_in_source);
    return NOT_FOUND;
  }

  s.declaration_state = ds;
  return AddTo(st, s);
}

Symbol SetValue(SymbolTable *st, Token t, Value v) {
  Symbol s = RetrieveFrom(st, t);
  if (s.token.type == ERROR) {
    Print("SetValue(): Token '%.*s' not found in symbol table", t.length, t.position_in_source);
    return NOT_FOUND;
  }

  s.value = v;
  return AddTo(st, s);
}

Symbol SetValueType(SymbolTable *st, Token t, Type type) {
  Symbol s = RetrieveFrom(st, t);
  if (s.token.type == ERROR) {
    Print("SetValueType(): Token '%.*s' not found in symbol table", t.length, t.position_in_source);
    return NOT_FOUND;
  }

  s.value.type = type;
  return AddTo(st, s);
}

static const char* const _DeclarationStateTranslation[] =
{
  [DECL_NONE] = "None",
  [DECL_UNINITIALIZED] = "UNINITIALIZED",
  [DECL_DECLARED] = "DECLARED",
  [DECL_DEFINED] = "DEFINED",
};

static const char *DeclarationStateTranslation(enum DeclarationState ds) {
  if (ds < 0 || ds >= DECL_ENUM_COUNT) {
    ERROR_AND_EXIT_FMTMSG("DeclarationStateTranslation(): '%d' out of range", ds);
  }
  return _DeclarationStateTranslation[ds];
}

static void InlinePrintDeclarationState(enum DeclarationState ds) {
  Print("%s", DeclarationStateTranslation(ds));
}

void PrintSymbol(Symbol s) {
  Print("%d: %.*s\n", s.symbol_id, s.token.length, s.token.position_in_source);
  InlinePrintDeclarationState(s.declaration_state);
  Print(" ");
  InlinePrintType(s.value.type);
  Print("\n");
  PrintValue(s.value);
}
