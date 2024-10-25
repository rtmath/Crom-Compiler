#include <stdio.h>  // for printf
#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

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
      _InlinePrintAnnotation("FLOAT", a);
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


static const char* const _NodeTypeTranslation[] =
{
  [UNTYPED] = "UNTYPED",
  [START_NODE] = "START",
  [CHAIN_NODE] = "CHAIN",
  [IF_NODE] = "IF",
  [FUNCTION_NODE] = "FUNCTION",
  [FUNCTION_RETURN_TYPE_NODE] = "RETURN TYPE",
  [FUNCTION_PARAM_NODE] = "FUNCTION PARAM",
  [FUNCTION_BODY_NODE] = "FUNCTION BODY",
};

const char *NodeTypeTranslation(NodeType t) {
  if (t < 0 || t >= NODE_TYPE_COUNT) { return "Out of bounds"; }
  return _NodeTypeTranslation[t];
}

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->type = type;
  n->annotation = a;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = token;
  n->arity = (left != NULL) + (middle != NULL) + (right != NULL);
  n->type = type;
  n->annotation = a;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->arity = arity;
  n->type = type;
  n->annotation = a;

  n->nodes[LEFT] = left;
  n->nodes[MIDDLE] = middle;
  n->nodes[RIGHT] = right;

  return n;
}

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

static void PrintASTRecurse(AST_Node *node, int depth) {
  if (node == NULL) return;
  if (node->type != FUNCTION_RETURN_TYPE_NODE &&
      node->type != LITERAL_NODE &&
      node->type != IDENTIFIER_NODE &&
      node->nodes[LEFT]   == NULL &&
      node->nodes[MIDDLE] == NULL &&
      node->nodes[RIGHT]  == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < depth * 4 && i + node->token.length < 100; i++) {
    buf[i] = ' ';
  }
  buf[i] = '\0';

  if (node->token.type == UNINITIALIZED ||
      node->type == FUNCTION_RETURN_TYPE_NODE ||
      node->type == FUNCTION_BODY_NODE) {
    printf("%s%s ", buf, NodeTypeTranslation(node->type));
    InlinePrintAnnotation(node->annotation);
    printf("\n");
  } else {
    printf("%s%.*s ", buf,
        node->token.length,
        node->token.position_in_source);
    InlinePrintAnnotation(node->annotation);
    printf("\n");
  }

  PrintASTRecurse(node->nodes[LEFT], depth + 1);
  PrintASTRecurse(node->nodes[MIDDLE], depth + 1);
  PrintASTRecurse(node->nodes[RIGHT], depth + 1);
}

void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0);
}
