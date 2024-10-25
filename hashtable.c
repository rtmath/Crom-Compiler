#include <math.h>   // for pow, sqrt, floor
#include <stdlib.h> // for malloc and friends
#include <stdbool.h>
#include <stdio.h>  // printf
#include <string.h> // for strlen, strcmp

#include "common.h"
#include "error.h"
#include "hashtable.h"

const int INITIAL_TABLE_CAPACITY = 53;
static int debug_guid = 0;

static Token HT_NOT_FOUND = {
  .type = ERROR,
  .position_in_source = "No HashTable entry found",
  .length = 24,
  .on_line = -1
};

HT_Entry Entry(Token t, ParserAnnotation a, DeclarationType d) {
  HashTable *fields = (a.ostensible_type == OST_STRUCT)
  ? NewHashTable()
  : NULL;

  HashTable *fn_params = (a.is_function)
  ? NewHashTable()
  : NULL;

  HT_Entry hte = {
    .token = t,
    .annotation = a,
    .declaration_type = d,
    .struct_fields = fields,
    .fn_params = fn_params,
  };

  return hte;
}

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

  const int hash_a = Hash(s, HT_PRIME_1, num_buckets);
  const int hash_b = Hash(s, HT_PRIME_2, num_buckets);

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

static Bucket *NewBucket(const char *k, HT_Entry e) {
  Bucket *b = malloc(sizeof(Bucket));
  b->key = CopyString(k);
  b->entry = e;

  return b;
}

static void FreeBucket(Bucket *b) {
  free(b->key);
  free(b);
}

static HashTable *NewHashTable_Sized(int initial_capacity) {
  HashTable *ht = malloc(sizeof(HashTable));
  ht->initial_capacity = initial_capacity;
  ht->capacity = NextPrime(initial_capacity);
  ht->num_buckets = 0;
  ht->buckets = calloc(ht->capacity, sizeof(Bucket));

  return ht;
}

HashTable *NewHashTable() {
  return NewHashTable_Sized(INITIAL_TABLE_CAPACITY);
}

static void DeleteHashTable(HashTable *ht) {
  for (int i = 0; i < ht->capacity; i++) {
    Bucket *b = ht->buckets[i];
    if (b != NULL) FreeBucket(b);
  }
  free(ht->buckets);
  free(ht);
}

static void ResizeHashTable(HashTable *ht, int new_capacity) {
  HashTable *temp_ht = NewHashTable_Sized(new_capacity);

  for (int i = 0; i < ht->capacity; i++) {
    Bucket *b = ht->buckets[i];
    if (b != NULL) SetEntry(temp_ht, b->key, b->entry);
  }

  ht->initial_capacity = temp_ht->initial_capacity;
  ht->num_buckets = temp_ht->num_buckets;

  int swap_capacity = ht->capacity;
  ht->capacity = temp_ht->capacity;
  temp_ht->capacity = swap_capacity;

  Bucket **swap_buckets = ht->buckets;
  ht->buckets = temp_ht->buckets;
  temp_ht->buckets = swap_buckets;

  DeleteHashTable(temp_ht);
}

HT_Entry GetEntry(HashTable *ht, const char *key) {
  int index = GetHash(key, ht->capacity, 0);
  Bucket *b = ht->buckets[index];

  int i = 1;
  while (b != NULL) {
    if (strcmp(b->key, key) == 0) return b->entry;

    // Collision occurred, try next bucket
    index = GetHash(key, ht->capacity, i);
    b = ht->buckets[index];
    i++;
  }

  return Entry(HT_NOT_FOUND, NoAnnotation(), DECL_NONE);
}

HT_Entry SetEntry(HashTable *ht, const char *key, HT_Entry e) {
  float table_load = (float)(ht->num_buckets / ht->capacity);
  if (table_load > 0.7) ResizeHashTable(ht, ht->initial_capacity * 2);

  Bucket *b = NewBucket(key, e);
  int index = GetHash(key, ht->capacity, 0);
  Bucket *check_bucket = ht->buckets[index];

  int i = 1;
  while (check_bucket != NULL) {
    if (strcmp(check_bucket->key, key) == 0) {
      // When SetEntry() overwrites an existing entry,
      // preserve the line it was declared on
      int preserve_dol = check_bucket->entry.annotation.declared_on_line;
      FreeBucket(check_bucket);

      b->entry.annotation.declared_on_line = preserve_dol;
      ht->buckets[index] = b;
      return b->entry;
    }

    // Collision occurred, try next bucket
    index = GetHash(b->key, ht->capacity, i);
    check_bucket = ht->buckets[index];
    i++;
  }

  // Save the line an item was declared on the first time it is stored
  b->entry.annotation.declared_on_line = b->entry.token.on_line;
  b->entry.debug_id = debug_guid++;

  ht->buckets[index] = b;
  ht->num_buckets++;
  return b->entry;
}

static const char* const _DeclarationTypeTranslation[] =
{
  [DECL_NONE] = "NONE",
  [DECL_UNINITIALIZED] = "UNINITIALIZED",
  [DECL_DECLARED] = "DECLARED",
  [DECL_DEFINED] = "DEFINED",
  [DECL_FN_PARAM] = "FUNCTION PARAM",
};

static const char *DeclarationTypeTranslation(DeclarationType dt) {
  if (dt < 0 || dt >= DECL_TYPE_COUNT) {
    ERROR_AND_EXIT_FMTMSG("PrintDeclarationType(): '%d' out of range", dt);
  }
  return _DeclarationTypeTranslation[dt];
}

static void InlinePrintDeclarationType(DeclarationType dt) {
  printf("DECL %s", DeclarationTypeTranslation(dt));
}

void PrintHTEntry(HT_Entry e) {
  PrintTokenVerbose(e.token);
  printf("HTEntry ID: '%d'\n", e.debug_id);
  InlinePrintDeclarationType(e.declaration_type);
  printf(" ");
  InlinePrintAnnotation(e.annotation);
  printf("\n");
  if (e.struct_fields != NULL) printf("has Struct Fields\n");
  if (e.fn_params != NULL) printf("has Function Params\n");
}
