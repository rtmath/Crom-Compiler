#ifndef LEXER_H
#define LEXER_H

#include "token.h"

void InitLexer(const char* filename);
Token ScanToken();

#endif
