#ifndef IO_H
#define IO_H

#include "token.h"

int ReadFile(const char *filename, char **dest);
void PrintSourceLine(const char *filename, int line_number);
void PrintSourceLineOfToken(Token t);

#endif
