#include <stdarg.h> // for variadic args, va_list et al.
#include <stdlib.h> // for exit()
#include <stdio.h>  // for printf(), vprintf()

#include "error.h"

static void PrintVariadic(const char *fmt_string, va_list args) {
  vprintf(fmt_string, args);
}

void ErrorAndContinue(const char *src_filename, int line_number,
                      const char *fmt_string, ...) {
  // va_list necessary for passing '...' to another function
  va_list args;
  va_start(args, fmt_string);

  printf("[%s:%i] ", src_filename, line_number);
  PrintVariadic(fmt_string, args);
  printf("\n");

  va_end(args);
}

void ErrorAndExit(const char* src_filename, int line_number,
                  const char *fmt_string, ...)
{
  // va_list necessary for passing '...' to another function
  va_list args;
  va_start(args, fmt_string);

  ErrorAndContinue(src_filename, line_number, fmt_string, args);

  va_end(args);

  exit(100);
}
