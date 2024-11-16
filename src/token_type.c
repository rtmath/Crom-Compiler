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
  [IF] = "IF", [ELSE] = "ELSE", [WHILE] = "WHILE", [FOR] = "FOR",
  [BREAK] = "BREAK", [CONTINUE] = "CONTINUE", [RETURN] = "RETURN",

  [IDENTIFIER] = "IDENTIFIER",

  [HEX_LITERAL] = "HEX_LITERAL",
  [BINARY_LITERAL] = "BINARY_LITERAL",
  [INT_LITERAL] = "INT_LITERAL",
  [FLOAT_LITERAL] = "FLOAT_LITERAL",

  [ENUM_LITERAL] = "ENUM_LITERAL",
  [CHAR_LITERAL] = "CHAR_LITERAL",
  [BOOL_LITERAL] = "BOOL_LITERAL",
  [STRING_LITERAL] = "STRING_LITERAL",

  [LCURLY] = "LCURLY", [RCURLY] = "RCURLY",
  [LPAREN] = "LPAREN", [RPAREN] = "RPAREN",
  [LBRACKET] = "LBRACKET", [RBRACKET] = "RBRACKET",
  [PERIOD] = "PERIOD", [COMMA] = "COMMA", [COLON] = "COLON", [SEMICOLON] = "SEMICOLON", [COLON_SEPARATOR] = "COLON_SEPARATOR",
  [QUESTION_MARK] = "QUESTION_MARK",

  [LOGICAL_NOT] = "LOGICAL_NOT", [LOGICAL_AND] = "LOGICAL_AND", [LOGICAL_OR] = "LOGICAL_OR", [LESS_THAN] = "LESS_THAN", [GREATER_THAN] = "GREATER_THAN",

  [PLUS] = "PLUS", [MINUS] = "MINUS", [ASTERISK] = "ASTERISK", [DIVIDE] = "DIVIDE", [MODULO] = "MODULO",
  [PLUS_PLUS] = "PLUS_PLUS", [MINUS_MINUS] = "MINUS_MINUS",
  [BITWISE_NOT] = "BITWISE_NOT", [BITWISE_XOR] = "BITWISE_XOR", [BITWISE_AND] = "BITWISE_AND", [BITWISE_OR] = "BITWISE_OR",
  [BITWISE_LEFT_SHIFT] = "LEFT_SHIFT", [BITWISE_RIGHT_SHIFT] = "RIGHT_SHIFT",

  [EQUALS] = "EQUALS", [LOGICAL_NOT_EQUALS] = "LOGICAL_NOT_EQUALS",
  [PLUS_EQUALS] = "PLUS_EQUALS", [MINUS_EQUALS] = "MINUS_EQUALS", [TIMES_EQUALS] = "TIMES_EQUALS", [DIVIDE_EQUALS] = "DIVIDE_EQUALS", [MODULO_EQUALS] = "MODULO_EQUALS",
  [BITWISE_XOR_EQUALS] = "XOR_EQUALS", [BITWISE_AND_EQUALS] = "AND_EQUALS", [BITWISE_OR_EQUALS] = "OR_EQUALS",
  [BITWISE_LEFT_SHIFT_EQUALS] = "LEFT_SHIFT_EQUALS", [BITWISE_RIGHT_SHIFT_EQUALS] = "RIGHT_SHIFT_EQUALS",

  [ERROR] = "ERROR",
  [TOKEN_EOF] = "EOF"
};

const char *TokenTypeTranslation(TokenType t) {
  if (t < 0 || t >= TOKEN_TYPE_COUNT) return "TokenTypeTranslation(): OUT OF BOUNDS";
  return _TokenTypeTranslation[t];
}