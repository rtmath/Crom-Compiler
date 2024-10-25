#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "ast.h"
#include "token.h"

typedef enum {
  DECL_NONE,
  DECL_UNINITIALIZED,
  DECL_DECLARED,
  DECL_DEFINED,
  DECL_FN_PARAM,
  DECL_TYPE_COUNT,
} DeclarationType;

typedef struct HashTable_impl HashTable;

typedef struct {
  int debug_id;
  DeclarationType declaration_type;
  ParserAnnotation annotation;
  Token token;
  HashTable *struct_fields;
  HashTable *fn_params;
} HT_Entry;

typedef struct {
  char *key;
  HT_Entry entry;
} Bucket;

struct HashTable_impl {
  int initial_capacity;
  int capacity;
  int num_buckets;
  Bucket **buckets;
};

HashTable *NewHashTable();
HT_Entry Entry(Token t, ParserAnnotation a, DeclarationType d);
HT_Entry GetEntry(HashTable *ht, const char *key);
HT_Entry SetEntry(HashTable *ht, const char *key, HT_Entry e);

void PrintHTEntry(HT_Entry e);

#endif
