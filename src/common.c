#include <errno.h>
#include <float.h>  // for DBL_MIN
#include <limits.h> // for LLONG_MAX and friends
#include <math.h>   // for HUGE_VAL
#include <stdarg.h> // for va_list and friends
#include <stddef.h> // for NULL
#include <stdio.h>
#include <stdlib.h> // for strtoll and friends
#include <string.h> // for strlen

#include "common.h"
#include "error.h"

static int GetBase(Token t) {
#define BASE_10 10
#define BASE_16 16
#define BASE_2  2

  return (t.type == HEX_LITERAL)
           ? BASE_16
           : (t.type == BINARY_LITERAL)
               ? BASE_2
               : BASE_10;

#undef BASE_10
#undef BASE_16
#undef BASE_2
}

int64_t TokenToInt64(Token t) {
  int base = GetBase(t);

  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  if (errno != 0) {
    SetErrorCode(ERR_OVERFLOW);
    COMPILER_ERROR("TokenToInt64() overflow");
  }

  return value;
}

uint64_t TokenToUint64(Token t) {
  int base = GetBase(t);

  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, base);
  if (errno != 0) {
    SetErrorCode(ERR_OVERFLOW);
    COMPILER_ERROR("TokenToUint64() overflow");
  }

  return value;
}

double TokenToDouble(Token t) {
  errno = 0;
  double value = strtod(t.position_in_source, NULL);
  if (errno != 0) {
    SetErrorCode(ERR_OVERFLOW);
    COMPILER_ERROR("TokenToDouble() underflow or overflow");
  }

  return value;
}

bool Int64Overflow(Token t) {
  int base = GetBase(t);

  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  return (errno == ERANGE && (value == LLONG_MAX ||
                              value == LLONG_MIN));
}

bool Uint64Overflow(Token t) {
  int base = GetBase(t);

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

char *NewString(int size) {
  return malloc(sizeof(char) * size);
}

char *CopyString(const char *s) {
  int length = strlen(s);
  char *new_s = NewString(length + ROOM_FOR_NULL_BYTE);

  for (int i = 0; i < length; i++) {
    new_s[i] = s[i];
  }
  new_s[length] = '\0';

  return new_s;
}

char *CopyStringL(const char *s, int length) {
  char *new_s = NewString(length + ROOM_FOR_NULL_BYTE);

  for (int i = 0; i < length; i++) {
    new_s[i] = s[i];
  }
  new_s[length] = '\0';

  return new_s;
}

char *Concat(char *a, char *b) {
  int a_len = strlen(a) + ROOM_FOR_NULL_BYTE;
  int b_len = strlen(b) + ROOM_FOR_NULL_BYTE;
  int total_len = a_len + b_len;

  char *s = NewString(total_len);

  strncpy(s, a, a_len);
  strncpy(&s[a_len - 1], b, b_len);

  return s;
}

bool StringsMatch(char *a, char *b) {
  return strcmp(a, b) == 0;
}

void Print(const char *fmt, ...) {
#if RUNNING_TESTS
  return;
#endif

  va_list args;
  va_start(args, fmt);

  vprintf(fmt, args);

  va_end(args);
}

void Print_VAList(const char *fmt, va_list args) {
#if RUNNING_TESTS
  return;
#endif

  vprintf(fmt, args);
}
