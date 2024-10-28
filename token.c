#include <stdio.h>

#include "token.h"

void InlinePrintToken(Token t) {
  printf("%s(%.*s)",
      TokenTypeTranslation(t.type),
      t.length,
      t.position_in_source);
}

void PrintToken(Token t) {
  InlinePrintToken(t);
  printf("\n");
}

void PrintTokenVerbose(Token t) {
  printf("'%.*s' [%s:%d]\n",
         t.length,
         t.position_in_source,
         TokenTypeTranslation(t.type),
         t.on_line);
}
