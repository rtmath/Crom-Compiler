#include <errno.h>
#include <stddef.h> // for NULL
#include <stdlib.h> // for strtoll and friends

#include "common.h"
#include "error.h"

int64_t TokenToInt64(Token t, int base) {
  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToLL() underflow or overflow");
  }

  return value;
}

uint64_t TokenToUint64(Token t, int base) {
  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToULL() underflow or overflow");
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


