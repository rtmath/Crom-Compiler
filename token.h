#ifndef TOKEN_H
#define TOKEN_H

#include "token_type.h"

typedef struct {
  TokenType type;
  const char *position_in_source;
  int length;
  int on_line;
} Token;

void InlinePrintToken(Token t);
void PrintToken(Token t);
void PrintTokenVerbose(Token t);

#endif
