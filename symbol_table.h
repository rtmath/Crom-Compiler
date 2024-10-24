#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>

#include "hashtable.h"
#include "token.h"

void AddTo(HashTable *ht, HT_Entry e);
HT_Entry RetrieveFrom(HashTable *ht, Token t);
bool IsIn(HashTable *ht, Token t);
Token ResolveIdentifierAsValue(HashTable *ht, Token t);

#endif
