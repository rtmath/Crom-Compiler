#include <errno.h>
#include <stdio.h>  // for printf
#include <stdlib.h> // for strtoll and friends
#include <string.h> // for strcmp

#include "common.h"
#include "error.h"
#include "value.h"

#define BASE_DECIMAL 10
#define BASE_HEX 16
#define BASE_BINARY 2

static long long TokenToLL(Token t, int base) {
  errno = 0;
  long long value = strtoll(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToLL() underflow or overflow");
  }

  return value;
}

static unsigned long long TokenToULL(Token t, int base) {
  errno = 0;
  unsigned long long value = strtoull(t.position_in_source, NULL, base);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToULL() underflow or overflow");
  }

  return value;
}

static double TokenToDouble(Token t) {
  errno = 0;
  double value = strtod(t.position_in_source, NULL);
  if (errno != 0) {
    ERROR_AND_EXIT("TokenToDouble() underflow or overflow");
  }

  return value;
}

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
        long long ll = TokenToLL(t, base);
        ret_val.as.integer = ll;
        ret_val.type = V_INT;

        return ret_val;
      } else {
        unsigned long long ull = TokenToULL(t, base);
        ret_val.as.uinteger = ull;
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

  printf("wtf\n");
  return ret_val;
}

void PrintValue(Value v) {
  switch(v.type) {
    case V_NONE: {
      printf("None\n");
    } break;
    case V_INT: {
      printf("Integer: %ld\n", v.as.integer);
    } break;
    case V_UINT: {
      printf("Unsigned Integer: %lu\n", v.as.uinteger);
    } break;
    case V_FLOAT: {
      printf("Float: %f\n", v.as.floating);
    } break;
    case V_CHAR: {
      printf("Char: %c\n", v.as.character);
    } break;
    case V_STRING: {
      printf("String: %s\n", v.as.string);
    } break;
    case V_BOOL: {
      printf("Bool: %s\n", (v.as.boolean) ? "true" : "false");
    } break;
    default: {
      printf("PrintValue(): Value type implemented yet\n");
    } break;
  }
}
