#include "token_type.h"

static const char* const _TokenTypeTranslation[] =
{
  [IDENTIFIER] = "IDENTIFIER",

  [INT_CONSTANT] = "INT",
  [FLOAT_CONSTANT] = "FLOAT",

  [LCURLY] = "LCURLY", [RCURLY] = "RCURLY",
  [SEMICOLON] = "SEMICOLON",
  [PLUS] = "PLUS", [MINUS] = "MINUS", [ASTERISK] = "ASTERISK", [DIVIDE] = "DIVIDE",

  [EQUALS] = "EQUALS",

  [ERROR] = "ERROR"
};

const char *TokenTypeTranslation(TokenType t) {
  return _TokenTypeTranslation[t];
}
