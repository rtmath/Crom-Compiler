#include <stdio.h>  // for printf
#include <stdlib.h> // for calloc

#include "ast.h"
#include "error.h"

static const char* const _NodeTypeTranslation[] =
{
  [UNTYPED] = "UNTYPED",
  [START_NODE] = "Start",
  [CHAIN_NODE] = "Chain",
  [DECLARATION_NODE] = "Declaration",
  [IDENTIFIER_NODE] = "Identifier",
  [STRUCT_DECLARATION_NODE] = "Struct Declaration",
  [STRUCT_IDENTIFIER_NODE] = "Struct Identifier",
  [STRUCT_MEMBER_IDENTIFIER_NODE] = "Struct Member",
  [ENUM_IDENTIFIER_NODE] = "Enum Identifier",
  [ENUM_LIST_ENTRY_NODE] = "Enum List Entry",
  [ARRAY_SUBSCRIPT_NODE] = "Array Subscript",
  [ARRAY_INITIALIZER_LIST_NODE] = "Array Initializer List",

  [IF_NODE] = "If",
  [WHILE_NODE] = "While",
  [FOR_NODE] = "For",

  [BREAK_NODE] = "Break",
  [CONTINUE_NODE] = "Continue",
  [RETURN_NODE] = "Return",

  [FUNCTION_NODE] = "Function",
  [FUNCTION_RETURN_TYPE_NODE] = "Fn Return Type",
  [FUNCTION_PARAM_NODE] = "Fn Param",
  [FUNCTION_BODY_NODE] = "Fn Body",
  [FUNCTION_CALL_NODE] = "Fn Call",
  [FUNCTION_ARGUMENT_NODE] = "Fn Argument",

  [LITERAL_NODE] = "Literal",

  [ASSIGNMENT_NODE] = "Assignment",
  [ENUM_ASSIGNMENT_NODE] = "Enum Assignment",
  [TERSE_ASSIGNMENT_NODE] = "Terse Assignment",
  [UNARY_OP_NODE] = "Unary",
  [BINARY_LOGICAL_NODE] = "Binary (Logical)",
  [BINARY_ARITHMETIC_NODE] = "Binary (Arithmetic)",
  [BINARY_BITWISE_NODE] = "Binary (Bitwise)",

  [PREFIX_INCREMENT_NODE] = "++Increment",
  [PREFIX_DECREMENT_NODE] = "--Decrement",
  [POSTFIX_INCREMENT_NODE] = "Increment++",
  [POSTFIX_DECREMENT_NODE] = "Decrement--",
};

const char *NodeTypeTranslation(NodeType t) {
  if (t < 0 || t >= NODE_TYPE_COUNT) { return "Out of bounds"; }
  return _NodeTypeTranslation[t];
}

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->type = type;
  n->annotation = a;

  LEFT_NODE(n) = left;
  MIDDLE_NODE(n) = middle;
  RIGHT_NODE(n) = right;

  return n;
}

AST_Node *NewNodeFromToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation a) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = token;
  n->type = type;
  n->annotation = a;

  LEFT_NODE(n) = left;
  MIDDLE_NODE(n) = middle;
  RIGHT_NODE(n) = right;

  return n;
}

AST_Node *NewNodeFromSymbol(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Symbol symbol) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = symbol.token;
  n->type = type;
  n->annotation = symbol.annotation;

  LEFT_NODE(n) = left;
  MIDDLE_NODE(n) = middle;
  RIGHT_NODE(n) = right;

  return n;
}

static void PrintASTRecurse(AST_Node *node, int depth, int unindent) {
  #define NUM_INDENT_SPACES 4

  if (node == NULL) return;
  if (node->type == CHAIN_NODE &&
      LEFT_NODE(node) == NULL &&
      MIDDLE_NODE(node) == NULL &&
      RIGHT_NODE(node) == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < (depth * NUM_INDENT_SPACES) - unindent && i + node->token.length < 100; i++) {
    buf[i] = (i == 0) ? '|' : ' ';
  }
  buf[i] = '\0';

  printf("%s", buf);
  if (node->token.type != UNINITIALIZED) {
    (node->token.type == STRING_LITERAL)
    ? printf("\"%.*s\" ", node->token.length, node->token.position_in_source)
    : printf("%.*s ", node->token.length, node->token.position_in_source);
  }

  if (node->annotation.actual_type != ACT_NOT_APPLICABLE) {
    InlinePrintActAnnotation(node->annotation);
    printf(" ");
  }

  if (node->type != UNTYPED &&
      node->type != CHAIN_NODE &&
      node->type != START_NODE &&
      node->type != FUNCTION_NODE) {
    printf("%s", NodeTypeTranslation(node->type));
  }

  if (node->value.type != V_NONE) {
    printf(" :: ");
    InlinePrintValue(node->value);
  }

  printf("\n");

  if (node->type == CHAIN_NODE) unindent += NUM_INDENT_SPACES;
  PrintASTRecurse(LEFT_NODE(node), depth + 1, unindent);
  PrintASTRecurse(MIDDLE_NODE(node), depth + 1, unindent);
  PrintASTRecurse(RIGHT_NODE(node), depth + 1, unindent);

  #undef NUM_INDENT_SPACES
}

void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0, 0);
}

static void InlinePrintNodeSummary(AST_Node *node) {
  printf("%16s Node | ",
         NodeTypeTranslation(node->type));
  InlinePrintToken(node->token);
  printf(" [OST ");
  InlinePrintOstAnnotation(node->annotation);
  printf(" : ACT ");
  InlinePrintActAnnotation(node->annotation);
  printf("]");
}

void PrintNode(AST_Node *node) {
  printf("%16s Node ", NodeTypeTranslation(node->type));
  printf("'%.*s'", node->token.length, node->token.position_in_source);
  printf(" [OST ");
  InlinePrintOstAnnotation(node->annotation);
  printf(" : ACT ");
  InlinePrintActAnnotation(node->annotation);
  printf("] | Value: ");
  PrintValue(node->value);
  printf("\n");

  if (LEFT_NODE(node) != NULL) {
    printf("  LEFT: ");
    InlinePrintNodeSummary(LEFT_NODE(node));
    printf("\n");
  }
  if (MIDDLE_NODE(node) != NULL) {
    printf("MIDDLE: ");
    InlinePrintNodeSummary(MIDDLE_NODE(node));
    printf("\n");
  }
  if (RIGHT_NODE(node) != NULL) {
    printf(" RIGHT: ");
    InlinePrintNodeSummary(RIGHT_NODE(node));
    printf("\n");
  }
  printf("----------------------------------------------------------\n");
}
