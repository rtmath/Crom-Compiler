#include <stdlib.h> // for malloc, free

#include "common.h"
#include "error.h"
#include "symbol_table.h"

static char *ExtractString(Token token) {
  char *str = malloc(sizeof(char) * (token.length + ROOM_FOR_NULL_BYTE));
  for (int i = 0; i < token.length; i++) {
    str[i] = token.position_in_source[i];
  }
  str[token.length] = '\0';

  return str;
}

HT_Entry AddTo(HashTable *ht, HT_Entry e) {
  if (e.token.type == ERROR) ERROR_AND_EXIT("Tried adding an ERROR token to Symbol Table");

  char *key = ExtractString(e.token);

  HT_Entry stored = SetEntry(ht, key, e);

  free(key);

  return stored;
}

HT_Entry RetrieveFrom(HashTable *ht, Token token) {
  if (token.type == ERROR) ERROR_AND_EXIT("Cannot retrieve ERROR token from Symbol Table");

  char *key = ExtractString(token);

  HT_Entry e = GetEntry(ht, key);

  free(key);
  return e;
}

bool IsIn(HashTable *ht, Token token) {
  char *key = ExtractString(token);

  HT_Entry e = GetEntry(ht, key);

  free(key);
  return (e.token.type != ERROR);
}

Token ResolveIdentifierAsValue(HashTable *ht, Token token) {
  HT_Entry e = RetrieveFrom(ht, token);

  // TODO: Don't treat all identifiers as Ints, use their actual types
  // TODO: Also maybe there is a better way to resolve identifiers
  e.token.type = INT_CONSTANT;
  return e.token;
}
