#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "error.h"
#include "parser_annotation.h"
#include "symbol_table.h"
#include "token.h"
#include "type.h"
#include "value.h"

#define LEFT 0
#define RIGHT 1
#define MIDDLE 2

#define LEFT_NODE(n) ((n)->nodes[LEFT])
#define RIGHT_NODE(n) ((n)->nodes[RIGHT])
#define MIDDLE_NODE(n) ((n)->nodes[MIDDLE])

typedef enum {
  UNTYPED_NODE,
  START_NODE,
  CHAIN_NODE,
  DECLARATION_NODE,
  IDENTIFIER_NODE,
  STRUCT_DECLARATION_NODE,
  STRUCT_IDENTIFIER_NODE,
  STRUCT_MEMBER_IDENTIFIER_NODE,
  ENUM_IDENTIFIER_NODE, // the Enum name (enum Weekday { ... })
  ENUM_LIST_ENTRY_NODE, // the individual Enum values (Monday, etc)
  ARRAY_SUBSCRIPT_NODE,
  ARRAY_INITIALIZER_LIST_NODE,

  IF_NODE,
  WHILE_NODE,
  FOR_NODE,

  BREAK_NODE,
  CONTINUE_NODE,
  RETURN_NODE,

  FUNCTION_NODE,
  FUNCTION_RETURN_TYPE_NODE,
  FUNCTION_PARAM_NODE,
  FUNCTION_BODY_NODE,
  FUNCTION_CALL_NODE,
  FUNCTION_ARGUMENT_NODE,

  LITERAL_NODE,

  ASSIGNMENT_NODE,
  ENUM_ASSIGNMENT_NODE,
  TERSE_ASSIGNMENT_NODE,
  UNARY_OP_NODE,
  BINARY_LOGICAL_NODE,
  BINARY_ARITHMETIC_NODE,
  BINARY_BITWISE_NODE,

  PREFIX_INCREMENT_NODE,
  PREFIX_DECREMENT_NODE,
  POSTFIX_INCREMENT_NODE,
  POSTFIX_DECREMENT_NODE,

  NODE_TYPE_COUNT
} NodeType;

typedef struct AST_Node {
  ErrorCode error_code; // This is really only for the Start node of the AST
  NodeType node_type;
  Token token;
  ParserAnnotation annotation;
  Value value;

  struct AST_Node *nodes[3];
} AST_Node;

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation annotation);
AST_Node *NewNodeFromToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation annotation);
AST_Node *NewNodeFromSymbol(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Symbol symbol);

const char *NodeTypeTranslation(NodeType t);

void PrintAST(AST_Node *root);
void PrintNode(AST_Node *node);

bool NodeIs_NULL(AST_Node *n);
bool NodeIs_Untyped(AST_Node *n);
bool NodeIs_Start(AST_Node *n);
bool NodeIs_Chain(AST_Node *n);
bool NodeIs_ArraySubscript(AST_Node *n);
bool NodeIs_Identifier(AST_Node *n);
bool NodeIs_TerseAssignment(AST_Node *n);
bool NodeIs_EnumAssignment(AST_Node *n);
bool NodeIs_EnumIdentifier(AST_Node *n);
bool NodeIs_EnumEntry(AST_Node *n);
bool NodeIs_StructMember(AST_Node *n);
bool NodeIs_If(AST_Node *n);
bool NodeIs_For(AST_Node *n);
bool NodeIs_While(AST_Node *n);
bool NodeIs_Function(AST_Node *n);
bool NodeIs_Return(AST_Node *n);
bool NodeIs_PrefixIncrement(AST_Node *n);
bool NodeIs_PrefixDecrement(AST_Node *n);
bool NodeIs_PostfixIncrement(AST_Node *n);
bool NodeIs_PostfixDecrement(AST_Node *n);

#endif
