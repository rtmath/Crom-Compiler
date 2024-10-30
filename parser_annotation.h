#ifndef PARSER_ANNOTATION_H
#define PARSER_ANNOTATION_H

#include <stdbool.h>

#include "token_type.h"

typedef enum {
  OST_UNKNOWN,
  OST_INT,
  OST_FLOAT,
  OST_BOOL,
  OST_CHAR,
  OST_STRING,
  OST_VOID,
  OST_ENUM,
  OST_STRUCT,
} OstensibleType;

typedef enum {
  ACT_NOT_APPLICABLE,
  ACT_INT,
  ACT_FLOAT,
  ACT_BOOL,
  ACT_CHAR,
  ACT_STRING,
  ACT_VOID,
  ACT_ENUM,
  ACT_STRUCT,
} ActualType;

typedef struct {
  OstensibleType ostensible_type;
  ActualType actual_type;

  bool is_signed;
  int bit_width; // for I8, U16, etc

  bool is_array;
  int array_size;

  bool is_function;

  // for helpful error messages
  int declared_on_line;
} ParserAnnotation;

const char *OstensibleTypeTranslation(OstensibleType type);
const char *ActualTypeTranslation(ActualType type);

ParserAnnotation NoAnnotation();
ParserAnnotation FunctionAnnotation(TokenType return_type);
ParserAnnotation ArrayAnnotation(TokenType array_type, int array_size);
ParserAnnotation AnnotateType(TokenType t);
ParserAnnotation Annotation(OstensibleType type, int bit_width, bool is_signed);

void InlinePrintOstAnnotation(ParserAnnotation a);
void InlinePrintActAnnotation(ParserAnnotation a);
const char *AnnotationTranslation(ParserAnnotation a);

#endif
