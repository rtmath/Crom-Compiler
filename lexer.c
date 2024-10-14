#include <stdbool.h>
#include <string.h> // for strlen

#include "lexer.h"
#include "token_type.h"

struct {
  const char *start;
  const char *end;
  int current_line;
} Lexer;

void InitLexer(const char *source) {
  Lexer.start = source;
  Lexer.end = source;
  Lexer.current_line = 1;
}

static bool IsAlpha(char c) {
  return (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         (c == '_');
}

static bool IsNumber(char c) {
  return c >= '0' && c <= '9';
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
  Lexer.end++;
  return Lexer.end[-1];
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

  return t;
}

static Token MakeToken(TokenType type) {
  Token t = {0};
  t.type = type;
  t.position_in_source = Lexer.start;
  t.length = Lexer.end - Lexer.start;
  t.on_line = Lexer.current_line;

  return t;
}

static Token Number() {
  bool is_float = false;

  while (IsNumber(Peek())) Advance();

  if (Peek() == '.' && IsNumber(PeekNext())) {
    is_float = true;
    Advance();

    while (IsNumber(Peek())) Advance();
  }

  return MakeToken((is_float) ? FLOAT_CONSTANT : INT_CONSTANT);
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

  if (IsNumber(c)) return Number();
  if (IsAlpha(c)) return Identifier();

  switch (c) {
    case '(': return MakeToken(LPAREN);
    case ')': return MakeToken(RPAREN);
    case ';': return MakeToken(SEMICOLON);
    case '+': return MakeToken(PLUS);
    case '-': return MakeToken(MINUS);
    case '*': return MakeToken(ASTERISK);
    case '/': return MakeToken(DIVIDE);
    case '=': return MakeToken(EQUALS);
    default:
      break;
  }

  return MakeErrorToken("Unexpected token");
}
