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

typedef struct {
  int symbol_id;

  enum DeclarationState declaration_state;
  Token token;
  Value value;

  int declared_on_line;
} Symbol;

typedef struct SymbolTable SymbolTable;

SymbolTable *NewSymbolTable();
void DeleteSymbolTable(SymbolTable *st);
Symbol NewSymbol(Token token, Type type, enum DeclarationState d);

Symbol AddTo(SymbolTable *st, Symbol s);
Symbol RetrieveFrom(SymbolTable *st, Token t);
bool IsIn(SymbolTable *st, Token t);
void RegisterFnParam(SymbolTable *st, Symbol function_name, Symbol param);

void AddParams(SymbolTable *st, Symbol function_symbol);

void SetValue(SymbolTable *st, Token t, Value v);
void SetValueType(SymbolTable *st, Token t, Type type);

void PrintSymbol(Symbol s);

#endif
