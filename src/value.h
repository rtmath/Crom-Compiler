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
  V_OVERFLOW,
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
    const char*  string;
    bool        boolean;
    void*     structure;
    struct Value* array;
  } as;
} Value;

Value NewValue(ParserAnnotation a, Token t);
Value NewIntValue(int64_t i);
Value NewUintValue(uint64_t u);
Value NewFloatValue(double d);
Value NewCharValue(char c);
Value NewStringValue(const char *s);
Value NewBoolValue(bool b);

Value AddValues(Value v1, Value v2);
Value SubValues(Value v1, Value v2);
Value MulValues(Value v1, Value v2);
Value DivValues(Value v1, Value v2);
Value ModValues(Value v1, Value v2);

Value LogicalOR(Value v1, Value v2);
Value LogicalAND(Value v1, Value v2);
Value GreaterThan(Value v1, Value v2);
Value LessThan(Value v1, Value v2);

void SetInt(Value *value, int64_t i);
void SetUint(Value *value, uint64_t u);
void SetBool(Value *value, bool b);

void InlinePrintValue(Value v);
void PrintValue(Value v);

#endif
