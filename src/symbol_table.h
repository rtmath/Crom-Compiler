#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "token.h"
#include "type.h"
#include "value.h"

typedef enum {
  DECL_NONE,
  DECL_UNINITIALIZED,
  DECL_DECLARED,
  DECL_DEFINED,
  DECL_ENUM_COUNT, // For bounds checking
} DeclarationState;

typedef struct SymbolTable_impl SymbolTable;

typedef struct {
  int debug_id;
  DeclarationState declaration_state;
  Token token;
  Value value;

  int declared_on_line;
} Symbol;

typedef struct {
  char *key;
  Symbol entry;
} Bucket;

struct SymbolTable_impl {
  int initial_capacity;
  int capacity;
  int num_buckets;
  Bucket **buckets;
};

SymbolTable *NewSymbolTable();
void DeleteSymbolTable(SymbolTable *st);
Symbol NewSymbol(Token token, Type type, DeclarationState d);

Symbol AddTo(SymbolTable *st, Symbol s);
Symbol RetrieveFrom(SymbolTable *st, Token t);
bool IsIn(SymbolTable *st, Token t);
void RegisterFnParam(SymbolTable *st, Symbol function_name, Symbol param);

void AddParams(SymbolTable *st, Symbol function_symbol);

void SetValue(SymbolTable *st, Token t, Value v);
void SetValueType(SymbolTable *st, Token t, Type type);

void PrintSymbol(Symbol s);

#endif
