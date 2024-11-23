#include <math.h>   // for pow, sqrt, floor
#include <stdlib.h> // for malloc and friends
#include <string.h> // for strlen, strcmp

#include "common.h"
#include "hashtable.h"

#define ROOM_FOR_NULL_BYTE 1

const int INITIAL_HT_TABLE_CAPACITY = 53;

static char *CopyString(const char *s) {
  int length = strlen(s);
  char *new_s = malloc(sizeof(char) * (length + ROOM_FOR_NULL_BYTE));

  for (int i = 0; i < length; i++) {
    new_s[i] = s[i];
  }
  new_s[length] = '\0';

  return new_s;
}

static int Hash(const char *s, int prime, int bucket_capacity) {
  unsigned long long hash = 0;
  int length = strlen(s);

  for (int i = 0; i < length; i++) {
    hash += (unsigned long long)pow(prime, length - (i + 1)) * s[i];
    hash = hash % bucket_capacity;
  }

  return (int)hash;
}

static int GetHash(const char *s, int num_buckets, int num_collisions) {
  /* Afaik these primes can be anything as long as they are greater
   * than ASCII's 128 table size and distinct from each other */
  #define HT_PRIME_1 151
  #define HT_PRIME_2 163

  int hash_a = Hash(s, HT_PRIME_1, num_buckets);
  int hash_b = Hash(s, HT_PRIME_2, num_buckets);
  if (hash_b % num_buckets == 0) hash_b = 1;

  /* (hash_b + 1) is used so that if hash_a collided at num_collisions = 0,
   * the (num_collisions * hash_b) expression won't potentially evaluate to
   * 0 when num_collisions >= 1. if that expression did evaluate to 0, this
   * function would essentially just return hash_a over and over (colliding infinitely) */
  return (hash_a + (num_collisions * (hash_b + 1))) % num_buckets;

  #undef HT_PRIME_1
  #undef HT_PRIME_2
}

static int IsPrime(int x) {
  const int UNDEFINED = -1;
  const int NOT_PRIME = 0;
  const int  IS_PRIME = 1;

  if (x < 2) return UNDEFINED;
  if (x < 4) return IS_PRIME;
  if (x % 2 == 0) return NOT_PRIME;
  for (int i = 3; i <= floor(sqrt((double) x)); i += 2) {
    if (x % i == 0) return NOT_PRIME;
  }

  return IS_PRIME;
}

static int NextPrime(int x) {
  while (IsPrime(x) != 1) {
    x++;
  }
  return x;
}

static HT_Bucket *NewBucket(const char *k, TestResults t) {
  HT_Bucket *b = malloc(sizeof(HT_Bucket));
  b->key = CopyString(k);
  b->results = t;

  return b;
}

static void FreeBucket(HT_Bucket *b) {
  free(b->key);
  free(b);
}

static HashTable *NewHashTable_Sized(int initial_capacity) {
  HashTable *ht = malloc(sizeof(HashTable));
  ht->initial_capacity = initial_capacity;
  ht->capacity = NextPrime(initial_capacity);
  ht->num_buckets = 0;
  ht->buckets = calloc(ht->capacity, sizeof(HT_Bucket));

  return ht;
}

HashTable *NewHashTable() {
  return NewHashTable_Sized(INITIAL_HT_TABLE_CAPACITY);
}

static void DeleteHashTable(HashTable *ht) {
  for (int i = 0; i < ht->capacity; i++) {
    HT_Bucket *b = ht->buckets[i];
    if (b != NULL) FreeBucket(b);
  }
  free(ht->buckets);
  free(ht);
}

static void ResizeHashTable(HashTable *ht, int new_capacity) {
  HashTable *temp_ht = NewHashTable_Sized(new_capacity);

  for (int i = 0; i < ht->capacity; i++) {
    HT_Bucket *b = ht->buckets[i];
    if (b != NULL) SetResults(temp_ht, b->key, b->results);
  }

  ht->initial_capacity = temp_ht->initial_capacity;
  ht->num_buckets = temp_ht->num_buckets;

  int swap_capacity = ht->capacity;
  ht->capacity = temp_ht->capacity;
  temp_ht->capacity = swap_capacity;

  HT_Bucket **swap_buckets = ht->buckets;
  ht->buckets = temp_ht->buckets;
  temp_ht->buckets = swap_buckets;

  DeleteHashTable(temp_ht);
}

TestResults GetResults(HashTable *ht, const char *key) {
  int index = GetHash(key, ht->capacity, 0);
  HT_Bucket *b = ht->buckets[index];

  int i = 1;
  while (b != NULL) {
    if (strcmp(b->key, key) == 0) return b->results;

    // Collision occurred, try next bucket
    index = GetHash(key, ht->capacity, i);
    b = ht->buckets[index];
    i++;
  }

  // Not found
  return (TestResults){
    .succeeded = 0,
    .failed = 0,
  };
}

void SetResults(HashTable *ht, const char *key, TestResults t) {
  float table_load = (float)(ht->num_buckets / ht->capacity);
  if (table_load > 0.7) ResizeHashTable(ht, ht->initial_capacity * 2);

  HT_Bucket *b = NewBucket(key, t);
  int index = GetHash(key, ht->capacity, 0);
  HT_Bucket *check_bucket = ht->buckets[index];

  int i = 1;
  while (check_bucket != NULL) {
    if (strcmp(check_bucket->key, key) == 0) {
      FreeBucket(check_bucket);
      ht->buckets[index] = b;
      return;
    }

    // Collision occurred, try next bucket
    index = GetHash(b->key, ht->capacity, i);
    check_bucket = ht->buckets[index];
    i++;
  }

  ht->buckets[index] = b;
  ht->num_buckets++;
}
