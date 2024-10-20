#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#include "hashtable.h"
#include "token.h"

void AddToSymbolTable(HashTable *ht, HT_Entry e);
HT_Entry RetrieveFromSymbolTable(HashTable *ht, Token t);
bool IsInSymbolTable(HashTable *ht, Token t);
Token ResolveIdentifierAsValue(HashTable *ht, Token t);

#endif
