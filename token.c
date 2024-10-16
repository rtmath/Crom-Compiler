#include <stdio.h>

#include "token.h"

void PrintToken(Token t) {
  printf("[Line %3d|%10s] %.*s\n",
      t.on_line,
      TokenTypeTranslation(t.type),
      t.length,
      t.position_in_source);
}
