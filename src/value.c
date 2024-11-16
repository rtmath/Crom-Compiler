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
    case ACT_INT: {
      if (a.is_signed) {
        uint64_t integer = TokenToInt64(t, base);
        ret_val.as.integer = integer;
        ret_val.type = V_INT;

        return ret_val;
      } else {
        uint64_t unsignedint = TokenToUint64(t, base);
        ret_val.as.uinteger = unsignedint;
        ret_val.type = V_UINT;

        return ret_val;
      }
    } break;

    case ACT_FLOAT: {
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

void TruncateValue(Value *value, int bit_width) {
  switch (bit_width) {
    case 8: {
      (value->as.uinteger) &= 0xFF;
    } break;
    case 16: {
      (value->as.uinteger) &= 0xFFFF;
    } break;
    case 32: {
      (value->as.uinteger) &= 0xFFFFFFFF;
    } break;
    case 64: {
      (value->as.uinteger) &= 0xFFFFFFFFFFFFFFFF;
    } break;
    default: {
      printf("Unsupported bit width '%d'\n", bit_width);
    } break;
  }
}

void SetInt(Value *value, int64_t i) {
  (*value).type = V_INT;
  (*value).as.integer = i;
}

void SetUint(Value *value, uint64_t u) {
  (*value).type = V_UINT;
  (*value).as.uinteger = u;
}

void SetBool(Value *value, bool b) {
  (*value).type = V_BOOL;
  (*value).as.boolean = b;
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
      printf("PrintValue(): Value type implemented yet.");
    } break;
  }
}

void PrintValue(Value v) {
  InlinePrintValue(v);
  printf("\n");
}