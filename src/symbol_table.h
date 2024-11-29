#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "parser_annotation.h"
#include "token.h"
#include "type.h"
#include "value.h"

#define MAX_FN_PARAMS 20

typedef enum {
  DECL_NONE,
  DECL_UNINITIALIZED,
  DECL_DECLARED,
  DECL_DEFINED,
  DECL_ENUM_COUNT, // For bounds checking
} DeclarationState;

typedef struct SymbolTable_impl SymbolTable;

typedef struct {
  int ordinality; // 0 is the first param, 1 is the second, etc
  Token param_token;
  ParserAnnotation annotation;
} FnParam;

typedef struct {
  int debug_id;
  DeclarationState declaration_state;
  ParserAnnotation annotation;
  Token token;
  Value value;
  SymbolTable *struct_fields;

  SymbolTable *fn_params;
  int fn_param_count;
  FnParam fn_param_list[MAX_FN_PARAMS];

  int declared_on_line; // I ultimately want to hold this data here, but it is still stored in the ParserAnnotation for now
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
Symbol NewSymbol(Token t, ParserAnnotation a, DeclarationState d);

Symbol AddTo(SymbolTable *st, Symbol s);
Symbol RetrieveFrom(SymbolTable *st, Token t);
bool IsIn(SymbolTable *st, Token t);
void RegisterFnParam(SymbolTable *st, Symbol function_name, Symbol param);

void SetValue(SymbolTable *st, Token t, Value v);

void PrintSymbol(Symbol s);

#endif
