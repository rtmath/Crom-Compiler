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

void AddToSymbolTable(HashTable *ht, Token token) {
  if (token.type == ERROR) ERROR_AND_EXIT("Tried adding an ERROR token to Symbol Table");

  char *key = ExtractString(token);

  SetToken(ht, key, token);

  free(key);
}

Token RetrieveFromSymbolTable(HashTable *ht, Token token) {
  if (token.type == ERROR) ERROR_AND_EXIT("Cannot retrieve ERROR token from Symbol Table");

  char *key = ExtractString(token);

  Token t = GetToken(ht, key);

  free(key);
  return t;
}

bool IsInSymbolTable(HashTable *ht, Token token) {
  char *key = ExtractString(token);

  Token t = GetToken(ht, key);

  free(key);
  return (t.type != ERROR);
}

Token ResolveIdentifierAsValue(HashTable *ht, Token token) {
  Token t = RetrieveFromSymbolTable(ht, token);

  // TODO: Don't treat all identifiers as Ints, use their actual types
  // TODO: Also maybe there is a better way to resolve identifiers
  t.type = INT_CONSTANT;
  return t;
}
