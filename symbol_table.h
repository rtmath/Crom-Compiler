#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "parser_annotation.h"
#include "token.h"

#define MAX_FN_PARAMS 20

typedef enum {
  DECL_NONE,
  DECL_UNINITIALIZED,
  DECL_DECLARED,
  DECL_DEFINED,
  DECL_FN_PARAM,
  DECL_TYPE_COUNT,
} DeclarationType;

typedef struct SymbolTable_impl SymbolTable;

typedef struct {
  int ordinal_value; // 0 is the first param, 1 is the second, etc
  Token param_token;
  OstensibleType ostensible_type;
  ActualType actual_type;
} FnParam;

typedef struct {
  int debug_id;
  DeclarationType declaration_type;
  ParserAnnotation annotation;
  Token token;
  SymbolTable *struct_fields;

  SymbolTable *fn_params;
  int fn_param_count;
  FnParam fn_param_list[MAX_FN_PARAMS];
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
Symbol NewSymbol(Token t, ParserAnnotation a, DeclarationType d);

Symbol AddTo(SymbolTable *st, Symbol s);
Symbol RetrieveFrom(SymbolTable *st, Token t);
bool IsIn(SymbolTable *st, Token t);
void RegisterFnParam(SymbolTable *st, Symbol function_name, Symbol param);

void PrintSymbol(Symbol s);

#endif
