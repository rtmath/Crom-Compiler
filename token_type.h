#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

typedef enum {
  UNINITIALIZED,

  // Keywords
  I8, I16, I32, I64,
  U8, U16, U32, U64,
  F32, F64,
  CHAR, STRING,
  BOOL,
  VOID,
  ENUM, STRUCT,
  IF, ELSE, WHILE, FOR,
  BREAK, CONTINUE, RETURN,

  IDENTIFIER,

  // Literals
  BINARY_LITERAL,
  HEX_LITERAL,
  INT_LITERAL,
  FLOAT_LITERAL,

  ENUM_LITERAL,
  CHAR_LITERAL,
  BOOL_LITERAL,
  STRING_LITERAL,

  // Punctuators
  LCURLY, RCURLY,
  LPAREN, RPAREN,
  LBRACKET, RBRACKET,
  PERIOD, COMMA, COLON, SEMICOLON, COLON_SEPARATOR,
  QUESTION_MARK,

  LOGICAL_NOT, LOGICAL_AND, LOGICAL_OR, LESS_THAN, GREATER_THAN,

  PLUS, MINUS, ASTERISK, DIVIDE, MODULO,
  PLUS_PLUS, MINUS_MINUS,
  BITWISE_NOT, BITWISE_XOR, BITWISE_AND, BITWISE_OR,
  BITWISE_LEFT_SHIFT, BITWISE_RIGHT_SHIFT,

  EQUALS, LOGICAL_NOT_EQUALS,
  PLUS_EQUALS, MINUS_EQUALS, TIMES_EQUALS, DIVIDE_EQUALS, MODULO_EQUALS,
  BITWISE_XOR_EQUALS, BITWISE_AND_EQUALS, BITWISE_OR_EQUALS, BITWISE_NOT_EQUALS,
  BITWISE_LEFT_SHIFT_EQUALS, BITWISE_RIGHT_SHIFT_EQUALS,

  // Misc
  ERROR, TOKEN_EOF,
  TOKEN_TYPE_COUNT,
} TokenType;

const char *TokenTypeTranslation(TokenType t);

#endif
