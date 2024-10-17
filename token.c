#include <stdio.h>

#include "token.h"

void PrintToken(Token t) {
  printf("%s(%.*s)\n",
      TokenTypeTranslation(t.type),
      t.length,
      t.position_in_source);
}
