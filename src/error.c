#include <stdio.h>  // for printf, vprintf
#include <stdlib.h> // for exit()

#include "common.h"
#include "error.h"

static int error_code = OK;

static void PrintFormattedStr(const char *fmt_string, va_list args) {
  Print_VAList(fmt_string, args);
}

void ErrorAndContinue(const char *src_filename, int line_number, const char *msg) {
  Print("[%s:%d] %s\n", src_filename, line_number, msg);
}

void ErrorAndExit(const char* src_filename, int line_number, const char *msg) {
  ErrorAndContinue(src_filename, line_number, msg);

  Exit();
}

void ErrorAndContinue_Variadic(const char *src_filename, int line_number,
                                const char *fmt_string, ...) {
  Print("[%s:%d] ", src_filename, line_number);

  va_list args;
  va_start(args, fmt_string);
  PrintFormattedStr(fmt_string, args);
  va_end(args);

  Print("\n");
}

void ErrorAndExit_Variadic(const char* src_filename, int line_number,
                  const char *fmt_string, ...)
{
  Print("[%s:%d] ", src_filename, line_number);

  va_list args;
  va_start(args, fmt_string);
  PrintFormattedStr(fmt_string, args);
  va_end(args);

  Print("\n");

  Exit();
}

void ErrorAndContinue_VAList(const char *src_filename, int line_number,
                           const char *fmt_string, va_list args) {
  Print("[%s:%d] ", src_filename, line_number);
  PrintFormattedStr(fmt_string, args);
  Print("\n");
}

void ErrorAndExit_VAList(const char *src_filename, int line_number,
                       const char *fmt_string, va_list args) {
  ErrorAndContinue_VAList(src_filename, line_number, fmt_string, args);

  Exit();
}

void Exit() {
  ReportErrorCode();
  exit(error_code);
}

void SetErrorCode(ErrorCode code) {
  // Only set the first encountered error code.
  if (error_code == OK) error_code = code;
}

const char *ErrorCodeTranslation(ErrorCode code) {
  switch(code) {
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
    case ERR_UNKNOWN:              return "UNKNOWN";
    default:                       return "Unhandled ErrorCodeTranslation case";
  }
}

ErrorCode ErrorCodeLookup(char *str) {
  if (StringsMatch(str, "OK")) return OK;
  if (StringsMatch(str, "ERR_UNDECLARED")) return ERR_UNDECLARED;
  if (StringsMatch(str, "ERR_UNDEFINED")) return ERR_UNDEFINED;
  if (StringsMatch(str, "ERR_UNINITIALIZED")) return ERR_UNINITIALIZED;
  if (StringsMatch(str, "ERR_REDECLARED")) return ERR_REDECLARED;
  if (StringsMatch(str, "ERR_UNEXPECTED")) return ERR_UNEXPECTED;
  if (StringsMatch(str, "ERR_TYPE_DISAGREEMENT")) return ERR_TYPE_DISAGREEMENT;
  if (StringsMatch(str, "ERR_IMPROPER_DECLARATION")) return ERR_IMPROPER_DECLARATION;
  if (StringsMatch(str, "ERR_IMPROPER_ASSIGNMENT")) return ERR_IMPROPER_ASSIGNMENT;
  if (StringsMatch(str, "ERR_OVERFLOW")) return ERR_OVERFLOW;
  if (StringsMatch(str, "ERR_UNDERFLOW")) return ERR_UNDERFLOW;
  if (StringsMatch(str, "ERR_TOO_MANY")) return ERR_TOO_MANY;
  if (StringsMatch(str, "ERR_TOO_FEW")) return ERR_TOO_FEW;
  if (StringsMatch(str, "ERR_EMPTY_BODY")) return ERR_EMPTY_BODY;
  if (StringsMatch(str, "ERR_UNREACHABLE_CODE")) return ERR_UNREACHABLE_CODE;
  if (StringsMatch(str, "ERR_LEXER_ERROR")) return ERR_LEXER_ERROR;
  if (StringsMatch(str, "ERR_MISSING_SEMICOLON")) return ERR_MISSING_SEMICOLON;
  if (StringsMatch(str, "ERR_PEBCAK")) return ERR_PEBCAK;
  if (StringsMatch(str, "ERR_MISC")) return ERR_MISC;

  Print("ErrorCodeLookup(): No match for '%s'\n", str);
  return ERR_UNKNOWN;
}

void ReportErrorCode() {
#ifndef RUNNING_TESTS
  printf("%s\n", ErrorCodeTranslation(error_code));
#endif
}
