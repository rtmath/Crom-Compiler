#include <stdlib.h> // for exit()
#include <stdio.h>  // for printf(), vprintf()

#include "error.h"

static void PrintFormattedStr(const char *fmt_string, va_list args) {
  vprintf(fmt_string, args);
}

void ErrorAndContinue(const char *src_filename, int line_number, const char *msg) {
  printf("[%s:%d] %s\n", src_filename, line_number, msg);
}

void ErrorAndExit(const char* src_filename, int line_number, const char *msg) {
  ErrorAndContinue(src_filename, line_number, msg);

  Exit();
}

void ErrorAndContinue_Variadic(const char *src_filename, int line_number,
                                const char *fmt_string, ...) {
  printf("[%s:%d] ", src_filename, line_number);

  va_list args;
  va_start(args, fmt_string);
  PrintFormattedStr(fmt_string, args);
  va_end(args);

  printf("\n");
}

void ErrorAndExit_Variadic(const char* src_filename, int line_number,
                  const char *fmt_string, ...)
{
  printf("[%s:%d] ", src_filename, line_number);

  va_list args;
  va_start(args, fmt_string);
  PrintFormattedStr(fmt_string, args);
  va_end(args);

  printf("\n");

  Exit();
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

  Exit();
}

void Exit() {
 // This is not a non-zero error message, in order to prevent Make
 // from complaining when executing tests,
  exit(0);
}

void SetErrorCode(ErrorCode *dest, ErrorCode code) {
  // Only set the first encountered error code. This is for testing, where
  // one expected error may cascade into several others.
  if (*dest == ERR_UNSET) *dest = code;
}
