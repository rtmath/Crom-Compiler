#include <stdio.h>  // for printf, vprintf
#include <stdlib.h> // for exit()

#include "common.h"
#include "error.h"

static SymbolTable *debug_symbol_table = NULL;
static int error_code = OK;

void Exit() {
  DebugReportErrorCode();
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
    case ERR_MISSING_RETURN:       return "MISSING RETURN";
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
  if (StringsMatch(str, "ERR_MISSING_RETURN")) return ERR_MISSING_RETURN;
  if (StringsMatch(str, "ERR_PEBCAK")) return ERR_PEBCAK;
  if (StringsMatch(str, "ERR_MISC")) return ERR_MISC;

  Print("ErrorCodeLookup(): No match for '%s'\n", str);
  return ERR_UNKNOWN;
}

void DebugRegisterSymbolTable(SymbolTable *st) {
  debug_symbol_table = st;
}

void DebugPrintSymbolsOnExit() {
  if (debug_symbol_table == NULL) return;

  PrintAllSymbols(debug_symbol_table);
}

void DebugReportErrorCode() {
#ifndef RUNNING_TESTS
  DebugPrintSymbolsOnExit();
  printf("\nExit Code: %s\n", ErrorCodeTranslation(error_code));
#endif
}

void Error(const char *file, int line, const char *func_name, ErrorCode error_code, Token token) {
  SetErrorCode(error_code);
  PrintSourceLineOfToken(token);

  Print("[%s:%d] %s(): ", file, line, func_name);

  switch (error_code) {
    case OK: {
      // This shouldn't ever happen
    } break;
    case ERR_UNDECLARED: {
      Print("Undeclared identifier '%.*s'", token.length, token.position_in_source);
    } break;
    case ERR_UNDEFINED: {
      Print("Undefined identifier '%.*s'", token.length, token.position_in_source);
    } break;
    case ERR_UNINITIALIZED: {
      Print("Uninitialized identifier '%.*s'", token.length, token.position_in_source);
    } break;
    case ERR_REDECLARED: {
      Symbol s = RetrieveFrom(debug_symbol_table, token);
      Print("Redeclaration of '%.*s', originally declared on line '%d'",
            token.length, token.position_in_source, s.declared_on_line);
      PrintSourceLineOfToken(s.token);
    } break;
    case ERR_UNEXPECTED: {
      Print("Unexpected token '%.*s'", token.length, token.position_in_source);
    } break;
    case ERR_TYPE_DISAGREEMENT: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_IMPROPER_DECLARATION: {
      Print("Improper declaration");
    } break;
    case ERR_IMPROPER_ASSIGNMENT: {
      Print("Improper assignment to identifier '.*s'", token.length, token.position_in_source);
    } break;
    case ERR_OVERFLOW: {
      Print("Overflow");
    } break;
    case ERR_UNDERFLOW: {
      Print("Underflow");
    } break;
    case ERR_TOO_MANY: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_TOO_FEW: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_EMPTY_BODY: {
      Print("Body cannot be empty");
    } break;
    case ERR_UNREACHABLE_CODE: {
      Print("Unreachable code in '%s'", func_name);
    } break;
    case ERR_LEXER_ERROR: {
      Print("Encountered error token: '%.*s'", token.length, token.position_in_source);
    } break;
    case ERR_MISSING_SEMICOLON: {
      Print("Expected semicolon");
    } break;
    case ERR_MISSING_RETURN: {
      Print("Missing return in non-void function '%s'", func_name);
    } break;
    case ERR_PEBCAK: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_MISC: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_UNKNOWN: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_COMPILER: {
      // This maybe shouldn't be handled in this function
    } break;
    case ERR_INTERPRETER: {
      // This maybe shouldn't be handled in this function
    } break;
  }

  Print("\n");
  Exit();
}

void ErrorMsg(const char *file, int line, const char *func_name,
              ErrorCode error_code, Token token, const char *msg) {
  SetErrorCode(error_code);
  PrintSourceLineOfToken(token);

  Print("[%s:%d] %s(): ", file, line, func_name);
  Print(msg);

  Print("\n");
  Exit();
}

void ErrorFmt(const char *file, int line, const char *func_name,
              ErrorCode error_code, Token token, const char *fmt, ...) {
  SetErrorCode(error_code);
  PrintSourceLineOfToken(token);

  Print("[%s:%d] %s(): ", file, line, func_name);
  va_list args;
  va_start(args, fmt);
  Print_VAList(fmt, args);
  va_end(args);

  Print("\n");
  Exit();
}

void ErrorVAList(const char *file, int line, const char *func_name,
                 ErrorCode error_code, Token token, const char *fmt, va_list args) {
  SetErrorCode(error_code);
  PrintSourceLineOfToken(token);

  Print("[%s:%d] %s(): ", file, line, func_name);
  Print_VAList(fmt, args);

  Print("\n");
  Exit();
}

void ErrorAndExit(const char* src_filename, int line_number, ErrorCode error_code, const char *msg) {
  SetErrorCode(error_code);
  Print("[%s:%d] %s\n", src_filename, line_number, msg);

  Exit();
}

void ErrorAndExit_Variadic(const char* src_filename, int line_number, ErrorCode error_code, const char *fmt_string, ...) {
  SetErrorCode(error_code);
  Print("[%s:%d] ", src_filename, line_number);

  va_list args;
  va_start(args, fmt_string);
  Print_VAList(fmt_string, args);
  va_end(args);

  Print("\n");

  Exit();
}
