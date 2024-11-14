#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "assert.h"
#include "common.h"

typedef struct {
  char *key;
  TestResults results;
} HT_Bucket;

typedef struct {
  int initial_capacity;
  int capacity;
  int num_buckets;
  HT_Bucket **buckets;
} HashTable;

HashTable *NewHashTable();
TestResults GetResults(HashTable *ht, const char *key);
void SetResults(HashTable *ht, const char *key, TestResults t);

#endif
