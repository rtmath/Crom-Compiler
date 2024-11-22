#include <errno.h>
#include <stdio.h>  // for printf
#include <stdlib.h> // for malloc
#include <string.h> // for strcmp

#include "common.h"
#include "error.h"
#include "value.h"

#define BASE_DECIMAL 10
#define BASE_HEX 16
#define BASE_BINARY 2

static char *ExtractString(Token token) {
  char *str = malloc(sizeof(char) * (token.length + ROOM_FOR_NULL_BYTE));
  for (int i = 0; i < token.length; i++) {
    str[i] = token.position_in_source[i];
  }
  str[token.length] = '\0';

  return str;
}

Value NewValue(ParserAnnotation a, Token t) {
  const int base =
    (t.type == HEX_LITERAL)
    ? BASE_HEX
    : (t.type == BINARY_LITERAL)
      ? BASE_BINARY
      : BASE_DECIMAL;

  Value ret_val = { 0 };

  switch(a.actual_type) {
    case ACT_NOT_APPLICABLE: {
      return ret_val;
    } break;
    case ACT_INT: {
      if (a.is_signed) {
        if (Int64Overflow(t, base)) {
          ERROR_AT_TOKEN(t, "I64 Overflow\n", "");
          return (Value){ .type = V_OVERFLOW, .as.integer = 0 };
        }

        uint64_t integer = TokenToInt64(t, base);
        ret_val.as.integer = integer;
        ret_val.type = V_INT;

        return ret_val;
      } else {
        if (Uint64Overflow(t, base)) {
          ERROR_AT_TOKEN(t, "U64 Overflow\n", "");
          return (Value){ .type = V_OVERFLOW, .as.uinteger = 0 };
        }

        uint64_t unsignedint = TokenToUint64(t, base);
        ret_val.as.uinteger = unsignedint;
        ret_val.type = V_UINT;

        return ret_val;
      }
    } break;

    case ACT_FLOAT: {
      if (DoubleOverflow(t) || DoubleUnderflow(t)) {
        ERROR_AT_TOKEN(t, "F64 Over/Underflow\n", "");
        return (Value){ .type = V_OVERFLOW, .as.floating = 0 };
      }

      double d = TokenToDouble(t);
      ret_val.as.floating = d;
      ret_val.type = V_FLOAT;

      return ret_val;
    } break;

    case ACT_BOOL: {
      char *s = ExtractString(t);
      ret_val.as.boolean = (strcmp(s, "true") == 0) ? true : false;
      ret_val.type = V_BOOL;
      free(s);

      return ret_val;
    } break;

    case ACT_CHAR: {
      char *s = ExtractString(t);
      ret_val.as.character = s[0];
      ret_val.type = V_CHAR;
      free(s);

      return ret_val;
    } break;

    case ACT_STRING: {
      char *s = ExtractString(t);
      ret_val.as.string = s;
      ret_val.type = V_STRING;
      ret_val.array_type = V_CHAR;
      ret_val.array_size = strlen(s);

      return ret_val;
    } break;

    default: {
      ERROR_AND_EXIT_FMTMSG(
        "NewValue(): '%s' not implemented yet",
        AnnotationTranslation(a));
    } break;
  }

  return ret_val;
}

Value NewIntValue(int64_t i) {
  return (Value){
    .type = V_INT,
    .array_type = 0,
    .array_size = 0,
    .as.integer = i,
  };
}

Value NewUintValue(uint64_t u) {
  return (Value){
    .type = V_UINT,
    .array_type = 0,
    .array_size = 0,
    .as.uinteger = u,
  };
}

Value NewFloatValue(double d) {
  return (Value){
    .type = V_FLOAT,
    .array_type = 0,
    .array_size = 0,
    .as.floating = d,
  };
}
Value NewCharValue(char c) {
  return (Value){
    .type = V_CHAR,
    .array_type = 0,
    .array_size = 0,
    .as.character = c,
  };
}

Value NewStringValue(const char *s) {
  return (Value){
    .type = V_STRING,
    .array_type = V_CHAR,
    .array_size = strlen(s),
    .as.string = s,
  };
}

Value NewBoolValue(bool b)  {
  return (Value){
    .type = V_BOOL,
    .array_type = 0,
    .array_size = 0,
    .as.boolean = b,
  };
}

Value AddValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("AddValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer + v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger + v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating + v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("AddValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value SubValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("SubValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer - v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger - v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating - v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("SubValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value MulValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("MulValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer * v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger * v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating * v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("MulValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value DivValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("DivValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer / v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger / v2.as.uinteger
      };
    } break;
    case V_FLOAT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.floating = v1.as.floating / v2.as.floating
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("DivValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value ModValues(Value v1, Value v2) {
  if (v1.type != v2.type) ERROR_AND_EXIT("ModValues(): Type mismatch");

  switch(v1.type) {
    case V_INT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.integer = v1.as.integer % v2.as.integer
      };
    } break;
    case V_UINT: {
      return (Value){
        .type = V_INT,
        .array_type = V_NONE,
        .as.uinteger = v1.as.uinteger % v2.as.uinteger
      };
    } break;
    default: ERROR_AND_EXIT_FMTMSG("ModValues(): Invalid type %d", v1.type);
  }

  return (Value){ .type = 0, .array_type = 0, .as.integer = 0 };
}

Value GreaterThan(Value v1, Value v2) {
  if (v1.type == V_INT) {
    return NewBoolValue(v1.as.integer > v2.as.integer);
  }

  if (v1.type == V_UINT) {
    return NewBoolValue(v1.as.uinteger > v2.as.uinteger);
  }

  if (v1.type == V_FLOAT) {
    return NewBoolValue(v1.as.floating > v2.as.floating);
  }

  ERROR_AND_EXIT_FMTMSG("Invalid type %d passed to GreaterThan()\n", v1.type);
  return (Value){0};
}

Value LessThan(Value v1, Value v2) {
  if (v1.type == V_INT) {
    return NewBoolValue(v1.as.integer < v2.as.integer);
  }

  if (v1.type == V_UINT) {
    return NewBoolValue(v1.as.uinteger < v2.as.uinteger);
  }

  if (v1.type == V_FLOAT) {
    return NewBoolValue(v1.as.floating < v2.as.floating);
  }

  ERROR_AND_EXIT_FMTMSG("Invalid type %d passed to LessThan()\n", v1.type);
  return (Value){0};
}

Value LogicalAND(Value v1, Value v2) {
  if (v1.type != V_BOOL || v2.type != V_BOOL) ERROR_AND_EXIT("LogicalAND(): Cannot compare non-bool types");
  if (v1.type != v2.type) ERROR_AND_EXIT("LogicalAND(): Type mismatch");

  return (Value){
    .type = V_BOOL,
    .array_type = 0,
    .as.boolean = v1.as.boolean && v2.as.boolean,
  };
}

Value LogicalOR(Value v1, Value v2) {
  if (v1.type != V_BOOL || v2.type != V_BOOL) ERROR_AND_EXIT("LogicalAND(): Cannot compare non-bool types");
  if (v1.type != v2.type) ERROR_AND_EXIT("LogicalAND(): Type mismatch");

  return (Value){
    .type = V_BOOL,
    .array_type = 0,
    .as.boolean = v1.as.boolean || v2.as.boolean,
  };
}

void InlinePrintValue(Value v) {
  switch(v.type) {
    case V_NONE: {
      printf("None");
    } break;
    case V_INT: {
      printf("Integer: %ld", v.as.integer);
    } break;
    case V_UINT: {
      printf("Unsigned Integer: %lu", v.as.uinteger);
    } break;
    case V_FLOAT: {
      printf("Float: %f", v.as.floating);
    } break;
    case V_CHAR: {
      printf("Char: %c", v.as.character);
    } break;
    case V_STRING: {
      printf("String: %s", v.as.string);
    } break;
    case V_BOOL: {
      printf("Bool: %s", (v.as.boolean) ? "true" : "false");
    } break;
    case V_ARRAY: {
      printf("Array [");
      InlinePrintValue(v.as.array[0]);
      if (v.array_size > 1) {
        printf(" .. ");
        InlinePrintValue(v.as.array[v.array_size - 1]);
      }
      printf("]");
    } break;
    default: {
      printf("PrintValue(): Value type '%d' not implemented yet.", v.type);
    } break;
  }
}

void PrintValue(Value v) {
  InlinePrintValue(v);
  printf("\n");
}
