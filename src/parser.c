#include <stdio.h>

#include <limits.h> // for LONG_MIN and LONG_MAX (strtol error checking)
#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdlib.h> // for malloc

#include "ast.h"
#include "common.h"
#include "error.h"
#include "io.h"
#include "lexer.h"

struct {
  Token current;
  Token next;
  Token after_next;
} Parser;

typedef enum {
  PREC_EOF = -1,
  NO_PRECEDENCE = 0,
  ASSIGNMENT = 1,
  TERNARY_CONDITIONAL = 2,
  LOGICAL = 3,
  BITWISE = 4,
  TERM = 5,
  FACTOR = 6,
  UNARY = 7,
  PREFIX_INCREMENT = 8, PREFIX_DECREMENT = 8,
  ARRAY_SUBSCRIPTING = 9,
} Precedence;

typedef AST_Node* (*ParseFn)(bool);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

/* === Forward Declarations  === */
static SymbolTable *SYMBOL_TABLE();
static AST_Node *StructMemberAccess(Token struct_name);
static AST_Node *FunctionDeclaration(Token function_name);
static AST_Node *FunctionCall(Token identifier);
static AST_Node *InitializerList(Type expected_type);

/* === Forward Declarations for Rules Table === */
#define _ false
#define ASSIGNABLE true

static AST_Node *TypeSpecifier(bool unused);
static AST_Node *Identifier(bool can_assign);
static AST_Node *Unary(bool unused);
static AST_Node *Binary(bool unused);
static AST_Node *TerseAssignment(bool unused);
static AST_Node *Parens(bool unused);
static AST_Node *Block(bool unused);
static AST_Node *Expression(bool unused);
static AST_Node *Statement(bool unused);
static AST_Node *IfStmt(bool unused);
static AST_Node *WhileStmt(bool unused);
static AST_Node *ForStmt(bool unused);
static AST_Node *Break(bool unused);
static AST_Node *Continue(bool unused);
static AST_Node *Return(bool unused);
static AST_Node *ArraySubscripting(bool unused);
static AST_Node *Enum(bool unused);
static AST_Node *Struct(bool unused);
static AST_Node *Literal(bool unused);

static ParseRule Rules[] = {
  // Type Keywords
  [I8]             = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [I16]            = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [I32]            = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [I64]            = { TypeSpecifier, NULL, NO_PRECEDENCE },

  [U8]             = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [U16]            = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [U32]            = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [U64]            = { TypeSpecifier, NULL, NO_PRECEDENCE },

  [F32]            = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [F64]            = { TypeSpecifier, NULL, NO_PRECEDENCE },

  [CHAR]           = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [STRING]         = { TypeSpecifier, NULL, NO_PRECEDENCE },

  [BOOL]           = { TypeSpecifier, NULL, NO_PRECEDENCE },
  [VOID]           = { TypeSpecifier, NULL, NO_PRECEDENCE },

  [ENUM]           = { Enum,     NULL, NO_PRECEDENCE },
  [STRUCT]         = { Struct,   NULL, NO_PRECEDENCE },

  [BREAK]          = { Break,    NULL, NO_PRECEDENCE },
  [CONTINUE]       = { Continue, NULL, NO_PRECEDENCE },
  [RETURN]         = { Return,   NULL, NO_PRECEDENCE },

  [IDENTIFIER]     = { Identifier, NULL, NO_PRECEDENCE },

  // Literals
  [BINARY_LITERAL] = { Literal, NULL, NO_PRECEDENCE },
  [HEX_LITERAL]    = { Literal, NULL, NO_PRECEDENCE },
  [INT_LITERAL]    = { Literal, NULL, NO_PRECEDENCE },
  [FLOAT_LITERAL]  = { Literal, NULL, NO_PRECEDENCE },

  [ENUM_LITERAL]   = { Literal, NULL, NO_PRECEDENCE },
  [CHAR_LITERAL]   = { Literal, NULL, NO_PRECEDENCE },
  [BOOL_LITERAL]   = { Literal, NULL, NO_PRECEDENCE },
  [STRING_LITERAL] = { Literal, NULL, NO_PRECEDENCE },

  // Punctuators
  [LPAREN]         = { Parens,              NULL,      NO_PRECEDENCE },
  [LBRACKET]       = {   NULL, ArraySubscripting, ARRAY_SUBSCRIPTING },

  [EQUALITY]            = {  NULL, Binary, LOGICAL },
  [LOGICAL_NOT]         = { Unary,   NULL, LOGICAL },
  [LOGICAL_AND]         = {  NULL, Binary, LOGICAL },
  [LOGICAL_OR]          = {  NULL, Binary, LOGICAL },
  [LOGICAL_NOT_EQUALS]  = {  NULL, Binary, LOGICAL },
  [LESS_THAN]           = {  NULL, Binary, LOGICAL },
  [GREATER_THAN]        = {  NULL, Binary, LOGICAL },
  [LESS_THAN_EQUALS]    = {  NULL, Binary, LOGICAL },
  [GREATER_THAN_EQUALS] = {  NULL, Binary, LOGICAL },

  [PLUS]           = {   NULL, Binary,   TERM },
  [MINUS]          = {  Unary, Binary,   TERM },
  [ASTERISK]       = {   NULL, Binary, FACTOR },
  [DIVIDE]         = {   NULL, Binary, FACTOR },
  [MODULO]         = {   NULL, Binary, FACTOR },

  [PLUS_PLUS]      = {  Unary, NULL, PREFIX_INCREMENT },
  [MINUS_MINUS]    = {  Unary, NULL, PREFIX_DECREMENT },

  [BITWISE_NOT]         = { Unary,   NULL, BITWISE },
  [BITWISE_AND]         = {  NULL, Binary, BITWISE },
  [BITWISE_XOR]         = {  NULL, Binary, BITWISE },
  [BITWISE_OR]          = {  NULL, Binary, BITWISE },
  [BITWISE_LEFT_SHIFT]  = {  NULL, Binary, BITWISE },
  [BITWISE_RIGHT_SHIFT] = {  NULL, Binary, BITWISE },

  // Misc
  [TOKEN_EOF]      = { NULL, NULL, PREC_EOF },
};

/* === Scope Related === */
static struct {
  int depth;
  SymbolTable *locals[10]; // TODO: figure out actual size or make dynamic array
} Scope;

static void BeginScope() {
  Scope.depth++;
  Scope.locals[Scope.depth] = NewSymbolTable();
}

static void EndScope() {
  if (Scope.depth == 0) {
    SetErrorCode(ERR_PEBCAK);
    COMPILER_ERROR("EndedScope at 0 depth.");
  }

  DeleteSymbolTable(Scope.locals[Scope.depth]);
  Scope.locals[Scope.depth] = NULL;
  Scope.depth--;
}

static Symbol ExistsInOuterScope(Token t) {
  for (int i = Scope.depth; i >= 0; i--) {
    Symbol result = RetrieveFrom(Scope.locals[i], t);
    if (result.token.type != ERROR) {
      return result;
    }
  }

  Token SYMBOL_NOT_FOUND = {
    .type = ERROR,
    .position_in_source = "No symbol found in Symbol Table",
    .length = 31,
    .on_line = 1,
  };

  return NewSymbol(SYMBOL_NOT_FOUND, NoType(), DECL_NONE);
}
/* === End Scope Related === */

static SymbolTable *SYMBOL_TABLE() {
  return Scope.locals[Scope.depth];
}

static void Advance() {
  Parser.current = Parser.next;
  Parser.next = Parser.after_next;
  Parser.after_next = ScanToken();

  if (Parser.next.type != ERROR) return;

  // In case the very first token is an error token
  if (Parser.current.type == UNINITIALIZED) {
    ERROR(ERR_LEXER_ERROR, Parser.next);
  }

  ERROR(ERR_LEXER_ERROR, Parser.next);
}

static bool NextTokenIs(TokenType type) {
  return (Parser.next.type == type);
}

static bool TokenAfterNextIs(TokenType type) {
  return Parser.after_next.type == type;
}

static bool NextTokenIsAnyType() {
  switch (Parser.next.type) {
    case I8:
    case I16:
    case I32:
    case I64:
    case U8:
    case U16:
    case U32:
    case U64:
    case F32:
    case F64:
    case BOOL:
    case STRUCT:
    case CHAR:
    case STRING:
    case VOID:
    {
      return true;
    }
    default: return false;
  }
}

static bool NextTokenIsLiteral() {
  switch (Parser.next.type) {
    case BINARY_LITERAL:
    case HEX_LITERAL:
    case INT_LITERAL:
    case FLOAT_LITERAL:
    case ENUM_LITERAL:
    case CHAR_LITERAL:
    case BOOL_LITERAL:
    case STRING_LITERAL:
    {
      return true;
    }
    default: return false;
  }
}

static bool NextTokenIsTerseAssignment() {
  switch (Parser.next.type) {
    case PLUS_EQUALS:
    case MINUS_EQUALS:
    case TIMES_EQUALS:
    case DIVIDE_EQUALS:
    case MODULO_EQUALS:
    case LOGICAL_NOT_EQUALS:
    case BITWISE_XOR_EQUALS:
    case BITWISE_AND_EQUALS:
    case BITWISE_OR_EQUALS:
    case BITWISE_LEFT_SHIFT_EQUALS:
    case BITWISE_RIGHT_SHIFT_EQUALS:
    {
      return true;
    }
    default: return false;
  }
}

static bool Match(TokenType type) {
  if (!NextTokenIs(type)) return false;

  Advance();

  return true;
}

static void Consume(TokenType type, const char *msg, ...) {
  if (NextTokenIs(type)) {
    Advance();
    return;
  }

  ErrorCode error_code = (type == SEMICOLON)
                           ? ERR_MISSING_SEMICOLON
                           : ERR_UNEXPECTED;
  va_list args;
  va_start(args, msg);

  ERROR_VALIST(error_code, Parser.next, msg, args);

  va_end(args);
}

static void ConsumeAnyType(const char *msg, ...) {
  if (NextTokenIs(I8)     ||
      NextTokenIs(I16)    ||
      NextTokenIs(I32)    ||
      NextTokenIs(I64)    ||
      NextTokenIs(U8)     ||
      NextTokenIs(U16)    ||
      NextTokenIs(U32)    ||
      NextTokenIs(U64)    ||
      NextTokenIs(F32)    ||
      NextTokenIs(F64)    ||
      NextTokenIs(BOOL)   ||
      NextTokenIs(STRUCT) ||
      NextTokenIs(CHAR)   ||
      NextTokenIs(STRING) ||
      NextTokenIs(VOID))
  {
    Advance();
    return;
  }

  va_list args;
  va_start(args, msg);

  ERROR_VALIST(ERR_UNEXPECTED, Parser.next, msg, args);

  va_end(args);
}

static void ConsumeAnyLiteral(const char *msg, ...) {
  if (NextTokenIs(BINARY_LITERAL) ||
      NextTokenIs(HEX_LITERAL)    ||
      NextTokenIs(INT_LITERAL)    ||
      NextTokenIs(FLOAT_LITERAL)  ||
      NextTokenIs(ENUM_LITERAL)   ||
      NextTokenIs(CHAR_LITERAL)   ||
      NextTokenIs(BOOL_LITERAL)   ||
      NextTokenIs(STRING_LITERAL))
  {
    Advance();
    return;
  }

  va_list args;
  va_start(args, msg);

  ERROR_VALIST(ERR_UNEXPECTED, Parser.next, msg, args);

  va_end(args);
}

static void ConsumeAnyTerseAssignment(const char *msg, ...) {
  if (NextTokenIs(PLUS_EQUALS)                ||
      NextTokenIs(MINUS_EQUALS)               ||
      NextTokenIs(TIMES_EQUALS)               ||
      NextTokenIs(DIVIDE_EQUALS)              ||
      NextTokenIs(MODULO_EQUALS)              ||
      NextTokenIs(LOGICAL_NOT_EQUALS)         ||
      NextTokenIs(BITWISE_XOR_EQUALS)         ||
      NextTokenIs(BITWISE_AND_EQUALS)         ||
      NextTokenIs(BITWISE_OR_EQUALS)          ||
      NextTokenIs(BITWISE_LEFT_SHIFT_EQUALS)  ||
      NextTokenIs(BITWISE_RIGHT_SHIFT_EQUALS))
  {
     Advance();
     return;
  }

  va_list args;
  va_start(args, msg);

  ERROR_VALIST(ERR_UNEXPECTED, Parser.next, msg, args);

  va_end(args);
}

void InitParser(SymbolTable *st) {
  Scope.depth = 0;
  Scope.locals[Scope.depth] = st;

  /* Two calls to Advance() will prime the parser, such that
   * Parser.current will still be zeroed out, and
   * Parser.next will hold the first Token from the lexer.
   * The first call to Advance() from inside Parse() will then
   * set Parser.current to the First Token, and Parser.next to
   * look ahead one token, and parsing will proceed normally. */
  Advance();
  Advance();
}

static AST_Node *Parse(int PrecedenceLevel) {
  if (PrecedenceLevel == PREC_EOF) return NULL;
  Advance();

  AST_Node *return_node = NULL;

  ParseFn prefix_rule = Rules[Parser.current.type].prefix;
  if (prefix_rule == NULL) {
    ERROR(ERR_UNEXPECTED, Parser.current);
  }

  bool can_assign = PrecedenceLevel <= ASSIGNMENT;
  AST_Node *prefix_node = prefix_rule(can_assign);

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) {
      ERROR(ERR_UNEXPECTED, Parser.current);
    }

    AST_Node *infix_node = infix_rule(can_assign);

    if (return_node == NULL) {
      infix_node->left = prefix_node;
    } else {
      infix_node->left = return_node;
      return_node = infix_node;
    }

    return_node = infix_node;
  }

  return (return_node == NULL) ? prefix_node : return_node;
}

static AST_Node *TypeSpecifier(bool) {
  Token type_token = Parser.current;
  bool is_array = false || type_token.type == STRING;
  long array_size = 0;

  if (Match(LBRACKET)) {
    if (Match(INT_LITERAL)) {
      array_size = TokenToInt64(Parser.current);
    }

    Consume(RBRACKET, "TypeSpecifier(): Expected ] after '%s', got '%s' instead.",
            TokenTypeTranslation(Parser.current.type),
            TokenTypeTranslation(Parser.next.type));

    is_array = true;
  }

  Consume(IDENTIFIER, "TypeSpecifier(): Expected IDENTIFIER after Type '%s%s', got '%s' instead.",
          TokenTypeTranslation(type_token.type),
          (is_array) ? "[]" : "",
          TokenTypeTranslation(Parser.next.type));

  if (type_token.type == VOID) {
    ERROR_MSG(ERR_IMPROPER_DECLARATION, Parser.current, "Cannot use VOID as a type declaration");
  }

  if (IsIn(SYMBOL_TABLE(), Parser.current)) {
    ERROR(ERR_REDECLARED, Parser.current);
  }

  if (NextTokenIs(LPAREN)) {
    ERROR_MSG(ERR_IMPROPER_DECLARATION, Parser.current, "Function declarations cannot be preceded by a type");
  }

  Type type = (is_array) ? NewArrayType(type_token.type, array_size) : NewType(type_token.type);
  AddTo(SYMBOL_TABLE(), NewSymbol(Parser.current, type, DECL_DECLARED));

  return Identifier(ASSIGNABLE);
}

static AST_Node *Identifier(bool can_assign) {
  Token identifier_token = Parser.current;
  Symbol identifier_symbol = RetrieveFrom(SYMBOL_TABLE(), identifier_token);
  bool is_in_symbol_table = IN_SYMBOL_TABLE(identifier_symbol);
  AST_Node *array_index = NULL;

  if (Match(LPAREN)) {
    if (NextTokenIsAnyType() ||
        (NextTokenIs(RPAREN) && TokenAfterNextIs(COLON_SEPARATOR)))
    { // Declaration
      if (is_in_symbol_table && !DECLARED(identifier_symbol)) {
        ERROR(ERR_REDECLARED, identifier_token);
      }

      // TODO: Check for function definition in outer scope
      if (!is_in_symbol_table) AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, NewFunctionType(VOID), DECL_UNINITIALIZED));

      return FunctionDeclaration(identifier_token);
    } else { // Function call
      if (!is_in_symbol_table) {
        ERROR(ERR_UNDECLARED, identifier_token);
      } else if (!DEFINED(identifier_symbol)) {
        ERROR(ERR_UNDEFINED, identifier_token);
      }

      return FunctionCall(identifier_token);
    }
  }

  if (!is_in_symbol_table) {
    Symbol s = ExistsInOuterScope(identifier_token);
    if (s.token.type == ERROR) {
      ERROR(ERR_UNDECLARED, identifier_token);
    }

    identifier_symbol = s;
    is_in_symbol_table = true;
  }

  if (UNDECLARED(identifier_symbol) && can_assign) {
    ERROR(ERR_REDECLARED, identifier_token);
  }

  if (Match(LBRACKET)) {
    array_index = ArraySubscripting(_);
  }

  if (Match(PLUS_PLUS)) {
    if (!DEFINED(identifier_symbol)) {
      ERROR(ERR_UNDEFINED, identifier_token);
    }

    return NewNodeFromToken(POSTFIX_INCREMENT_NODE, NULL, NULL, NULL, identifier_token, identifier_symbol.data_type);
  }

  if (Match(MINUS_MINUS)) {
    if (!DEFINED(identifier_symbol)) {
      ERROR(ERR_UNDEFINED, identifier_token);
    }

    return NewNodeFromToken(POSTFIX_DECREMENT_NODE, NULL, NULL, NULL, identifier_token, identifier_symbol.data_type);
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR(ERR_IMPROPER_ASSIGNMENT, identifier_token);
    }

    if (TypeIs_Array(identifier_symbol.data_type) &&
        !TypeIs_String(identifier_symbol.data_type)) {
      if (Match(LCURLY)) {
        AST_Node *initializer_list = InitializerList(identifier_symbol.data_type);
        identifier_symbol = SetDecl(SYMBOL_TABLE(), identifier_token, DECL_DEFINED);
        return NewNodeFromSymbol(ASSIGNMENT_NODE, initializer_list, array_index, NULL, identifier_symbol);
      } else if (array_index != NULL) {
        /* Subscripting */
      } else {
        ERROR_MSG(ERR_IMPROPER_ASSIGNMENT, identifier_token, "Arrays are not assignable");
      }
    }

    AST_Node *expr = Expression(_);
    Symbol stored_symbol = AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, identifier_symbol.data_type, DECL_DEFINED));
    return NewNodeFromSymbol(ASSIGNMENT_NODE, expr, array_index, NULL, stored_symbol);
  }

  if (NextTokenIsTerseAssignment()) {
    ConsumeAnyTerseAssignment("Identifier() Terse Assignment: How did this error message appear?");
    if (!DEFINED(identifier_symbol)) {
      ERROR(ERR_UNDEFINED, identifier_token);
    }

    AST_Node *terse_assignment = TerseAssignment(_);
    terse_assignment->left = NewNodeFromSymbol(IDENTIFIER_NODE, NULL, NULL, NULL, identifier_symbol);
    return terse_assignment;
  }

  if (TypeIs_Struct(identifier_symbol.data_type) && Match(PERIOD)) {
    return StructMemberAccess(identifier_token);
  }

  // Check for invalid syntax like "i64 i + 1;"
  if (DECLARED(identifier_symbol) && !NextTokenIs(SEMICOLON)) {
    ERROR(ERR_MISSING_SEMICOLON, Parser.next);
  }

  return NewNodeFromToken(
    (DECLARED(identifier_symbol)) ? DECLARATION_NODE
                                  : IDENTIFIER_NODE,
    NULL, array_index, NULL, identifier_token, identifier_symbol.data_type
  );
}

static AST_Node *Unary(bool) {
  if (NextTokenIsAnyType()) {
    ERROR(ERR_IMPROPER_DECLARATION, Parser.next);
  }

  Token operator_token = Parser.current;
  Token token_after_operator = Parser.next; // for error messages
  AST_Node *parse_result = Parse(UNARY);

  switch(operator_token.type) {
    case PLUS_PLUS: {
      if (token_after_operator.type != IDENTIFIER) {
        ERROR_FMT(ERR_UNEXPECTED, token_after_operator, "Expected Identifier, got '%s' instead", TokenTypeTranslation(token_after_operator.type));
      }

      Symbol s = RetrieveFrom(SYMBOL_TABLE(), token_after_operator);
      if (!DEFINED(s)) {
        ERROR(ERR_UNDEFINED, token_after_operator);
      }

      return NewNodeFromToken(PREFIX_INCREMENT_NODE, parse_result, NULL, NULL, operator_token, NoType());
    } break;

    case MINUS_MINUS: {
      if (token_after_operator.type != IDENTIFIER) {
        ERROR(ERR_UNEXPECTED, token_after_operator);
      }

      Symbol s = RetrieveFrom(SYMBOL_TABLE(), token_after_operator);
      if (!DEFINED(s)) {
        ERROR(ERR_UNDEFINED, token_after_operator);
      }

      return NewNodeFromToken(PREFIX_DECREMENT_NODE, parse_result, NULL, NULL, operator_token, NoType());
    } break;

    case BITWISE_NOT:
    case LOGICAL_NOT:
    case MINUS:
      return NewNodeFromToken(UNARY_OP_NODE, parse_result, NULL, NULL, operator_token, NoType());
    default:
      ERROR_FMT(ERR_PEBCAK, operator_token, "Unknown operator '%s'", TokenTypeTranslation(operator_token.type));
      return NULL;
  }
}

static AST_Node *Binary(bool) {
  Token operator_token = Parser.current;

  if (NextTokenIsAnyType()) {
    ERROR(ERR_IMPROPER_DECLARATION, Parser.next);
  }

  Precedence precedence = Rules[Parser.current.type].precedence;
  AST_Node *parse_result = Parse(precedence + 1);

  switch(operator_token.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
    case MODULO:
      return NewNodeFromToken(BINARY_ARITHMETIC_NODE, NULL, NULL, parse_result, operator_token, NoType());
    case EQUALITY:
    case LOGICAL_AND:
    case LOGICAL_OR:
    case LOGICAL_NOT_EQUALS:
    case LESS_THAN:
    case GREATER_THAN:
    case LESS_THAN_EQUALS:
    case GREATER_THAN_EQUALS:
      return NewNodeFromToken(BINARY_LOGICAL_NODE, NULL, NULL, parse_result, operator_token, NewType(BOOL));
    case BITWISE_XOR:
    case BITWISE_NOT:
    case BITWISE_AND:
    case BITWISE_OR:
    case BITWISE_LEFT_SHIFT:
    case BITWISE_RIGHT_SHIFT:
      return NewNodeFromToken(BINARY_BITWISE_NODE, NULL, NULL, parse_result, operator_token, NewType(U64));
    default:
      ERROR_FMT(ERR_PEBCAK, operator_token, "Unknown operator '%s'", TokenTypeTranslation(operator_token.type));
      return NULL;
  }
}

static AST_Node *TerseAssignment(bool) {
  Token operator_token = Parser.current;

  Precedence precedence = Rules[Parser.current.type].precedence;
  AST_Node *parse_result = Parse(precedence + 1);

  switch(operator_token.type) {
    case PLUS_EQUALS:
    case MINUS_EQUALS:
    case TIMES_EQUALS:
    case DIVIDE_EQUALS:
    case MODULO_EQUALS:
    case LOGICAL_NOT_EQUALS:
    case BITWISE_XOR_EQUALS:
    case BITWISE_AND_EQUALS:
    case BITWISE_OR_EQUALS:
    case BITWISE_LEFT_SHIFT_EQUALS:
    case BITWISE_RIGHT_SHIFT_EQUALS:
      return NewNodeFromToken(TERSE_ASSIGNMENT_NODE, NULL, NULL, parse_result, operator_token, NoType());
    default:
      ERROR_FMT(ERR_PEBCAK, operator_token, "Unknown operator '%s'", TokenTypeTranslation(operator_token.type));
      return NULL;
  }
}

static AST_Node *Block(bool) {
  AST_Node *n = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->left = Statement(_);
    (*current)->right = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());

    current = &(*current)->right;
  }

  Consume(RCURLY, "Block(): Expected '}' after Block, got '%s' instead.", TokenTypeTranslation(Parser.next.type));

  return n;
}

static AST_Node *Expression(bool) {
  return Parse((Precedence)1);
}

static AST_Node *Statement(bool) {
  if (Match(IF)) return IfStmt(_);
  if (Match(WHILE)) return WhileStmt(_);
  if (Match(FOR)) return ForStmt(_);

  AST_Node *expr_result = Expression(_);

  // Allow optional semicolon after Enum, Struct and Function definitions
  if (TypeIs_Enum(expr_result->data_type)   ||
      TypeIs_Struct(expr_result->data_type) ||
      TypeIs_Function(expr_result->data_type))
  {
    Match(SEMICOLON);
  } else {
    Consume(SEMICOLON, "Statement(): A ';' is expected after an expression statement, got '%s' instead",
        TokenTypeTranslation(Parser.next.type));
  }

  return expr_result;
}

static AST_Node *IfStmt(bool) {
  Consume(LPAREN, "IfStmt(): Expected '(' after IF token, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));
  AST_Node *condition = Expression(_);
  Consume(RPAREN, "IfStmt(): Expected ')' after IF condition, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  Consume(LCURLY, "IfStmt(): Expected '{', got '%s' instead", TokenTypeTranslation(Parser.next.type));

  BeginScope();

  AST_Node *body_if_true = Block(_);
  AST_Node *body_if_false = NULL;

  if (Match(ELSE)) {
    if (Match(IF))  {
      body_if_false = IfStmt(_);
    } else {
      Consume(LCURLY, "IfStmt(): Expected block starting with '{' after ELSE, got '%s' instead", TokenTypeTranslation(Parser.next.type));
      body_if_false = Block(_);
    }
  }

  EndScope();

  return NewNode(IF_NODE, condition, body_if_true, body_if_false, NoType());
}

static AST_Node *TernaryIfStmt(AST_Node *condition) {
  Consume(QUESTION_MARK, "TernaryIfStmt(): Expected '?' after Ternary Condition, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *if_true = Expression(_);

  Consume(COLON, "TernaryIfStmt(): Expected ':' after Ternary Statement, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *if_false = Expression(_);

  return NewNode(IF_NODE, condition, if_true, if_false, NoType());
}

static AST_Node *WhileStmt(bool) {
  AST_Node *condition = Expression(_);
  Consume(LCURLY, "WhileStmt(): Expected '{' after While condition, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *block = Block(_);
  Match(SEMICOLON);
  return NewNode(WHILE_NODE, condition, NULL, block, NoType());
}

static AST_Node *ForStmt(bool) {
  Consume(LPAREN, "ForStmt(): Expected '(' after For, got '%s instead", TokenTypeTranslation(Parser.next.type));

  AST_Node *initialization = Statement(_);
  AST_Node *condition = Statement(_);
  AST_Node *after_loop = Expression(_);

  Consume(RPAREN, "ForStmt(): Expected ')' after For, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  Consume(LCURLY, "ForStmt(): Expected '{' after For, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *body = Block(_);
  AST_Node **find_last_body_statement = &body;

  while ((*find_last_body_statement)->right != NULL) find_last_body_statement = &(*find_last_body_statement)->right;

  (*find_last_body_statement)->left = after_loop;

  AST_Node *while_node = NewNode(WHILE_NODE, condition, NULL, body, NoType());
  return NewNode(FOR_NODE, initialization, NULL, while_node, NoType());
}

static AST_Node *Break(bool) {
  Consume(SEMICOLON,
          "Break(): Expected ';' after Break, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  return NewNode(BREAK_NODE, NULL, NULL, NULL, NoType());
}

static AST_Node *Continue(bool) {
  Consume(SEMICOLON,
          "Continue(): Expected ';' after Continue, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  return NewNode(CONTINUE_NODE, NULL, NULL, NULL, NoType());
}

static AST_Node *Return(bool) {
  AST_Node *expr = NULL;

  if (!NextTokenIs(SEMICOLON)) {
    expr = Expression(_);
  }

  return NewNode(RETURN_NODE, expr, NULL, NULL, (expr == NULL) ? NewType(VOID) : expr->data_type);
}

static AST_Node *Parens(bool) {
  AST_Node *parse_result = Expression(_);
  Consume(RPAREN, "Parens(): Missing ')' after expression");

  if (NextTokenIs(QUESTION_MARK)) {
    return TernaryIfStmt(parse_result);
  }

  return parse_result;
}

static AST_Node *ArraySubscripting(bool) {
  AST_Node *return_value = NULL;

  if (Match(IDENTIFIER)) {
    Symbol symbol = RetrieveFrom(SYMBOL_TABLE(), Parser.current);
    bool is_in_symbol_table = IsIn(SYMBOL_TABLE(), Parser.current);

    if (!is_in_symbol_table) {
      ERROR(ERR_UNDECLARED, Parser.current);
    }

    if (!DEFINED(symbol)) {
      ERROR(ERR_UNINITIALIZED, Parser.current);
    }

    return_value = NewNodeFromSymbol(ARRAY_SUBSCRIPT_NODE, NULL, NULL, NULL, symbol);
  } else if (Match(INT_LITERAL)) {
    return_value = NewNodeFromToken(ARRAY_SUBSCRIPT_NODE, NULL, NULL, NULL, Parser.current, NewType(Parser.current.type));
  }

  Consume(RBRACKET, "ArraySubscripting(): Where's the ']'?");

  return return_value;
}

static AST_Node *EnumListEntry(bool can_assign) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE(), Parser.current);
  bool is_in_symbol_table = IsIn(SYMBOL_TABLE(), Parser.current);
  Token identifier_token = Parser.current;

  if (!is_in_symbol_table) {
    ERROR(ERR_UNDECLARED, identifier_token);
  }

  if (UNDECLARED(symbol) && can_assign) {
    ERROR(ERR_REDECLARED, identifier_token);
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR(ERR_IMPROPER_ASSIGNMENT, identifier_token);
    }

    Symbol stored_symbol = AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, symbol.data_type, DECL_DEFINED));
    return NewNodeFromSymbol(ENUM_ASSIGNMENT_NODE, Expression(_), NULL, NULL, stored_symbol);
  }

  return NewNodeFromToken(ENUM_LIST_ENTRY_NODE, NULL, NULL, NULL, identifier_token, NewType(ENUM_LITERAL));
}

static void EnumBlock(AST_Node **enum_name) {
  AST_Node **current = enum_name;

  Consume(LCURLY, "EnumBlock(): Expected '{' after ENUM declaration, got %s", TokenTypeTranslation(Parser.current.type));

  bool empty_body = true;
  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    empty_body = false;

    Consume(IDENTIFIER, "EnumBlock(): Expected IDENTIFIER, got '%s' instead.",
            TokenTypeTranslation(Parser.next.type));
    Token enum_identifier = Parser.current;

    if (IsIn(SYMBOL_TABLE(), enum_identifier)) {
      ERROR(ERR_REDECLARED, enum_identifier);
    }

    AddTo(SYMBOL_TABLE(), NewSymbol(enum_identifier, NewType(ENUM_LITERAL), DECL_DEFINED));

    (*current)->left = EnumListEntry(ASSIGNABLE);
    (*current)->right = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());

    current = &(*current)->right;
    if (!NextTokenIs(RCURLY)) {
      Consume(COMMA, "Expected COMMA, got '%s' instead.", TokenTypeTranslation(Parser.next.type));
    }
  }

  Consume(RCURLY, "EnumBlock(): Expected '}' after ENUM block, got %s", TokenTypeTranslation(Parser.current.type));

  if (empty_body) {
    ERROR(ERR_EMPTY_BODY, Parser.current);
  }
}

static AST_Node *Enum(bool) {
  Consume(IDENTIFIER, "Enum(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(Parser.next.type),
          TokenTypeTranslation(Parser.next.type));

  Token enum_identifier = Parser.current;
  Symbol stored_symbol = RetrieveFrom(SYMBOL_TABLE(), enum_identifier);

  if (DEFINED(stored_symbol)) {
    ERROR(ERR_REDECLARED, enum_identifier);
  }

  AddTo(SYMBOL_TABLE(), NewSymbol(enum_identifier, NewType(ENUM), DECL_UNINITIALIZED));

  AST_Node *enum_name = Identifier(false);
  enum_name->node_type = ENUM_IDENTIFIER_NODE;

  EnumBlock(&enum_name);

  AddTo(SYMBOL_TABLE(), NewSymbol(enum_identifier, NewType(ENUM), DECL_DEFINED));
  return enum_name;
}

static AST_Node *StructMemberAccess(Token struct_name) {
  AST_Node *expr = NULL;
  AST_Node *array_index = NULL;
  Symbol struct_symbol = RetrieveFrom(SYMBOL_TABLE(), struct_name);
  if (!DEFINED(struct_symbol)) {
    ERROR(ERR_UNDEFINED, Parser.next);
  }

  BeginScope();

  Consume(IDENTIFIER, "StructMemberAccess(): Expected identifier", "");
  Token field_name = Parser.current;
  if (!StructContainsMember(struct_symbol.data_type, field_name)) {
    ERROR(ERR_UNDEFINED, field_name);
  }

  if (Match(LBRACKET)) {
    array_index = ArraySubscripting(_);
  }

  if (Match(EQUALS)) {
    expr = Expression(_);
    SetDecl(SYMBOL_TABLE(), field_name, DECL_DEFINED);
  }

  Symbol field_symbol = RetrieveFrom(SYMBOL_TABLE(), field_name);
  if (!DEFINED(field_symbol)) {
    ERROR(ERR_UNDEFINED, field_name);
  }

  EndScope();

  AST_Node *parent_struct = NewNodeFromToken(STRUCT_IDENTIFIER_NODE, NULL, NULL, NULL, struct_name, NoType());
  return NewNodeFromToken(STRUCT_MEMBER_IDENTIFIER_NODE, expr, array_index, parent_struct, field_name, field_symbol.data_type);
}

static void StructBody(AST_Node **struct_name) {
  AST_Node **current = struct_name;

  Consume(LCURLY, "Struct(): Expected '{' after STRUCT declaration, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  bool empty_body = true;
  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    empty_body = false;

    ConsumeAnyType("StructBody(): Expected type in struct member declaration.", "");
    Token type_token = Parser.current;

    if (type_token.type == VOID) {
      ERROR_MSG(ERR_IMPROPER_DECLARATION, Parser.current, "Cannot declare a struct member VOID");
    }

    bool is_array = false;
    int array_size = 0;
    if (Match(LBRACKET)) {
      if (!NextTokenIs(RBRACKET)) {
        Consume(INT_LITERAL, "StructBody(): Expected INT_LITERAL, got '%s'", Parser.next);
        array_size = TokenToInt64(Parser.current);
      }
      Consume(RBRACKET, "StructBody(): Expected ']' after '['");
      is_array = true;
    }

    Consume(IDENTIFIER,
            "StructBody(): Expected IDENTIFIER, got '%s' instead",
            TokenTypeTranslation(Parser.next.type));

    Token member_token = Parser.current;
    Type member_type = (is_array) ? NewArrayType(type_token.type, array_size) : NewType(type_token.type);

    if (StructContainsMember((*struct_name)->data_type, member_token)) {
      ERROR(ERR_REDECLARED, member_token);
    }

    AddMemberToStruct(&(*struct_name)->data_type, member_type, member_token);

    (*current)->left = NewNodeFromToken(STRUCT_MEMBER_IDENTIFIER_NODE, NULL, NULL, NULL, member_token, member_type);
    (*current)->right = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());

    current = &(*current)->right;

    Consume(SEMICOLON, "StructBody(): Expected semicolon after struct member declaration", "");
  }

  Consume(RCURLY, "StructBody(): Expected '}' after STRUCT block, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  if (empty_body) {
    ERROR(ERR_EMPTY_BODY, Parser.current);
  }
}

static AST_Node *Struct() {
  Consume(IDENTIFIER, "Struct(): Expected IDENTIFIER after Type '%s, got '%s instead",
          TokenTypeTranslation(Parser.current.type),
          TokenTypeTranslation(Parser.next.type));
  Token identifier_token = Parser.current;

  if (IsIn(SYMBOL_TABLE(), identifier_token)) {
    ERROR(ERR_REDECLARED, identifier_token);
  }
  Symbol identifier_symbol = AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, NewType(STRUCT), DECL_DECLARED));

  AST_Node *struct_identifier = NewNodeFromSymbol(STRUCT_DECLARATION_NODE, NULL, NULL, NULL, identifier_symbol);
  StructBody(&struct_identifier);

  SetDecl(SYMBOL_TABLE(), identifier_symbol.token, DECL_DEFINED);

  return struct_identifier;
}

static AST_Node *InitializerList(Type expected_type) {
  AST_Node *n = NULL;
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    if (n == NULL) n = NewNode(ARRAY_INITIALIZER_LIST_NODE, NULL, NULL, NULL, expected_type);

    (*current)->left = Expression(_);
    (*current)->right = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());

    current = &(*current)->right;

    Match(COMMA);
  }

  Consume(RCURLY, "InitializerList(): Expected '}' after Initializer List", "");

  if (n == NULL) {
    ERROR_MSG(ERR_IMPROPER_ASSIGNMENT, Parser.current, "Initializer List cannot be empty");
  }

  return n;
}

static AST_Node *FunctionParams(Token function_name) {
  Symbol function = RetrieveFrom(SYMBOL_TABLE(), function_name);

  AST_Node *params = NewNode(FUNCTION_PARAM_NODE, NULL, NULL, NULL, NoType());
  AST_Node **current = &params;

  while (!NextTokenIs(RPAREN) && !NextTokenIs(TOKEN_EOF)) {
    ConsumeAnyType("FunctionParams(): Expected a type, got '%s' instead", TokenTypeTranslation(Parser.next.type));
    Token type_token = Parser.current;

    if (type_token.type == VOID) {
      ERROR_MSG(ERR_IMPROPER_DECLARATION, Parser.current, "Cannot declare a function parameter VOID");
    }

    bool is_array = false;
    if (Match(LBRACKET)) {
      Consume(RBRACKET, "FunctionParams(): Expected ']' after '['");
      is_array = true;
    }

    Consume(IDENTIFIER, "FunctionParams(): Expected identifier after '(', got '%s' instead",
            TokenTypeTranslation(Parser.next.type));
    Token member_name = Parser.current;
    Type member_type = (is_array) ? NewArrayType(type_token.type, 0) : NewType(type_token.type);

    if (FunctionHasParam(function.data_type, member_name) && !DECLARED(function)) {
      ERROR(ERR_REDECLARED, member_name);
    }

    AddParamToFunction(&function.data_type, member_type, member_name);

    (*current)->data_type = member_type;
    (*current)->token = member_name;

    if (Match(COMMA) || !NextTokenIs(RPAREN)) {
      (*current)->left = NewNode(FUNCTION_PARAM_NODE, NULL, NULL, NULL, NoType());

      current = &(*current)->left;
    }
  }

  AddTo(SYMBOL_TABLE(), function);

  return params;
}

static AST_Node *FunctionReturnType() {
  Consume(RPAREN, "FunctionReturnType(): ')' required after function declaration");
  Consume(COLON_SEPARATOR, "FunctionReturnType(): '::' required after function declaration");
  ConsumeAnyType("FunctionReturnType(): Expected a type after '::'");

  Token fn_return_type = Parser.current;

  return NewNodeFromToken(FUNCTION_RETURN_TYPE_NODE, NULL, NULL, NULL, fn_return_type, NewType(fn_return_type.type));
}

static AST_Node *FunctionBody(Token function_name) {
  if (NextTokenIs(SEMICOLON)) { return NULL; }

  Consume(LCURLY, "FunctionBody(): Expected '{' to begin function body, got '%s' instead", TokenTypeTranslation(Parser.next.type));

  Symbol function = RetrieveFrom(SYMBOL_TABLE(), function_name);

  AST_Node *body = NewNode(FUNCTION_BODY_NODE, NULL, NULL, NULL, NoType());
  AST_Node **current = &body;

  BeginScope();
  AddParams(SYMBOL_TABLE(), function);

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->left = Statement(_);
    (*current)->right = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());

    current = &(*current)->right;
  }

  Consume(RCURLY, "FunctionBody(): Expected '}' after function body");

  EndScope();

  if (body->left == NULL) { // Insert a Void Return if there's no function body
    body->left = NewNode(RETURN_NODE, NULL, NULL, NULL, NewType(VOID));
  }

  return body;
}

static AST_Node *FunctionDeclaration(Token function_name) {
  if (Scope.depth != 0) {
    ERROR_MSG(ERR_IMPROPER_DECLARATION, function_name, "Functions must be declared in global scope");
  }

  AST_Node *params = FunctionParams(function_name);
  AST_Node *return_type = FunctionReturnType();
  AST_Node *body = FunctionBody(function_name);

  Symbol function = RetrieveFrom(SYMBOL_TABLE(), function_name);

  if (DECLARED(function) && body == NULL) {
    ERROR(ERR_REDECLARED, function.token);
  }

  if (!DECLARED(function)) {
    function.data_type.specifier = return_type->data_type.specifier;
  }

  function.declaration_state = (body == NULL) ? DECL_DECLARED : DECL_DEFINED;
  function = AddTo(SYMBOL_TABLE(), function);

  return NewNodeFromSymbol((body == NULL) ? DECLARATION_NODE : FUNCTION_NODE, return_type, params, body, function);
}

static AST_Node *FunctionCall(Token function_name) {
  AST_Node *args = NULL;
  AST_Node **current = &args;

  while (!NextTokenIs(RPAREN) && !NextTokenIs(TOKEN_EOF)) {
    if (NextTokenIs(IDENTIFIER)) {
      Consume(IDENTIFIER, "FunctionCall(): Expected identifier\n");
      Token identifier_token = Parser.current;
      Symbol identifier = RetrieveFrom(SYMBOL_TABLE(), identifier_token);

      if (Match(LPAREN)) {
        (*current) = FunctionCall(identifier_token);
      } else {
        (*current) = NewNodeFromSymbol(FUNCTION_ARGUMENT_NODE, NULL, NULL, NULL, identifier);
      }

    } else if (NextTokenIsLiteral()) {
      ConsumeAnyLiteral("FunctionCall(): Expected literal\n");
      Token literal = Parser.current;

      (*current) = NewNodeFromToken(FUNCTION_ARGUMENT_NODE, NULL, NULL, NULL, literal, NewType(literal.type));
    }

    if (Match(COMMA)) {
      if (NextTokenIs(RPAREN)) { break; }

      (*current)->right = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());
      current = &(*current)->right;
    }
  }

  Consume(RPAREN, "FunctionCall(): Expected ')'");

  Symbol fn_definition = RetrieveFrom(SYMBOL_TABLE(), function_name);

  return NewNodeFromToken(FUNCTION_CALL_NODE, NULL, args, NULL, function_name, fn_definition.data_type);
}

static AST_Node *Literal(bool) {
  Type t = (Parser.current.type == STRING_LITERAL)
             ? NewArrayType(Parser.current.type, Parser.current.length)
             : NewType(Parser.current.type);
  return NewNodeFromToken(LITERAL_NODE, NULL, NULL, NULL, Parser.current, t);
}

AST_Node *ParserBuildAST() {
  AST_Node *root = NewNode(START_NODE, NULL, NULL, NULL, NoType());

  AST_Node **current_node = &root;

  while (!Match(TOKEN_EOF)) {
    AST_Node *parse_result = Statement(_);

    if (parse_result == NULL) {
      SetErrorCode(ERR_MISC);
      COMPILER_ERROR("ParserBuildAST(): AST could not be created");
    }

    AST_Node *next_statement = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoType());

    (*current_node)->left = parse_result;
    (*current_node)->right = next_statement;

    current_node = &(*current_node)->right;
  }

  return root;
}
