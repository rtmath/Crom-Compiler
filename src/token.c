#include <string.h>

#include "common.h"
#include "token.h"

bool TokenValuesMatch(Token a, Token b) {
  return (a.type != ERROR &&
          a.type == b.type &&
          a.length == b.length &&
          strncmp(a.position_in_source,
                  b.position_in_source,
                  a.length) == 0);
}

void InlinePrintToken(Token t) {
  Print("%s(%.*s)",
        TokenTypeTranslation(t.type),
        t.length,
        t.position_in_source);
}

void PrintToken(Token t) {
  InlinePrintToken(t);
  Print("\n");
}

void PrintTokenVerbose(Token t) {
  Print("'%.*s' [%s:%d]\n",
         t.length,
         t.position_in_source,
         TokenTypeTranslation(t.type),
         t.on_line);
}
