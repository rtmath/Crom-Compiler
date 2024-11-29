#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

#include "token_type.h"

enum TypeCategory {
  TC_NONE,
  TC_ARRAY,
  TC_FUNCTION,
};

enum TypeSpecifier {
  T_NONE,

  T_I8, T_I16, T_I32, T_I64,
  T_U8, T_U16, T_U32, T_U64,
               T_F32, T_F64,

  T_CHAR, T_STRING,
  T_BOOL,
};

typedef struct {
  enum TypeCategory  category;
  enum TypeSpecifier specifier;

  int array_size;
} Type;

Type NoType();
Type NewType(TokenType t);
Type NewArrayType(TokenType t, int size);
Type NewFunctionType(TokenType t);

void InlinePrintType(Type t);
void PrintType(Type t);

bool TypesEqual(Type t1, Type t2);

bool TypeIs_None(Type t);

bool TypeIs_Array(Type t);

bool TypeIs_Int(Type t);
bool TypeIs_Uint(Type t);
bool TypeIs_Float(Type t);
bool TypeIs_Char(Type t);
bool TypeIs_String(Type t);
bool TypeIs_Bool(Type t);

#endif
