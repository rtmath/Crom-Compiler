#include <float.h> // FLT_MAX and DBL_MAX
#include <stdlib.h> // for calloc
#include <string.h> // for strcmp

#include "common.h"
#include "error.h"
#include "type.h"

static Type _Type(enum TypeSpecifier type_specifier, enum TypeCategory type_category, int array_size) {
  return (Type){
    .category = type_category,
    .specifier = type_specifier,

    .array_size = array_size,
  };
}

Type _NewType(TokenType t, int array_size) {
  const int _ = TC_NONE;

  switch(t) {
    case I8:  return _Type(T_I8,  _, array_size);
    case I16: return _Type(T_I16, _, array_size);
    case I32: return _Type(T_I32, _, array_size);
    case I64: return _Type(T_I64, _, array_size);

    case U8:  return _Type(T_U8,  _, array_size);
    case U16: return _Type(T_U16, _, array_size);
    case U32: return _Type(T_U32, _, array_size);
    case U64: return _Type(T_U64, _, array_size);

    case F32:  return _Type(T_F32, _, array_size);
    case F64:
    case FLOAT_LITERAL: return _Type(T_F64, _, array_size);

    case BINARY_LITERAL:
    case HEX_LITERAL: return _Type(T_U64, _, array_size);

    case INT_LITERAL:
    case ENUM_LITERAL: return _Type(T_I64, _, array_size);

    case BOOL:
    case BOOL_LITERAL: return _Type(T_BOOL, _, array_size);

    case CHAR:
    case CHAR_LITERAL: return _Type(T_CHAR, _, array_size);

    case STRING:
    case STRING_LITERAL: return _Type(T_STRING, TC_ARRAY, array_size);

    case VOID: return _Type(T_VOID, _, array_size);
    case ENUM: return _Type(T_ENUM, _, array_size);
    case STRUCT: return _Type(T_STRUCT, _, array_size);

    default:
      COMPILER_ERROR_FMTMSG("NewType(): Invalid token type '%s'\n", TokenTypeTranslation(t));
      return _Type(T_NONE, _, 0);
  }
}

int GetTypeBitWidth(Type t) {
  if (t.specifier == T_I8  || t.specifier == T_U8) return 8;
  if (t.specifier == T_I16 || t.specifier == T_U16) return 16;
  if (t.specifier == T_I32 || t.specifier == T_U32 || t.specifier == T_F32) return 32;
  if (t.specifier == T_I64 || t.specifier == T_U64 || t.specifier == T_F64) return 64;

  // TODO: Implement other types?
  return 0;
}

Type SmallestContainingIntType(int64_t i64) {
  if (i64 >= INT8_MIN  && i64 <= INT8_MAX)  return NewType(I8);
  if (i64 >= INT16_MIN && i64 <= INT16_MAX) return NewType(I16);
  if (i64 >= INT32_MIN && i64 <= INT32_MAX) return NewType(I32);
  return NewType(I64);
}

Type SmallestContainingUintType(uint64_t u64) {
  if (u64 <= UINT8_MAX)  return NewType(U8);
  if (u64 <= UINT16_MAX) return NewType(U16);
  if (u64 <= UINT32_MAX) return NewType(U32);
  return NewType(U64);
}

Type SmallestContainingFloatType(double d) {
  if (d >= -FLT_MAX && d <= FLT_MAX) return NewType(F32);
  return NewType(F64);
}

Type NoType() {
  return (Type){0};
}

Type NewType(TokenType t) {
  return _NewType(t, 0);
}

Type NewArrayType(TokenType t, int size) {
  Type type = _NewType(t, size);
  type.category = TC_ARRAY;

  return type;
}

Type NewFunctionType(TokenType t) {
  Type type = _NewType(t, 0);
  type.category = TC_FUNCTION;

  return type;
}

Type EnumMemberType(Type t) {
  t.category = TC_ENUM_MEMBER;
  return t;
}

void InlinePrintType(Type t) {
  if (t.category == TC_FUNCTION) {
    Print("Fn::");
  }

  switch (t.specifier) {
    case T_NONE: Print("NONE"); break;

    case T_I8:  Print("I8");  break;
    case T_I16: Print("I16"); break;
    case T_I32: Print("I32"); break;
    case T_I64: Print("I64"); break;

    case T_U8:  Print("U8");  break;
    case T_U16: Print("U16"); break;
    case T_U32: Print("U32"); break;
    case T_U64: Print("U64"); break;

    case T_F32: Print("F32"); break;
    case T_F64: Print("F64"); break;

    case T_CHAR: Print("char"); break;
    case T_STRING: Print("string"); break;
    case T_BOOL: Print("bool"); break;

    case T_ENUM: Print("enum"); break;
    case T_STRUCT: {
      if (t.specifier == T_STRUCT) {
        StructMember *member = t.members.next;

        if (member != NULL) Print(" { ");
        while (member != NULL) {
          InlinePrintType(member->type);
          Print(" ");
          member = member->next;
        }
        if (t.members.next != NULL) Print("}");
      }
    } break;
    case T_VOID: Print("void"); break;
  }

  if (t.category == TC_ARRAY) {
    Print("[%d]", t.array_size);
  }
}

void PrintType(Type t) {
  InlinePrintType(t);
  Print("\n");
}

const char *TypeCategoryTranslation(Type t) {
  switch (t.category) {
    case TC_NONE: return "NONE";
    case TC_ARRAY: return "ARRAY";
    case TC_FUNCTION: return "FUNCTION";
    case TC_ENUM_MEMBER: return "ENUM MEMBER";
    default: return "NOT FOUND";
  }
}

const char *TypeTranslation(Type t) {
  switch (t.specifier) {
    case T_NONE: return "NONE";

    case T_I8:  return "I8";
    case T_I16: return "I16";
    case T_I32: return "I32";
    case T_I64: return "I64";

    case T_U8:  return "U8";
    case T_U16: return "U16";
    case T_U32: return "U32";
    case T_U64: return "U64";

    case T_F32: return "F32";
    case T_F64: return "F64";

    case T_CHAR: return "char";
    case T_STRING: return "string";
    case T_BOOL: return "bool";

    case T_ENUM: return "enum";
    case T_STRUCT: return "struct";
    case T_VOID: return "void";
  }

  return "";
}

bool TypesAreInt(Type t1, Type t2) {
  return TypeIs_Int(t1) && TypeIs_Int(t2);
}

bool TypesAreUint(Type t1, Type t2) {
  return TypeIs_Uint(t1) && TypeIs_Uint(t2);
}

bool TypesAreFloat(Type t1, Type t2) {
  return TypeIs_Float(t1) && TypeIs_Float(t2);
}

bool TypesMatchExactly(Type t1, Type t2) {
  return t1.category  == t2.category &&
         t1.specifier == t2.specifier;
}

bool TypeIs_None(Type t) {
  return t.specifier == T_NONE;
}

bool TypeIs_Array(Type t) {
  return t.category == TC_ARRAY;
}

bool TypeIs_Function(Type t) {
  return t.category == TC_FUNCTION;
}

bool TypeIs_Numeric(Type t) {
  return TypeIs_Int(t) || TypeIs_Uint(t) || TypeIs_Float(t);
}

bool TypeIs_Signed(Type t) {
  return TypeIs_Int(t) || TypeIs_Float(t);
}

bool TypeIs_Int(Type t) {
  return t.specifier == T_I8  ||
         t.specifier == T_I16 ||
         t.specifier == T_I32 ||
         t.specifier == T_I64;
}

bool TypeIs_I8(Type t) {
  return t.specifier == T_I8;
}

bool TypeIs_I16(Type t) {
  return t.specifier == T_I16;
}

bool TypeIs_I32(Type t) {
  return t.specifier == T_I32;
}

bool TypeIs_I64(Type t) {
  return t.specifier == T_I64;
}

bool TypeIs_Uint(Type t) {
  return t.specifier == T_U8  ||
         t.specifier == T_U16 ||
         t.specifier == T_U32 ||
         t.specifier == T_U64;
}

bool TypeIs_U8(Type t) {
  return t.specifier == T_U8;
}

bool TypeIs_U16(Type t) {
  return t.specifier == T_U16;
}

bool TypeIs_U32(Type t) {
  return t.specifier == T_U32;
}

bool TypeIs_U64(Type t) {
  return t.specifier == T_U64;
}

bool TypeIs_Float(Type t) {
  return t.specifier == T_F32 || t.specifier == T_F64;
}

bool TypeIs_F32(Type t) {
  return t.specifier == T_F32;
}

bool TypeIs_F64(Type t) {
  return t.specifier == T_F64;
}

bool TypeIs_Char(Type t) {
  return t.specifier == T_CHAR;
}

bool TypeIs_String(Type t) {
  return t.specifier == T_STRING;
}

bool TypeIs_Bool(Type t) {
  return t.specifier == T_BOOL;
}

bool TypeIs_Enum(Type t) {
  return t.specifier == T_ENUM;
}

bool TypeIs_EnumMember(Type t) {
  return t.category == TC_ENUM_MEMBER;
}

bool TypeIs_Struct(Type t) {
  return t.specifier == T_STRUCT;
}

bool TypeIs_Void(Type t) {
  return t.specifier == T_VOID;
}

static StructMember *NewStructMember(Type type, Token token) {
  StructMember *struct_member= calloc(1, sizeof(StructMember));

  struct_member->type = type;
  struct_member->token = token;

  return struct_member;
}

StructMember *GetStructMember(Type struct_type, Token member_name) {
  char *ident = CopyStringL(member_name.position_in_source,
                            member_name.length);
  StructMember *matching_member = NULL;
  StructMember *check = struct_type.members.next;

  while (check != NULL) {
    char *name= CopyStringL(check->token.position_in_source,
                            check->token.length);

    if (strcmp(ident, name) == 0) {
      matching_member = struct_type.members.next;
      break;
    }

    check = check->next;

    free(name);
  }

  free(ident);

  return matching_member;
}

bool StructContainsMember(Type struct_type, Token member_name) {
  char *ident = CopyStringL(member_name.position_in_source,
                            member_name.length);
  StructMember *matching_member = NULL;
  StructMember *check = struct_type.members.next;

  while (check != NULL) {
    char *name= CopyStringL(check->token.position_in_source,
                            check->token.length);

    if (strcmp(ident, name) == 0) {
      matching_member = check;
      break;
    }

    check = check->next;

    free(name);
  }

  free(ident);

  return matching_member != NULL;
}

void AddMemberToStruct(Type *struct_type, Type member_type, Token member_name) {
  StructMember *check = struct_type->members.next;

  // If first member in list
  if (check == NULL) {
    struct_type->members.next = NewStructMember(member_type, member_name);
    return;
  }

  // Scan to end of members
  while (check->next != NULL) {
    check = check->next;
  }

  // Add member
  check->next = NewStructMember(member_type, member_name);
}

static FnParam *NewFnParam(Type type, Token token) {
  FnParam *fn_param = calloc(1, sizeof(FnParam));

  fn_param->type = type;
  fn_param->token = token;

  return fn_param;
}

bool FunctionHasParam(Type function_type, Token param_name) {
  char *ident = CopyStringL(param_name.position_in_source,
                            param_name.length);
  FnParam *matching_param = NULL;
  FnParam *check = function_type.params.next;

  while (check != NULL) {
    char *name = CopyStringL(check->token.position_in_source,
                             check->token.length);

    if (strcmp(ident, name) == 0) {
      matching_param = function_type.params.next;
      break;
    }

    check = (*check).next;

    free(name);
  }

  free(ident);

  return matching_param != NULL;
}

void AddParamToFunction(Type *function_type, Type param_type, Token param_name) {
  FnParam *check = function_type->params.next;

  // If first param in list
  if (check == NULL) {
    function_type->params.next = NewFnParam(param_type, param_name);
    return;
  }

  // Scan to end of param list
  while (check->next != NULL) {
    check = check->next;
  }

  // Add param
  check->next = NewFnParam(param_type, param_name);
}

FnParam *GetFunctionParam(Type function_type, Token param_name) {
  char *ident = CopyStringL(param_name.position_in_source,
                            param_name.length);
  FnParam *matching_param = NULL;
  FnParam *check = function_type.params.next;

  while (check != NULL) {
    char *name = CopyStringL(check->token.position_in_source,
                             check->token.length);

    if (strcmp(ident, name) == 0) {
      matching_param = function_type.params.next;
      break;
    }

    check = (*check).next;

    free(name);
  }

  free(ident);

  return matching_param;
}
