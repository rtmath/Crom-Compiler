#include <stdio.h>

#include "common.h"
#include "token.h"

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
