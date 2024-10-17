#include "token_type.h"

static const char* const _TokenTypeTranslation[] =
{
  [UNINITIALIZED] = "UNINITIALIZED",

  [I8]  = "I8", [I16] = "I16", [I32] = "I32", [I64] = "I64",
  [U8]  = "U8", [U16] = "U16", [U32] = "U32", [U64] = "U64",
  [F32] = "F32", [F64] = "F64",
  [CHAR] = "CHAR", [STRING] = "STRING",
  [BOOL] = "BOOL",
  [VOID] = "VOID",
  [ENUM] = "ENUM",
  [STRUCT] = "STRUCT",

  [IDENTIFIER] = "IDENTIFIER",

  [INT_CONSTANT] = "INT",
  [FLOAT_CONSTANT] = "FLOAT",

  [STRING_LITERAL] = "STRING_LITERAL",

  [LCURLY] = "LCURLY", [RCURLY] = "RCURLY",
  [LPAREN] = "LPAREN", [RPAREN] = "RPAREN",
  [SEMICOLON] = "SEMICOLON",
  [PLUS] = "PLUS", [MINUS] = "MINUS", [ASTERISK] = "ASTERISK", [DIVIDE] = "DIVIDE",

  [EQUALS] = "EQUALS",

  [ERROR] = "ERROR",
  [TOKEN_EOF] = "EOF"
};

const char *TokenTypeTranslation(TokenType t) {
  if (t < 0 || t >= TOKEN_TYPE_COUNT) return "OUT OF BOUNDS";
  return _TokenTypeTranslation[t];
}
