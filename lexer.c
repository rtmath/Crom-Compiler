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

static Token Identifier() {
  while (IsAlpha(Peek()) || IsNumber(Peek())) Advance();
  return MakeToken(IDENTIFIER);
}

Token ScanToken() {
  SkipWhitespace();

  Lexer.start = Lexer.end;

  if (AtEOF()) return MakeToken(TOKEN_EOF);

  char c = Advance();

  if (IsNumber(c)) return Number();
  if (IsAlpha(c)) return Identifier();

  switch (c) {
    default:
      break;
  }

  return MakeErrorToken("Unexpected token");
}
