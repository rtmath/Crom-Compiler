#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stdint.h> // for uint64_t et al

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

  T_ENUM,
  T_STRUCT,
  T_VOID,
};

struct FnParam;
struct StructMember;

struct ParamList {
  struct FnParam *next;
};

struct MemberList {
  struct StructMember *next;
};

typedef struct Type {
  enum TypeCategory  category;
  enum TypeSpecifier specifier;

  int array_size;

  struct ParamList params;
  struct MemberList members;
} Type;

typedef struct StructMember {
  struct Type type;
  Token token;
  struct StructMember *next;
} StructMember;

typedef struct FnParam {
  struct Type type;
  Token token;
  struct FnParam *next;
} FnParam;

Type SmallestContainingIntType(int64_t i64);
Type SmallestContainingUintType(uint64_t u64);
Type SmallestContainingFloatType(double d);

Type NoType();
Type NewType(TokenType t);
Type NewArrayType(TokenType t, int size);
Type NewFunctionType(TokenType t);

void InlinePrintType(Type t);
void PrintType(Type t);
const char *TypeTranslation(Type t);

bool TypesMatchExactly(Type t1, Type t2);
bool TypesAreInt(Type t1, Type t2);
bool TypesAreUint(Type t1, Type t2);
bool TypesAreFloat(Type t1, Type t2);

bool TypeIs_None(Type t);

bool TypeIs_Array(Type t);
bool TypeIs_Function(Type t);

bool TypeIs_Numeric(Type t);
bool TypeIs_Signed(Type t);

bool TypeIs_Int(Type t);
bool TypeIs_I8(Type t);
bool TypeIs_I16(Type t);
bool TypeIs_I32(Type t);
bool TypeIs_I64(Type t);

bool TypeIs_Uint(Type t);
bool TypeIs_U8(Type t);
bool TypeIs_U16(Type t);
bool TypeIs_U32(Type t);
bool TypeIs_U64(Type t);

bool TypeIs_Float(Type t);
bool TypeIs_F32(Type t);
bool TypeIs_F64(Type t);

bool TypeIs_Char(Type t);
bool TypeIs_String(Type t);
bool TypeIs_Bool(Type t);

bool TypeIs_Enum(Type t);
bool TypeIs_Struct(Type t);
bool TypeIs_Void(Type t);

bool StructContainsMember(Type struct_type, Token member_name);
void AddMemberToStruct(Type *struct_type, Type member_type, Token member_name);
StructMember *GetStructMember(Type struct_type, Token member_name);

bool FunctionHasParam(Type function_type, Token param_name);
void AddParamToFunction(Type *function_type, Type param_type, Token param_name);
FnParam *GetFunctionParam(Type struct_type, Token param_name);

#endif
