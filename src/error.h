#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h> // for variadic args, va_list et al.

#include "io.h"
#include "symbol_table.h" // for DebugRegisterSymbolTable

typedef enum {
  OK, // No error occurred.
  ERR_UNDECLARED,
  ERR_UNDEFINED,
  ERR_UNINITIALIZED,
  ERR_REDECLARED,
  ERR_UNEXPECTED,
  ERR_TYPE_DISAGREEMENT,
  ERR_IMPROPER_DECLARATION,
  ERR_IMPROPER_ASSIGNMENT,
  ERR_OVERFLOW,
  ERR_UNDERFLOW,
  ERR_TOO_MANY,
  ERR_TOO_FEW,
  ERR_EMPTY_BODY,
  ERR_UNREACHABLE_CODE,
  ERR_LEXER_ERROR,
  ERR_MISSING_SEMICOLON,
  ERR_PEBCAK,
  ERR_MISC,
  ERR_UNKNOWN,
} ErrorCode;

#define ERROR_AND_CONTINUE(msg) ErrorAndContinue(__FILE__, __LINE__, msg)
#define ERROR_AND_EXIT(msg) ErrorAndExit(__FILE__, __LINE__, msg)
#define ERROR_AND_CONTINUE_FMTMSG(fmt, ...) ErrorAndContinue_Variadic(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define ERROR_AND_EXIT_FMTMSG(fmt, ...) ErrorAndExit_Variadic(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define ERROR_AND_CONTINUE_VALIST(fmt, valist) ErrorAndContinue_VAList(__FILE__, __LINE__, fmt, valist)
#define ERROR_AND_EXIT_VALIST(fmt, valist) ErrorAndExit_VAList(__FILE__, __LINE__, fmt, valist)

#define ERROR_AT_TOKEN(token, error_code, fmt, ...) {  \
  SetErrorCode(error_code);                \
  PrintSourceLineOfToken(token);           \
  ERROR_AND_EXIT_FMTMSG(fmt, __VA_ARGS__); \
}

#define ERROR_AT_TOKEN_VALIST(token, error_code, fmt, valist) { \
  SetErrorCode(error_code);           \
  PrintSourceLineOfToken(token);      \
  ERROR_AND_EXIT_VALIST(fmt, valist); \
}

#define REDECLARATION_AT_TOKEN(offending, original, fmt, ...) {  \
  SetErrorCode(ERR_REDECLARED);                \
  PrintSourceLineOfToken(offending);           \
  ERROR_AND_CONTINUE_FMTMSG(fmt, __VA_ARGS__); \
  PrintSourceLineOfToken(original);            \
  Exit();                                      \
}

void ErrorAndContinue(const char *src_filename, int line_number, const char *msg);
void ErrorAndExit(const char *src_filename, int line_number, const char *msg);

void ErrorAndContinue_Variadic(const char *src_filename, int line_number, const char *fmt_string, ...);
void ErrorAndExit_Variadic(const char *src_filename, int line_number, const char *fmt_string, ...);

void ErrorAndContinue_VAList(const char *src_filename, int line_number, const char *fmt_string, va_list args);
void ErrorAndExit_VAList(const char *src_filename, int line_number, const char *fmt_string, va_list args);

void Exit();

void SetErrorCode(ErrorCode code);

const char *ErrorCodeTranslation(ErrorCode code);
ErrorCode ErrorCodeLookup(char *str);

void DebugReportErrorCode();
void DebugRegisterSymbolTable(SymbolTable *st);

#endif
