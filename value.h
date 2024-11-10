#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

#include "parser_annotation.h"
#include "token.h"

typedef enum {
  V_NONE,
  V_INT,
  V_UINT,
  V_FLOAT,
  V_CHAR,
  V_STRING,
  V_BOOL,
  V_STRUCT,
  V_ARRAY,
} ValueType;

typedef struct {
  ValueType type;
  ValueType array_type;
  union {
    uint64_t uinteger;
    int64_t   integer;
    double   floating;
    char    character;
    char*      string;
    bool      boolean;
    void*   structure;
    void*       array;
  } as;
} Value;

Value NewValue(ParserAnnotation a, Token t);
void InlinePrintValue(Value v);
void PrintValue(Value v);

#endif
