#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#include "hashtable.h"
#include "token.h"

void AddToSymbolTable(HashTable *ht, Token token);
Token RetrieveFromSymbolTable(HashTable *ht, Token token);
bool IsInSymbolTable(HashTable *ht, Token token);
Token ResolveIdentifierAsValue(HashTable *ht, Token token);

#endif
