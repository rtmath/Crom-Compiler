#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

#include "token.h"

#define ROOM_FOR_NULL_BYTE 1

int64_t  TokenToInt64(Token t, int base);
uint64_t TokenToUint64(Token t, int base);
double   TokenToDouble(Token t);

bool Int64Overflow(Token t, int base);
bool Uint64Overflow(Token t, int base);
bool DoubleOverflow(Token t);
bool DoubleUnderflow(Token t);

char *CopyString(const char *s);
char *CopyStringL(const char *s, int length);

#endif
