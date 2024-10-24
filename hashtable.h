#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "ast.h"
#include "token.h"

typedef enum {
  DECL_NOT_APPLICABLE,
  DECL_AWAITING_INIT,
  DECL_INITIALIZED,
} DeclarationType;

typedef struct HashTable_impl HashTable;

typedef struct {
  DeclarationType declaration_type;
  ParserAnnotation annotation;
  Token token;
  HashTable *struct_fields;
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
void SetEntry(HashTable *ht, const char *key, HT_Entry e);

#endif
