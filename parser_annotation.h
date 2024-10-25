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

typedef struct {
  OstensibleType ostensible_type;
  int bit_width; // for I8, U16, etc
  bool is_signed;
  int declared_on_line;
  bool is_array;
  int array_size;
  bool is_function;
} ParserAnnotation;

ParserAnnotation NoAnnotation();
ParserAnnotation FunctionAnnotation(TokenType return_type);
ParserAnnotation AnnotateType(TokenType t);

void InlinePrintAnnotation(ParserAnnotation);

#endif
