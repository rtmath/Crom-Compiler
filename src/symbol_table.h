#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "dynamic_array.h"
#include "token.h"
#include "value.h"

enum DeclarationState {
  DECL_NONE,
  DECL_UNINITIALIZED,
  DECL_DECLARED,
  DECL_DEFINED,
  DECL_ENUM_COUNT, // For bounds checking
};

#define UNDECLARED(symbol)    (symbol.declaration_state == DECL_NONE)
#define UNINITIALIZED(symbol) (symbol.declaration_state == DECL_UNINITIALIZED)
#define DECLARED(symbol)      (symbol.declaration_state == DECL_DECLARED)
#define DEFINED(symbol)       (symbol.declaration_state == DECL_DEFINED)

typedef struct {
  int symbol_id;

  enum DeclarationState declaration_state;
  Token token;
  Value value;

  int declared_on_line;
} Symbol;

#define IN_SYMBOL_TABLE(symbol) (symbol.token.type != ERROR)

typedef struct SymbolTable SymbolTable;

SymbolTable *NewSymbolTable();
void DeleteSymbolTable(SymbolTable *st);
Symbol NewSymbol(Token token, Type type, enum DeclarationState d);

Symbol AddTo(SymbolTable *st, Symbol s);
Symbol RetrieveFrom(SymbolTable *st, Token t);
bool IsIn(SymbolTable *st, Token t);

void RegisterFnParam(SymbolTable *st, Symbol function_name, Symbol param);
void AddParams(SymbolTable *st, Symbol function_symbol);

Symbol SetDecl(SymbolTable *st, Token t, enum DeclarationState ds);
Symbol SetValue(SymbolTable *st, Token t, Value v);
Symbol SetValueType(SymbolTable *st, Token t, Type type);

void PrintSymbol(Symbol s);

#endif
