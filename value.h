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

typedef struct Value {
  ValueType type;
  ValueType array_type;
  int array_size;
  union {
    uint64_t   uinteger;
    int64_t     integer;
    double     floating;
    char      character;
    char*        string;
    bool        boolean;
    void*     structure;
    struct Value* array;
  } as;
} Value;

Value NewValue(ParserAnnotation a, Token t);

Value AddValues(Value v1, Value v2);
Value SubValues(Value v1, Value v2);
Value MulValues(Value v1, Value v2);
Value DivValues(Value v1, Value v2);
Value ModValues(Value v1, Value v2);

void InlinePrintValue(Value v);
void PrintValue(Value v);

#endif
