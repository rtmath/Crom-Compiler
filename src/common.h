#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#include "token.h"

#define ROOM_FOR_NULL_BYTE 1

int64_t  TokenToInt64(Token t, int base);
uint64_t TokenToUint64(Token t, int base);
double   TokenToDouble(Token t);

#endif
