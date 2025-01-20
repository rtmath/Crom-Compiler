#include <stdlib.h> // for calloc

#include "ast.h"
#include "common.h"
#include "error.h"

static const char* const _NodeTypeTranslation[] =
{
  [UNTYPED_NODE] = "UNTYPED",
  [START_NODE] = "Start",
  [CHAIN_NODE] = "Chain",
  [DECLARATION_NODE] = "Declaration",
  [IDENTIFIER_NODE] = "Identifier",
  [LITERAL_NODE] = "Literal",

  [ASSIGNMENT_NODE] = "Assignment",
  [TERSE_ASSIGNMENT_NODE] = "Terse Assignment",
  [INITIALIZER_LIST_NODE] = "Initializer List",

  [STRUCT_DECLARATION_NODE] = "Struct Declaration",
  [STRUCT_IDENTIFIER_NODE] = "Struct Identifier",
  [STRUCT_MEMBER_IDENTIFIER_NODE] = "Struct Member",

  [ENUM_IDENTIFIER_NODE] = "Enum Identifier",
  [ENUM_LIST_ENTRY_NODE] = "Enum List Entry",
  [ENUM_ASSIGNMENT_NODE] = "Enum Assignment",

  [ARRAY_SUBSCRIPT_NODE] = "Array Subscript",

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
  [PRINT_CALL_NODE] = "Print",

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

AST_Node *NewNode(NodeType node_type, AST_Node *left, AST_Node *middle, AST_Node *right, Type type) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->node_type = node_type;
  SetNodeDataType(n, type);

  n->left = left;
  n->middle = middle;
  n->right = right;

  return n;
}

AST_Node *NewNodeFromToken(NodeType node_type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, Type type) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = token;
  n->node_type = node_type;
  SetNodeDataType(n, type);

  n->left = left;
  n->middle = middle;
  n->right = right;

  return n;
}

AST_Node *NewNodeFromSymbol(NodeType node_type, AST_Node *left, AST_Node *middle, AST_Node *right, Symbol symbol) {
  AST_Node *n = calloc(1, sizeof(AST_Node));

  n->token = symbol.token;
  n->node_type = node_type;
  SetNodeDataType(n, symbol.data_type);

  n->left = left;
  n->middle = middle;
  n->right = right;

  return n;
}

void SetNodeDataType(AST_Node *n, Type t) {
  n->data_type = t;
}

static void PrintASTRecurse(AST_Node *n, int depth, int unindent) {
  #define NUM_INDENT_SPACES 4

  if (n== NULL) return;
  if (NodeIs_Chain(n)        &&
      NodeIs_NULL(n->left)   &&
      NodeIs_NULL(n->middle) &&
      NodeIs_NULL(n->right)) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < (depth * NUM_INDENT_SPACES) - unindent && i + n->token.length < 100; i++) {
    buf[i] = (i == 0) ? '|' : ' ';
  }
  buf[i] = '\0';

  Print("%s", buf);
  if (n->token.type != UNINITIALIZED) {
    (n->token.type == STRING_LITERAL)
    ? Print("\"%.*s\" ", n->token.length, n->token.position_in_source)
    : Print("%.*s ", n->token.length, n->token.position_in_source);
  }

  if (!TypeIs_None(n->data_type) && n->node_type != START_NODE) {
    InlinePrintType(n->data_type);
    Print(" ");
  }

  if (!NodeIs_Untyped(n) &&
      !NodeIs_Chain(n)   &&
      !NodeIs_Start(n)   &&
      !NodeIs_Function(n)) {
    Print("%s", NodeTypeTranslation(n->node_type));
  }

  Print("\n");

  if (NodeIs_Chain(n)) unindent += NUM_INDENT_SPACES;
  PrintASTRecurse(n->left, depth + 1, unindent);
  PrintASTRecurse(n->middle, depth + 1, unindent);
  PrintASTRecurse(n->right, depth + 1, unindent);

  #undef NUM_INDENT_SPACES
}

void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0, 0);
}

static void InlinePrintNodeSummary(AST_Node *n) {
  Print("%16s Node | ",
         NodeTypeTranslation(n->node_type));
  InlinePrintToken(n->token);
  Print(" {");
  InlinePrintType(n->data_type);
  Print("}");
}

void PrintNode(AST_Node *n) {
  Print("%16s Node ", NodeTypeTranslation(n->node_type));
  Print("'%.*s'", n->token.length, n->token.position_in_source);
  Print(" {");
  InlinePrintType(n->data_type);
  Print("}");
  Print("\n");

  if (n->left != NULL) {
    Print("  LEFT: ");
    InlinePrintNodeSummary(n->left);
    Print("\n");
  }
  if (n->middle != NULL) {
    Print("MIDDLE: ");
    InlinePrintNodeSummary(n->middle);
    Print("\n");
  }
  if (n->right != NULL) {
    Print(" RIGHT: ");
    InlinePrintNodeSummary(n->right);
    Print("\n");
  }
  Print("----------------------------------------------------------\n");
}

bool NodeIs_NULL(AST_Node *n) {
  return n == NULL;
}

bool NodeIs_Untyped(AST_Node *n) {
  return n->node_type == UNTYPED_NODE;
}

bool NodeIs_Start(AST_Node *n) {
  return n->node_type == START_NODE;
}

bool NodeIs_Chain(AST_Node *n) {
  return n->node_type == CHAIN_NODE;
}

bool NodeIs_Identifier(AST_Node *n) {
  return n->node_type == IDENTIFIER_NODE;
}

bool NodeIs_ArraySubscript(AST_Node *n) {
  return n->node_type == ARRAY_SUBSCRIPT_NODE;
}

bool NodeIs_InitializerList(AST_Node *n) {
  return n->node_type == INITIALIZER_LIST_NODE;
}

bool NodeIs_TerseAssignment(AST_Node *n) {
  return n->node_type == TERSE_ASSIGNMENT_NODE;
}

bool NodeIs_EnumAssignment(AST_Node *n) {
  return n->node_type == ENUM_ASSIGNMENT_NODE;
}

bool NodeIs_EnumIdentifier(AST_Node *n) {
  return n->node_type == ENUM_IDENTIFIER_NODE;
}

bool NodeIs_EnumEntry(AST_Node *n) {
  return n->node_type == ENUM_LIST_ENTRY_NODE;
}

bool NodeIs_StructMember(AST_Node *n) {
  return n->node_type == STRUCT_MEMBER_IDENTIFIER_NODE;
}

bool NodeIs_TernaryIf(AST_Node *n) {
  return n->node_type == TERNARY_IF_NODE;
}

bool NodeIs_If(AST_Node *n) {
  return n->node_type == IF_NODE;
}

bool NodeIs_For(AST_Node *n) {
  return n->node_type == FOR_NODE;
}

bool NodeIs_While(AST_Node *n) {
  return n->node_type == WHILE_NODE;
}

bool NodeIs_Function(AST_Node *n) {
  return n->node_type == FUNCTION_NODE;
}

bool NodeIs_Return(AST_Node *n) {
  return n->node_type == RETURN_NODE;
}

bool NodeIs_PrefixIncrement(AST_Node *n) {
  return n->node_type == PREFIX_INCREMENT_NODE;
}

bool NodeIs_PrefixDecrement(AST_Node *n) {
  return n->node_type == PREFIX_DECREMENT_NODE;
}

bool NodeIs_PostfixIncrement(AST_Node *n) {
  return n->node_type == POSTFIX_INCREMENT_NODE;
}

bool NodeIs_PostfixDecrement(AST_Node *n) {
  return n->node_type == POSTFIX_DECREMENT_NODE;
}

bool NodeIs_DeadEnd(AST_Node *n) {
  return (n == NULL) ||
         (NodeIs_Chain(n)        &&
          NodeIs_NULL(n->left)   &&
          NodeIs_NULL(n->middle) &&
          NodeIs_NULL(n->right));
}
