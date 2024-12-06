#include <errno.h>
#include <float.h>  // for DBL_MIN
#include <limits.h> // for LLONG_MAX and friends
#include <math.h>   // for HUGE_VAL
#include <stddef.h> // for NULL
#include <stdlib.h> // for strtoll and friends
#include <string.h> // for strlen

#include "common.h"
#include "error.h"

int64_t TokenToInt64(Token t, int base) {
  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToInt64() underflow or overflow");
  }

  return value;
}

uint64_t TokenToUint64(Token t, int base) {
  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToUint64() underflow or overflow");
  }

  return value;
}

double TokenToDouble(Token t) {
  errno = 0;
  double value = strtod(t.position_in_source, NULL);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToDouble() underflow or overflow");
  }

  return value;
}

bool Int64Overflow(Token t, int base) {
  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  return (errno == ERANGE && (value == LLONG_MAX ||
                              value == LLONG_MIN));
}

bool Uint64Overflow(Token t, int base) {
  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, base);
  return (errno == ERANGE && value == ULLONG_MAX);
}

bool DoubleOverflow(Token t) {
  errno = 0;
  double value = strtod(t.position_in_source, NULL);
  return (errno == ERANGE && (value ==  HUGE_VAL ||
                              value == -HUGE_VAL));
}

bool DoubleUnderflow(Token t) {
  errno = 0;
  double value = strtod(t.position_in_source, NULL);
  return (errno == ERANGE && (value <= DBL_MIN));
}

char *CopyString(const char *s) {
  int length = strlen(s);
  char *new_s = malloc(sizeof(char) * (length + ROOM_FOR_NULL_BYTE));

  for (int i = 0; i < length; i++) {
    new_s[i] = s[i];
  }
  new_s[length] = '\0';

  return new_s;
}

char *CopyStringL(const char *s, int length) {
  char *new_s = malloc(sizeof(char) * (length + ROOM_FOR_NULL_BYTE));

  for (int i = 0; i < length; i++) {
    new_s[i] = s[i];
  }
  new_s[length] = '\0';

  return new_s;
}
