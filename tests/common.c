#include <stdio.h>
#include <stdlib.h>

#include "common.h"

const char *error_msgs[MAX_ERROR_MESSAGES];
int emi = 0;

bool ASSERT(bool predicate, const char *msg, const char *file, int line) {
  char *str = malloc(sizeof(char) * 100);
  if (!predicate) {
    if (emi < MAX_ERROR_MESSAGES) {
      snprintf(str, 99, "[%s:%d] Assertion failed: %s", file, line, msg);
      error_msgs[emi++] = str;
    }
  }
  return predicate;
}

void PrintErrors() {
  for (int i = 0; i < emi; i++) {
    printf("%s\n", error_msgs[i]);
  }
}
