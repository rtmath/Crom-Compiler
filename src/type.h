#ifndef TYPE_H
#define TYPE_H

enum TypeCategory {
  TC_NONE,
  TC_ARRAY,
  TC_FUNCTION,
};

enum TypeSpecifier {
  T_NONE,

  T_I8,
  T_I16,
  T_I32,
  T_I64,

  T_U8,
  T_U16,
  T_U32,
  T_U64,

  T_F32,
  T_F64,

  T_BOOL,

  T_CHAR,
  T_STRING,

  T_ENUM,
  T_STRUCT,
};

typedef struct {
  enum TypeCategory  category;
  enum TypeSpecifier specifier;

  int array_size;
} Type;

#endif
