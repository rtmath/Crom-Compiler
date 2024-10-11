#include <stdarg.h> // for variadic args, va_list et al.
#include <stdlib.h> // for exit()
#include <stdio.h>  // for printf(), vprintf()

#include "error.h"

void ErrorExit(const char* src_filename, int line_number,
               const char *fmt_string, ...)
{
  va_list args;
  va_start(args, fmt_string);

  printf("[%s:%i] ", src_filename, line_number);
  vprintf(fmt_string, args);
  printf("\n");

  va_end(args);

  exit(100);
}
