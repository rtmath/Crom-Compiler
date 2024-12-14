#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h> // for variadic args, va_list et al.

#include "io.h"
#include "symbol_table.h" // for DebugRegisterSymbolTable
#include "token.h"

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
  ERR_IMPROPER_ACCESS,
  ERR_OVERFLOW,
  ERR_UNDERFLOW,
  ERR_TOO_MANY,
  ERR_TOO_FEW,
  ERR_EMPTY_BODY,
  ERR_UNREACHABLE_CODE,
  ERR_LEXER_ERROR,
  ERR_MISSING_SIZE,
  ERR_MISSING_SEMICOLON,
  ERR_MISSING_RETURN,
  ERR_PEBCAK,
  ERR_MISC,
  ERR_UNKNOWN,
  ERR_COMPILER,
  ERR_INTERPRETER,
} ErrorCode;

void Exit();

void SetErrorCode(ErrorCode code);
const char *ErrorCodeTranslation(ErrorCode code);
ErrorCode ErrorCodeLookup(char *str);

void DebugReportErrorCode();
void DebugRegisterSymbolTable(SymbolTable *st);

/* Helpers for errors that may occur during the normal course of parsing and compiling */
#define ERROR(code, token) Error(__FILE__, __LINE__, __func__, code, token)
#define ERROR_MSG(code, token, msg) ErrorMsg(__FILE__, __LINE__, __func__, code, token, msg)
#define ERROR_FMT(code, token, fmt, ...) ErrorFmt(__FILE__, __LINE__, __func__, code, token, fmt, __VA_ARGS__)
#define ERROR_VALIST(code, token, fmt, valist) ErrorVAList(__FILE__, __LINE__, __func__, code, token, fmt, valist)

void Error(const char *file, int line, const char *func_name, ErrorCode error_code, Token token);
void ErrorMsg(const char *file, int line, const char *func_name, ErrorCode error_code, Token token, const char *msg);
void ErrorFmt(const char *file, int line, const char *func_name, ErrorCode error_code, Token token, const char *fmt, ...);
void ErrorVAList(const char *file, int line, const char *func_name, ErrorCode error_code, Token token, const char *fmt, va_list args);

/* Helpers for errors that originate from the machinery of the compiler */
#define COMPILER_ERROR(msg) ErrorAndExit(__FILE__, __LINE__, ERR_COMPILER, msg)
#define COMPILER_ERROR_FMTMSG(fmt, ...) ErrorAndExit_Variadic(__FILE__, __LINE__, ERR_COMPILER, fmt, __VA_ARGS__)
#define INTERPRETER_ERROR(msg) ErrorAndExit(__FILE__, __LINE__, ERR_INTERPRETER, msg)

void ErrorAndExit(const char *src_filename, int line_number, ErrorCode error_code, const char *msg);
void ErrorAndExit_Variadic(const char *src_filename, int line_number, ErrorCode error_code, const char *fmt_string, ...);

#endif
