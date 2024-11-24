#include <math.h>   // for pow, sqrt, floor
#include <stdlib.h> // for malloc and friends
#include <stdbool.h>
#include <stdio.h>  // printf
#include <string.h> // for strlen, strcmp

#include "common.h"
#include "error.h"
#include "symbol_table.h"

const int INITIAL_TABLE_CAPACITY = 53;
static int debug_guid = 0;

static Symbol GetSymbol(SymbolTable *st, const char *key);
static Symbol SetSymbol(SymbolTable *st, const char *key, Symbol s);

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

  return (hash_a + (num_collisions * (hash_b))) % num_buckets;

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

static Bucket *NewBucket(const char *k, Symbol s) {
  Bucket *b = malloc(sizeof(Bucket));
  b->key = CopyString(k);
  b->entry = s;

  return b;
}

static void FreeBucket(Bucket *b) {
  free(b->key);
  free(b);
}

static SymbolTable *NewSymbolTable_Sized(int initial_capacity) {
  SymbolTable *st = malloc(sizeof(SymbolTable));
  st->initial_capacity = initial_capacity;
  st->capacity = NextPrime(initial_capacity);
  st->num_buckets = 0;
  st->buckets = calloc(st->capacity, sizeof(Bucket));

  return st;
}

void DeleteSymbolTable(SymbolTable *st) {
  for (int i = 0; i < st->capacity; i++) {
    Bucket *b = st->buckets[i];
    if (b != NULL) FreeBucket(b);
  }
  free(st->buckets);
  free(st);
}

static void ResizeSymbolTable(SymbolTable *st, int new_capacity) {
  SymbolTable *temp_st = NewSymbolTable_Sized(new_capacity);

  for (int i = 0; i < st->capacity; i++) {
    Bucket *b = st->buckets[i];
    if (b != NULL) SetSymbol(temp_st, b->key, b->entry);
  }

  st->initial_capacity = temp_st->initial_capacity;
  st->num_buckets = temp_st->num_buckets;

  int swap_capacity = st->capacity;
  st->capacity = temp_st->capacity;
  temp_st->capacity = swap_capacity;

  Bucket **swap_buckets = st->buckets;
  st->buckets = temp_st->buckets;
  temp_st->buckets = swap_buckets;

  DeleteSymbolTable(temp_st);
}

static Symbol GetSymbol(SymbolTable *st, const char *key) {
  int index = GetHash(key, st->capacity, 0);
  Bucket *b = st->buckets[index];

  int i = 1;
  while (b != NULL) {
    if (strcmp(b->key, key) == 0) return b->entry;

    // Collision occurred, try next bucket
    index = GetHash(key, st->capacity, i);
    b = st->buckets[index];
    i++;
  }

  Token SYMBOL_NOT_FOUND = {
    .type = ERROR,
    .position_in_source = "No symbol found in Symbol Table",
    .length = 31,
    .on_line = -1
  };

  return NewSymbol(SYMBOL_NOT_FOUND, NoAnnotation(), DECL_NONE);
}

static Symbol SetSymbol(SymbolTable *st, const char *key, Symbol s) {
  float table_load = (float)(st->num_buckets / st->capacity);
  if (table_load > 0.7) ResizeSymbolTable(st, st->initial_capacity * 2);

  Bucket *b = NewBucket(key, s);
  int index = GetHash(key, st->capacity, 0);
  Bucket *check_bucket = st->buckets[index];

  int i = 1;
  while (check_bucket != NULL) {
    if (strcmp(check_bucket->key, key) == 0) {
      // When SetEntry() overwrites an existing entry,
      // preserve the line it was declared on
      int preserve_dol = check_bucket->entry.annotation.declared_on_line;
      int preserve_id = check_bucket->entry.debug_id;
      FreeBucket(check_bucket);

      b->entry.annotation.declared_on_line = preserve_dol;
      b->entry.debug_id = preserve_id;
      st->buckets[index] = b;
      return b->entry;
    }

    // Collision occurred, try next bucket
    index = GetHash(b->key, st->capacity, i);
    check_bucket = st->buckets[index];
    i++;
  }

  // Save the line an item was declared on the first time it is stored
  b->entry.annotation.declared_on_line = b->entry.token.on_line;
  b->entry.debug_id = debug_guid++;

  st->buckets[index] = b;
  st->num_buckets++;
  return b->entry;
}

static const char* const _DeclarationStateTranslation[] =
{
  [DECL_NONE] = "NONE",
  [DECL_UNINITIALIZED] = "UNINITIALIZED",
  [DECL_DECLARED] = "DECLARED",
  [DECL_DEFINED] = "DEFINED",
};

static const char *DeclarationStateTranslation(DeclarationState ds) {
  if (ds < 0 || ds >= DECL_ENUM_COUNT) {
    ERROR_AND_EXIT_FMTMSG("DeclarationStateTranslation(): '%d' out of range", ds);
  }
  return _DeclarationStateTranslation[ds];
}

static void InlinePrintDeclarationState(DeclarationState ds) {
  printf("%s", DeclarationStateTranslation(ds));
}

static char *ExtractString(Token token) {
  char *str = malloc(sizeof(char) * (token.length + ROOM_FOR_NULL_BYTE));
  for (int i = 0; i < token.length; i++) {
    str[i] = token.position_in_source[i];
  }
  str[token.length] = '\0';

  return str;
}

SymbolTable *NewSymbolTable() {
  return NewSymbolTable_Sized(INITIAL_TABLE_CAPACITY);
}

Symbol NewSymbol(Token t, ParserAnnotation a, DeclarationState d) {
  SymbolTable *fields = (a.ostensible_type == OST_STRUCT)
  ? NewSymbolTable()
  : NULL;

  SymbolTable *fn_params = (a.is_function)
  ? NewSymbolTable()
  : NULL;

  Symbol s = {
    .token = t,
    .annotation = a,
    .value = {0},
    .declaration_state = d,
    .struct_fields = fields,
    .fn_params = fn_params,
    .fn_param_count = 0,
  };

  return s;
}

Symbol AddTo(SymbolTable *st, Symbol s) {
  if (s.token.type == ERROR) ERROR_AND_EXIT("Tried adding an ERROR token to Symbol Table");

  char *key = ExtractString(s.token);

  Symbol stored_symbol = SetSymbol(st, key, s);
  free(key);

  return stored_symbol;
}

Symbol RetrieveFrom(SymbolTable *st, Token token) {
  if (token.type == ERROR) ERROR_AND_EXIT("Cannot retrieve ERROR token from Symbol Table");

  char *key = ExtractString(token);

  Symbol symbol = GetSymbol(st, key);

  free(key);
  return symbol;
}

bool IsIn(SymbolTable *st, Token token) {
  char *key = ExtractString(token);

  Symbol symbol = GetSymbol(st, key);

  free(key);
  return (symbol.token.type != ERROR);
}

void RegisterFnParam(SymbolTable *st, Symbol function, Symbol param) {
  FnParam fp = {
    .ordinality = function.fn_param_count,
    .param_token = param.token,
    .annotation = param.annotation,
  };
  function.fn_param_list[function.fn_param_count] = fp;
  function.fn_param_count++;

  AddTo(st, function);
}

void SetValue(SymbolTable *st, Token t, Value v) {
  Symbol s = RetrieveFrom(st, t);
  if (s.token.type == ERROR) {
    printf("SetValue(): Token %.*s not found in symbol table", t.length, t.position_in_source);
    return;
  };

  s.value = v;
  AddTo(st, s);
}

void PrintSymbol(Symbol s) {
  printf("%d: %.*s\n", s.debug_id, s.token.length, s.token.position_in_source);
  InlinePrintDeclarationState(s.declaration_state);
  printf(" ");
  InlinePrintOstAnnotation(s.annotation);
  printf("\n");
  if (s.struct_fields != NULL) printf("has Struct Fields\n");
  if (s.fn_params != NULL) printf("has %d Function Params\n", s.fn_param_count);
  PrintValue(s.value);
}
