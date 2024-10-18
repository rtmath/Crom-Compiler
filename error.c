#include <stdlib.h> // for exit()
#include <stdio.h>  // for printf(), vprintf()

#include "error.h"

static void PrintFormattedStr(const char *fmt_string, va_list args) {
  vprintf(fmt_string, args);
}

void ErrorAndContinue(const char *src_filename, int line_number, const char *msg) {
  printf("[%s:%d] %s\n", src_filename, line_number, msg);
}

void ErrorAndExit(const char* src_filename, int line_number, const char *msg)
{
  ErrorAndContinue(src_filename, line_number, msg);

  exit(100);
}


void ErrorAndContinue_Variadic(const char *src_filename, int line_number,
                                const char *fmt_string, ...) {
  // va_list necessary for passing '...' to another function
  va_list args;
  va_start(args, fmt_string);

  printf("[%s:%d] ", src_filename, line_number);
  PrintFormattedStr(fmt_string, args);
  printf("\n");

  va_end(args);
}

void ErrorAndExit_Variadic(const char* src_filename, int line_number,
                  const char *fmt_string, ...)
{
  // va_list necessary for passing '...' to another function
  va_list args;
  va_start(args, fmt_string);

  printf("[%s:%d] ", src_filename, line_number);
  PrintFormattedStr(fmt_string, args);
  printf("\n");

  va_end(args);

  exit(100);
}

void ErrorAndContinue_VAList(const char *src_filename, int line_number,
                           const char *fmt_string, va_list args) {
  printf("[%s:%d] ", src_filename, line_number);
  PrintFormattedStr(fmt_string, args);
  printf("\n");
}

void ErrorAndExit_VAList(const char *src_filename, int line_number,
                       const char *fmt_string, va_list args) {
  ErrorAndContinue_VAList(src_filename, line_number, fmt_string, args);

  exit(100);
}
