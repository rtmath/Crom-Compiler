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
  if (a.is_function) {
    printf("Fn :: %s%d", s, a.bit_width);
    return;
  }

  if (a.is_array) {
    (a.bit_width > 0)
    ? printf("%s%d[%d]", s, a.bit_width, a.array_size)
    : printf("%s[%d]", s, a.array_size);

    return;
  }

  (a.bit_width > 0)
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
    case F64: return Annotation(OST_FLOAT, 64, SIGNED);
    case BOOL: return Annotation(OST_BOOL, 0, 0);
    case CHAR: return Annotation(OST_CHAR, 0, 0);
    case ENUM: return Annotation(OST_ENUM, 0, 0);
    case VOID: return Annotation(OST_VOID, 0, 0);
    case STRING: return Annotation(OST_STRING, 0, 0);
    case STRUCT: return Annotation(OST_STRUCT, 0, 0);

    case INT_LITERAL:
    case HEX_LITERAL:
    case BINARY_LITERAL:
    case ENUM_LITERAL: {
      return Annotation(OST_INT, 64, SIGNED);
    }

    case FLOAT_LITERAL: return Annotation(OST_FLOAT, 64, SIGNED);
    case BOOL_LITERAL: return Annotation(OST_BOOL, 0, 0);
    case STRING_LITERAL: return Annotation(OST_STRING, 0, 0);

    default:
      ERROR_AND_CONTINUE_FMTMSG("AnnotateType(): Unimplemented ToOstensibleType for TokenType '%s'\n", TokenTypeTranslation(t));
      return Annotation(OST_UNKNOWN, 0, 0);
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
  if (a.actual_type == ACT_INT) {
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
  }

  if (a.actual_type == ACT_FLOAT) {
    if (a.bit_width == 32) return "F32";
    if (a.bit_width == 64) return "F64";
    return "Unknown bit width";
  }

  if (a.actual_type == ACT_NOT_APPLICABLE) {
    return "NOT APPLICABLE";
  }

  return "AnnotationTranslation(): Not implemented yet";
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
