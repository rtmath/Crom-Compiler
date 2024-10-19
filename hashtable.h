#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "token.h"

typedef struct {
  char *key;
  Token token;
} Bucket;

typedef struct {
  int initial_capacity;
  int capacity;
  int num_buckets;
  Bucket **buckets;
} HashTable;

HashTable *NewHashTable();
Token GetToken(HashTable *ht, const char *key);
void SetToken(HashTable *ht, const char *key, Token t);

#endif
