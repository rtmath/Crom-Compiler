#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "parser_annotation.h"
#include "token.h"

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
  int debug_id;
  DeclarationType declaration_type;
  ParserAnnotation annotation;
  Token token;
  SymbolTable *struct_fields;
  SymbolTable *fn_params;
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
Symbol NewSymbol(Token t, ParserAnnotation a, DeclarationType d);

Symbol AddTo(SymbolTable *st, Symbol s);
Symbol RetrieveFrom(SymbolTable *st, Token t);
bool IsIn(SymbolTable *st, Token t);

void PrintSymbol(Symbol s);

#endif
