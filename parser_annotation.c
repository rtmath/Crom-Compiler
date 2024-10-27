#include <stdio.h> // for printf

#include "error.h"
#include "parser_annotation.h"

ParserAnnotation NoAnnotation() {
  ParserAnnotation a = {
    .ostensible_type = OST_UNKNOWN,
    .bit_width = 0,
    .is_signed = 0,
    .declared_on_line = -1,
    .is_array = 0,
    .array_size = 0,
  };

  return a;
}

static void _InlinePrintAnnotation(const char *s, ParserAnnotation a) {
  (a.is_function)
  ? printf("[Fn :: %s%d]", s, a.bit_width)
  : (a.is_array)
    ? printf("[%s[%d]]}", s, a.array_size)
    : (a.bit_width > 0)
      ? printf("[%s%d]", s, a.bit_width)
      : printf("[%s]", s);
}

void InlinePrintAnnotation(ParserAnnotation a) {
  switch (a.ostensible_type) {
    case OST_UNKNOWN: {
    } break;
    case OST_INT: {
      _InlinePrintAnnotation((a.is_signed) ? "I" : "U", a);
    } break;
    case OST_FLOAT: {
      _InlinePrintAnnotation("F", a);
    } break;
    case OST_BOOL: {
      _InlinePrintAnnotation("BOOL", a);
    } break;
    case OST_CHAR: {
      _InlinePrintAnnotation("CHAR", a);
    } break;
    case OST_STRING: {
      _InlinePrintAnnotation("STRING", a);
    } break;
    case OST_VOID: {
      _InlinePrintAnnotation("VOID", a);
    } break;
    case OST_ENUM: {
      _InlinePrintAnnotation("ENUM", a);
    } break;
    case OST_STRUCT: {
      _InlinePrintAnnotation("STRUCT", a);
    } break;
  }
}

static ParserAnnotation Annotation(OstensibleType type, int bit_width, bool is_signed) {
  ParserAnnotation a = NoAnnotation();

  a.ostensible_type = type;
  a.bit_width = bit_width;
  a.is_signed = is_signed;

  return a;
}

ParserAnnotation AnnotateType(TokenType t) {
  const bool SIGNED = true;
  const bool UNSIGNED = false;

  switch (t) {
    case I8:  return Annotation(OST_INT,  8, SIGNED);
    case I16: return Annotation(OST_INT, 16, SIGNED);
    case I32: return Annotation(OST_INT, 32, SIGNED);
    case I64: return Annotation(OST_INT, 64, SIGNED);
    case U8:  return Annotation(OST_INT,  8, UNSIGNED);
    case U16: return Annotation(OST_INT, 16, UNSIGNED);
    case U32: return Annotation(OST_INT, 32, UNSIGNED);
    case U64: return Annotation(OST_INT, 64, UNSIGNED);
    case F32: return Annotation(OST_FLOAT, 32, SIGNED);
    case F64: return Annotation(OST_FLOAT, 32, SIGNED);
    case BOOL: return Annotation(OST_BOOL, 0, 0);
    case CHAR: return Annotation(OST_CHAR, 0, 0);
    case ENUM: return Annotation(OST_ENUM, 0, 0);
    case VOID: return Annotation(OST_VOID, 0, 0);
    case STRING: return Annotation(OST_STRING, 0, 0);
    case STRUCT: return Annotation(OST_STRUCT, 0, 0);

    default:
      ERROR_AND_CONTINUE_FMTMSG("AnnotateType(): Unimplemented ToOstensibleType for TokenType '%s'\n", TokenTypeTranslation(t));
      return Annotation(OST_UNKNOWN, 0, 0);
  }
}

ParserAnnotation FunctionAnnotation(TokenType return_type) {
  ParserAnnotation a = AnnotateType(return_type);
  a.is_function = true;

  return a;
}

ParserAnnotation ArrayAnnotation(TokenType array_type, int array_size) {
  ParserAnnotation a = AnnotateType(array_type);
  a.is_array = true;
  a.array_size = array_size;

  return a;
}
