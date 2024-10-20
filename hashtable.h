#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "ast.h"
#include "token.h"

typedef struct {
  ParserAnnotation annotation;
  Token token;
} HT_Entry;

typedef struct {
  char *key;
  HT_Entry entry;
} Bucket;

typedef struct {
  int initial_capacity;
  int capacity;
  int num_buckets;
  Bucket **buckets;
} HashTable;

HashTable *NewHashTable();
HT_Entry GetEntry(HashTable *ht, const char *key);
void SetEntry(HashTable *ht, const char *key, HT_Entry e);

#endif
