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
  bool print_bit_width = (a.bit_width > 0) &&
                         (a.actual_type == ACT_FLOAT ||
                          a.actual_type == ACT_INT);

  if (a.is_function) {
    (print_bit_width)
    ? printf("Fn :: %s%d", s, a.bit_width)
    : printf("Fn :: %s", s);
    return;
  }

  if (a.actual_type == ACT_STRING) {
    printf("CHAR[%d]", a.array_size);
    return;
  }

  if (a.is_array) {
    (print_bit_width)
    ? printf("%s%d[%d]", s, a.bit_width, a.array_size)
    : printf("%s[%d]", s, a.array_size);

    return;
  }

  (print_bit_width)
  ? printf("%s%d", s, a.bit_width)
  : printf("%s", s);
}

void InlinePrintOstAnnotation(ParserAnnotation a) {
  switch (a.ostensible_type) {
    case OST_UNKNOWN: {
      _InlinePrintAnnotation("UNKNOWN", a);
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

void InlinePrintActAnnotation(ParserAnnotation a) {
  switch (a.actual_type) {
    case ACT_NOT_APPLICABLE: {
      _InlinePrintAnnotation("N/A", a);
    } break;
    case ACT_INT: {
      _InlinePrintAnnotation((a.is_signed) ? "I" : "U", a);
    } break;
    case ACT_FLOAT: {
      _InlinePrintAnnotation("F", a);
    } break;
    case ACT_BOOL: {
      _InlinePrintAnnotation("BOOL", a);
    } break;
    case ACT_CHAR: {
      _InlinePrintAnnotation("CHAR", a);
    } break;
    case ACT_STRING: {
      _InlinePrintAnnotation("STRING", a);
    } break;
    case ACT_VOID: {
      _InlinePrintAnnotation("VOID", a);
    } break;
    case ACT_ENUM: {
      _InlinePrintAnnotation("ENUM", a);
    } break;
    case ACT_STRUCT: {
      _InlinePrintAnnotation("STRUCT", a);
    } break;
  }
}

ParserAnnotation Annotation(OstensibleType type, int bit_width, bool is_signed) {
  ParserAnnotation a = NoAnnotation();

  a.ostensible_type = type;
  a.bit_width = bit_width;
  a.is_signed = is_signed;

  return a;
}

ParserAnnotation AnnotateType(TokenType t) {
  const bool SIGNED = true;
  const bool UNSIGNED = false;
  const int _ = 0;

  switch (t) {
    case I8:  return Annotation(OST_INT,  8, SIGNED);
    case I16: return Annotation(OST_INT, 16, SIGNED);
    case I32: return Annotation(OST_INT, 32, SIGNED);
    case I64: return Annotation(OST_INT, 64, SIGNED);

    case BINARY_LITERAL:
    case HEX_LITERAL:
    case INT_LITERAL:
    case ENUM_LITERAL: {
      // Treat integer literals as largest available type,
      // Type Checker will shrink them later
      return Annotation(OST_INT, 64, SIGNED);
    }

    case U8:  return Annotation(OST_INT,  8, UNSIGNED);
    case U16: return Annotation(OST_INT, 16, UNSIGNED);
    case U32: return Annotation(OST_INT, 32, UNSIGNED);
    case U64: return Annotation(OST_INT, 64, UNSIGNED);

    case F32: return Annotation(OST_FLOAT, 32, SIGNED);
    case F64: return Annotation(OST_FLOAT, 64, SIGNED);
    // Treat float literals as largest available type,
    // Type Checker will shrink them later
    case FLOAT_LITERAL: return Annotation(OST_FLOAT, 64, SIGNED);

    case BOOL:
    case BOOL_LITERAL: {
      return Annotation(OST_BOOL, 8, UNSIGNED);
    }

    case CHAR:
    case CHAR_LITERAL: {
      return Annotation(OST_CHAR, 8, UNSIGNED);
    }

    case STRING:
    case STRING_LITERAL: {
      return Annotation(OST_STRING, _, _);
      //return ArrayAnnotation(CHAR, 0);
    }

    case ENUM: return Annotation(OST_ENUM, _, _);
    case VOID: return Annotation(OST_VOID, _, _);
    case STRUCT: return Annotation(OST_STRUCT, _, _);

    default:
      ERROR_AND_CONTINUE_FMTMSG("AnnotateType(): Unimplemented ToOstensibleType for TokenType '%s'\n", TokenTypeTranslation(t));
      return Annotation(OST_UNKNOWN, _, _);
  }
}

static const char * const _OstensibleTypeTranslation[] = {
  [OST_UNKNOWN] = "UNKNOWN",
  [OST_INT]     = "INT",
  [OST_FLOAT]   = "FLOAT",
  [OST_BOOL]    = "BOOL",
  [OST_CHAR]    = "CHAR",
  [OST_STRING]  = "STRING",
  [OST_VOID]    = "VOID",
  [OST_ENUM]    = "ENUM",
  [OST_STRUCT]  = "STRUCT",
};

const char *OstensibleTypeTranslation(OstensibleType type) {
  return _OstensibleTypeTranslation[type];
}

const char *ActualTypeTranslation(ActualType type) {
  return _OstensibleTypeTranslation[type];
}

const char *AnnotationTranslation(ParserAnnotation a) {
  switch(a.actual_type) {
    case ACT_INT: {
      if (a.is_signed) {
        switch (a.bit_width) {
          case 8:  return "I8";
          case 16: return "I16";
          case 32: return "I32";
          case 64: return "I64";
          default: return "Unknown bit width";
        }
      } else {
        switch (a.bit_width) {
          case 8:  return "U8";
          case 16: return "U16";
          case 32: return "U32";
          case 64: return "U64";
          default: return "Unknown bit width";
        }
      }
    } break;

    case ACT_FLOAT: {
      if (a.bit_width == 32) return "F32";
      if (a.bit_width == 64) return "F64";
      return "Unknown bit width";
    } break;

    case ACT_BOOL: return "BOOL";

    case ACT_NOT_APPLICABLE: return "NOT APPLICABLE";

    case ACT_CHAR: return "CHAR";
    case ACT_STRING: return "STRING";

    default: return "AnnotationTranslation(): Not implemented yet";
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
