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

void SetErrorCodeIfUnset(ErrorCode *dest, ErrorCode code) {
  // Only set the first encountered error code. This is for testing, where
  // one expected error may cascade into several others.
  if (*dest == ERR_UNSET) *dest = code;
}

void UnsetErrorCode(ErrorCode *dest) {
  *dest = ERR_UNSET;
}

const char *ErrorCodeTranslation(ErrorCode code) {
  switch(code) {
    case ERR_UNSET:                return "UNSET";
    case OK:                       return "OK";
    case ERR_UNDECLARED:           return "UNDECLARED";
    case ERR_UNDEFINED:            return "UNDEFINED";
    case ERR_UNINITIALIZED:        return "UNINITIALIZED";
    case ERR_REDECLARED:           return "REDECLARED";
    case ERR_UNEXPECTED:           return "UNEXPECTED";
    case ERR_TYPE_DISAGREEMENT:    return "TYPE DISAGREEMENT";
    case ERR_IMPROPER_DECLARATION: return "IMPROPER DECLARATION";
    case ERR_IMPROPER_ASSIGNMENT:  return "IMPROPER ASSIGNMENT";
    case ERR_OVERFLOW:             return "OVERFLOW";
    case ERR_UNDERFLOW:            return "UNDERFLOW";
    case ERR_TOO_MANY:             return "TOO MANY";
    case ERR_TOO_FEW:              return "TOO FEW";
    case ERR_EMPTY_BODY:           return "EMPTY BODY";
    case ERR_UNREACHABLE_CODE:     return "UNREACHABLE CODE";
    case ERR_LEXER_ERROR:          return "LEXER ERROR";
    case ERR_MISSING_SEMICOLON:    return "MISSING SEMICOLON";
    case ERR_PEBCAK:               return "PEBCAK";
    case ERR_MISC:                 return "MISC";
    default:                       return "Unhandled ErrorCodeTranslation case";
  }
}
