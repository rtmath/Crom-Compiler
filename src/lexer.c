#include <stdbool.h>
#include <string.h> // for strlen

#include "lexer.h"
#include "token_type.h"

struct {
  const char *start;
  const char *end;

  // For helpful error messages
  int current_line;
  int current_x_offset;
  const char *src_filename;
} Lexer;

void InitLexer(const char *filename, const char *contents) {
  Lexer.start = contents;
  Lexer.end = contents;
  Lexer.current_line = 1;
  Lexer.current_x_offset = 0;
  Lexer.src_filename = filename;
}

static int LexemeLength() {
  return Lexer.end - Lexer.start;
}

static bool IsAlpha(char c) {
  return (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         (c == '_');
}

static bool IsNumber(char c) {
  return c >= '0' && c <= '9';
}

static bool IsHex(char c) {
  return IsNumber(c) || (c >= 'A' && c <= 'H') ||
                        (c >= 'a' && c <= 'h');
}

static bool AtEOF() {
  return *Lexer.start == '\0';
}

static char Peek() {
  return Lexer.end[0];
}

static char PeekNext() {
  return Lexer.end[1];
}

static char Advance() {
  Lexer.current_x_offset++;
  Lexer.end++;
  return Lexer.end[-1];
}

static bool Match(char c) {
  if (Lexer.end[0] != c) return false;

  Advance();

  return true;
}

static void SkipWhitespace() {
  while(1) {
    char c = Peek();
    switch(c) {
      case ' ':
      case '\r':
      case '\t': {
        Advance();
      } break;

      case '\n': {
        Lexer.current_line++;
        Lexer.current_x_offset = -1; // The subsequent call to Advance() will set this appropriately to 0
        Advance();
      } break;

      case '/': {
        if (PeekNext() == '/') {
          while (Peek() != '\n' && !AtEOF()) Advance();
        } else {
          return;
        }
      } break;

      default: return;
    }
  }
}

static Token MakeErrorToken(const char *msg) {
  Token t = {0};
  t.type = ERROR;
  t.position_in_source = msg;
  t.length = (int)strlen(msg);
  t.on_line = Lexer.current_line;
  t.from_filename = Lexer.src_filename;

  return t;
}

static Token MakeToken(TokenType type) {
  Token t = {0};
  t.type = type;
  t.position_in_source = Lexer.start;
  t.length = Lexer.end - Lexer.start;
  t.on_line = Lexer.current_line;
  t.from_filename = Lexer.src_filename;
  t.line_x_offset = Lexer.current_x_offset - t.length;

  return t;
}

static Token Hex() {
  Advance(); // consume the Peek()'d 'x'

  while (IsHex(Peek())) Advance();

  if (LexemeLength() > (2 + 16)) { // "0x" + up to 16 Hex Digits (0-F)
    return MakeErrorToken("Hex Literal cannot be more than 64 bits wide");
  }

  return MakeToken(HEX_LITERAL);
}

static Token Binary() {
  Lexer.start = Lexer.end; // Shave the '`' from the start of the lexeme

  while (Peek() == '0' || Peek() == '1' || Peek() == ' ') Advance();

  if (Peek() != '`') return MakeErrorToken("Expected \'`\' after Binary Literal");
  Advance(); // consume the Peek()'d "`"

  Token t = MakeToken(BINARY_LITERAL);
  t.length--; // Shave the "`" from the end of the lexeme

  return t;
}

static Token Number() {
  bool is_float = false;

  while (IsNumber(Peek())) Advance();

  if (Peek() == '.' && !IsNumber(PeekNext())) {
    return MakeErrorToken("Invalid float literal");
  }

  if (Peek() == '.' && IsNumber(PeekNext())) {
    is_float = true;
    Advance();

    while (IsNumber(Peek())) Advance();
  }

  return MakeToken((is_float) ? FLOAT_LITERAL: INT_LITERAL);
}

static Token Char() {
  if (Peek() == '\'') return MakeErrorToken("Empty char constant");

  Match('\\');
  Advance(); // consume char value
  if (Peek() != '\'') {
    return MakeErrorToken("More than one character in char literal");
  }
  Advance(); // consume '

  return MakeToken(CHAR_LITERAL);
}

static Token String() {
  Lexer.start = Lexer.end; // Forget the '"' part of the lexeme

  while (Peek() != '"' && !AtEOF()) {
    if (Peek() == '\0') return MakeErrorToken("Unterminated string");
    if (Peek() == '\n') return MakeErrorToken("Multi-line strings are not allowed");

    Advance();
  }

  if (AtEOF()) return MakeErrorToken("Unterminated string");

  Advance();

  Token t = MakeToken(STRING_LITERAL);
  t.length--; // Shave the '"' from the end of the lexeme

  return t;
}

static bool LexemeEquals(const char *str, int len) {
  return (Lexer.end - Lexer.start == len) &&
         (memcmp(Lexer.start, str, len) == 0);
}

static TokenType IdentifierType() {
  if (LexemeEquals( "i8", 2)) return I8;
  if (LexemeEquals("i16", 3)) return I16;
  if (LexemeEquals("i32", 3)) return I32;
  if (LexemeEquals("i64", 3)) return I64;

  if (LexemeEquals( "u8", 2)) return U8;
  if (LexemeEquals("u16", 3)) return U16;
  if (LexemeEquals("u32", 3)) return U32;
  if (LexemeEquals("u64", 3)) return U64;

  if (LexemeEquals("f32", 3)) return F32;
  if (LexemeEquals("f64", 3)) return F64;

  if (LexemeEquals("char", 4)) return CHAR;
  if (LexemeEquals("string", 6)) return STRING;

  if (LexemeEquals("bool", 4)) return BOOL;
  if (LexemeEquals("void", 4)) return VOID;

  if (LexemeEquals("enum", 4)) return ENUM;
  if (LexemeEquals("struct", 6)) return STRUCT;

  if (LexemeEquals("if", 2)) return IF;
  if (LexemeEquals("else", 4)) return ELSE;
  if (LexemeEquals("while", 5)) return WHILE;
  if (LexemeEquals("for", 3)) return FOR;

  if (LexemeEquals("break", 5)) return BREAK;
  if (LexemeEquals("continue", 8)) return CONTINUE;
  if (LexemeEquals("return", 6)) return RETURN;

  if (LexemeEquals("true", 4))  return BOOL_LITERAL;
  if (LexemeEquals("false", 5)) return BOOL_LITERAL;

  return IDENTIFIER;
}

static Token Identifier() {
  while (IsAlpha(Peek()) || IsNumber(Peek())) Advance();
  return MakeToken(IdentifierType());
}

Token ScanToken() {
  SkipWhitespace();

  Lexer.start = Lexer.end;

  if (AtEOF()) return MakeToken(TOKEN_EOF);

  char c = Advance();

  if (c == '0' && Peek() == 'x') return Hex();
  if (IsNumber(c)) return Number();

  if (IsAlpha(c)) return Identifier();

  switch (c) {
    case '{': return MakeToken(LCURLY);
    case '}': return MakeToken(RCURLY);
    case '(': return MakeToken(LPAREN);
    case ')': return MakeToken(RPAREN);
    case '[': return MakeToken(LBRACKET);
    case ']': return MakeToken(RBRACKET);
    case '.': return MakeToken(PERIOD);
    case ',': return MakeToken(COMMA);
    case ':': return MakeToken(Match(':') ? COLON_SEPARATOR : COLON);
    case ';': return MakeToken(SEMICOLON);
    case '+': {
      if (Match('=')) return MakeToken(PLUS_EQUALS);
      if (Match('+')) return MakeToken(PLUS_PLUS);
      return MakeToken(PLUS);
    }
    case '-': {
      if (Match('=')) return MakeToken(MINUS_EQUALS);
      if (Match('-')) return MakeToken(MINUS_MINUS);
      return MakeToken(MINUS);
    }
    case '*': return MakeToken(Match('=') ? TIMES_EQUALS: ASTERISK);
    case '/': return MakeToken(Match('=') ? DIVIDE_EQUALS : DIVIDE);
    case '%': return MakeToken(Match('=') ? MODULO_EQUALS : MODULO);
    case '~': return MakeToken(BITWISE_NOT);
    case '`': return Binary();
    case '^': return MakeToken(Match('=') ? BITWISE_XOR_EQUALS : BITWISE_XOR);
    case '&': return MakeToken(Match('=')
                               ? BITWISE_AND_EQUALS
                               : Match('&')
                                 ? LOGICAL_AND
                                 : BITWISE_AND);
    case '|': return MakeToken(Match('=')
                               ? BITWISE_OR_EQUALS
                               : Match('|')
                                 ? LOGICAL_OR
                                 : BITWISE_OR);
    case '!': return MakeToken(Match('=') ? LOGICAL_NOT_EQUALS : LOGICAL_NOT);
    case '?': return MakeToken(QUESTION_MARK);
    case '<': {
      if (Match('<')) return MakeToken(Match('=') ? BITWISE_LEFT_SHIFT_EQUALS : BITWISE_LEFT_SHIFT);
      return MakeToken(Match('=') ? LESS_THAN_EQUALS: LESS_THAN);
    }
    case '>': {
      if (Match('>')) return MakeToken(Match('=') ? BITWISE_RIGHT_SHIFT_EQUALS : BITWISE_RIGHT_SHIFT);
      return MakeToken(Match('=') ? GREATER_THAN_EQUALS : GREATER_THAN);
    }
    case '=': return MakeToken(Match('=') ? EQUALITY : EQUALS);
    case '\'': return Char();
    case '"': return String();
    default:
      break;
  }

  return MakeErrorToken("Unexpected token");
}
