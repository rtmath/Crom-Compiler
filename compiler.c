#include <limits.h> // for LONG_MIN and LONG_MAX (strtol error checking)
#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdio.h>  // for printf and friends
#include <stdlib.h> // for malloc

#include "ast.h"
#include "common.h"
#include "compiler.h"
#include "error.h"
#include "lexer.h"
#include "symbol_table.h"

SymbolTable *SYMBOL_TABLE;

#define UNUSED false
#define CAN_ASSIGN true

struct {
  Token current;
  Token next;
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

/* Rules table Forward Declarations */
static AST_Node *Type(bool unused);
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
static AST_Node *Break(bool unused);
static AST_Node *Continue(bool unused);
static AST_Node *Return(bool unused);
static AST_Node *ArraySubscripting(bool unused);
static AST_Node *Enum(bool unused);
static AST_Node *Struct(bool unused);
static AST_Node *Literal(bool unused);

/* Other Forward Declarations */
static AST_Node *FunctionDeclaration(Symbol symbol);

ParseRule Rules[] = {
  // Type Keywords
  [I8]             = {   Type,   NULL, NO_PRECEDENCE },
  [I16]            = {   Type,   NULL, NO_PRECEDENCE },
  [I32]            = {   Type,   NULL, NO_PRECEDENCE },
  [I64]            = {   Type,   NULL, NO_PRECEDENCE },

  [U8]             = {   Type,   NULL, NO_PRECEDENCE },
  [U16]            = {   Type,   NULL, NO_PRECEDENCE },
  [U32]            = {   Type,   NULL, NO_PRECEDENCE },
  [U64]            = {   Type,   NULL, NO_PRECEDENCE },

  [F32]            = {   Type,   NULL, NO_PRECEDENCE },
  [F64]            = {   Type,   NULL, NO_PRECEDENCE },

  [CHAR]           = {   Type,   NULL, NO_PRECEDENCE },
  [STRING]         = {   Type,   NULL, NO_PRECEDENCE },

  [BOOL]           = {   Type,   NULL, NO_PRECEDENCE },
  [VOID]           = {   Type,   NULL, NO_PRECEDENCE },
  [ENUM]           = {   Enum,   NULL, NO_PRECEDENCE },
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

  [LOGICAL_NOT]    = { Unary,   NULL, LOGICAL },
  [LOGICAL_AND]    = {  NULL, Binary, LOGICAL },
  [LOGICAL_OR]     = {  NULL, Binary, LOGICAL },
  [LESS_THAN]      = {  NULL, Binary, LOGICAL },
  [GREATER_THAN]   = {  NULL, Binary, LOGICAL },

  [PLUS]           = {   NULL, Binary,   TERM },
  [MINUS]          = {  Unary, Binary,   TERM },
  [ASTERISK]       = {   NULL, Binary, FACTOR },
  [DIVIDE]         = {   NULL, Binary, FACTOR },
  [MODULO]         = {   NULL, Binary, FACTOR },

  [PLUS_PLUS]      = {  Unary, NULL, PREFIX_INCREMENT },
  [MINUS_MINUS]    = {  Unary, NULL, PREFIX_DECREMENT },

  // Misc
  [TOKEN_EOF]      = { NULL, NULL, PREC_EOF },
};

static void Advance() {
  Parser.current = Parser.next;
  Parser.next = ScanToken();

  if (Parser.next.type != ERROR) return;

  ERROR_AND_EXIT_FMTMSG("Advance(): Error token encountered after token '%s': %.*s",
      TokenTypeTranslation(Parser.current.type),
      Parser.next.length,
      Parser.next.position_in_source);
}

static bool NextTokenIs(TokenType type) {
  return (Parser.next.type == type);
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
    case BITWISE_NOT_EQUALS:
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

  va_list args;
  va_start(args, msg);

  ERROR_AND_EXIT_VALIST(msg, args);

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
      NextTokenIs(STRING))
  {
    Advance();
    return;
  }

  va_list args;
  va_start(args, msg);

  ERROR_AND_EXIT_VALIST(msg, args);

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
      NextTokenIs(BITWISE_NOT_EQUALS)         ||
      NextTokenIs(BITWISE_LEFT_SHIFT_EQUALS)  ||
      NextTokenIs(BITWISE_RIGHT_SHIFT_EQUALS))
  {
     Advance();
     return;
  }

  va_list args;
  va_start(args, msg);

  ERROR_AND_EXIT_VALIST(msg, args);

  va_end(args);
}

static void InitParser() {
  /* One call to Advance() will prime the parser, such that
   * Parser.current will still be zeroed out, and
   * Parser.next will hold the First Token(TM) from the lexer.
   * The first call to Advance() from inside Parse() will then
   * set Parser.current to the First Token, and Parser.next to
   * look ahead one token, and parsing will proceed normally. */

  Advance();
}

static AST_Node *Parse(int PrecedenceLevel) {
  if (PrecedenceLevel == PREC_EOF) return NULL;
  Advance();

  AST_Node *return_node = NULL;

  ParseFn prefix_rule = Rules[Parser.current.type].prefix;
  if (prefix_rule == NULL) {
    printf("Prefix Rule for '%s' is NULL.\n",
        TokenTypeTranslation(Parser.current.type));
    return NULL;
  }

  bool can_assign = PrecedenceLevel <= ASSIGNMENT;
  AST_Node *prefix_node = prefix_rule(can_assign);

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) {
      ERROR_AND_EXIT_FMTMSG("Infix Rule for '%s' is NULL.\n",
                            TokenTypeTranslation(Parser.current.type));
    }

    AST_Node *infix_node = infix_rule(can_assign);

    if (return_node == NULL) {
      infix_node->nodes[LEFT] = prefix_node;
    } else {
      infix_node->nodes[LEFT] = return_node;
      return_node = infix_node;
    }

    return_node = infix_node;
  }

  return (return_node == NULL) ? prefix_node : return_node;
}

static AST_Node *Type(bool) {
  Token remember_token = Parser.current; // TODO: I don't think I need this
  bool is_array = false;
  long array_size = 0;

  if (Match(LBRACKET)) {
    if (Match(INT_LITERAL)) {
      array_size = strtol(Parser.current.position_in_source, NULL, 10);
      if (array_size == LONG_MIN) ERROR_AND_EXIT("Type(): strtol underflowed");
      if (array_size == LONG_MAX) ERROR_AND_EXIT("Type(): strtol overflowed");
    }

    Consume(RBRACKET, "Type(): Expected ] after '%s', got '%s' instead.",
            TokenTypeTranslation(remember_token.type),
            TokenTypeTranslation(Parser.next.type));

    is_array = true;
  }

  if (NextTokenIs(IDENTIFIER)) {
    if (IsIn(SYMBOL_TABLE, Parser.next)) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE, Parser.next);
      ERROR_AND_EXIT_FMTMSG("Type(): Re-declaration of identifier '%.*s', previously declared on line %d\n",
                            Parser.next.length,
                            Parser.next.position_in_source,
                            s.annotation.declared_on_line);
    }

    ParserAnnotation a = (is_array) ? ArrayAnnotation(remember_token.type, array_size) : AnnotateType(remember_token.type);
    AddTo(SYMBOL_TABLE, NewSymbol(Parser.next, a, DECL_DECLARED));
  }

  Consume(IDENTIFIER, "Type(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(remember_token.type),
          TokenTypeTranslation(Parser.next.type));

  return Identifier(CAN_ASSIGN);
}

static AST_Node *Identifier(bool can_assign) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE, Parser.current);
  bool is_in_symbol_table = IsIn(SYMBOL_TABLE, Parser.current);
  AST_Node *array_index = NULL;
  Token remember_token = Parser.current;

  if (Match(LPAREN)) {
    if (NextTokenIsAnyType()) { // Declaration
      if (is_in_symbol_table && symbol.declaration_type != DECL_DECLARED) {
        ERROR_AND_EXIT_FMTMSG("Function '%.*s' has been redeclared, original declaration on line %d\n",
                              remember_token.length,
                              remember_token.position_in_source,
                              symbol.annotation.declared_on_line);
      }

      if (!is_in_symbol_table) AddTo(SYMBOL_TABLE, NewSymbol(remember_token, FunctionAnnotation(VOID), DECL_UNINITIALIZED));
      symbol = RetrieveFrom(SYMBOL_TABLE, remember_token);

      return FunctionDeclaration(symbol);
    } else { // Function call
      // TODO: Implement
    }
  }

  if (!is_in_symbol_table) {
    ERROR_AND_EXIT_FMTMSG("Identifier(): Line %d: Undeclared identifier '%.*s'",
                          remember_token.on_line,
                          remember_token.length,
                          remember_token.position_in_source);
  }

  if (Match(LBRACKET)) {
    array_index = ArraySubscripting(UNUSED);
  }

  if (Match(PLUS_PLUS)) {
    if (symbol.declaration_type != DECL_DEFINED) {
      ERROR_AND_EXIT_FMTMSG("Cannot increment undefined variable '%.*s",
                            remember_token.length,
                            remember_token.position_in_source);
    }

    return NewNodeFromToken(POSTFIX_INCREMENT_NODE, NULL, NULL, NULL, remember_token, NoAnnotation());
  }

  if (Match(MINUS_MINUS)) {
    if (symbol.declaration_type != DECL_DEFINED) {
      ERROR_AND_EXIT_FMTMSG("Cannot decrement undefined variable '%.*s",
                            remember_token.length,
                            remember_token.position_in_source);
    }

    return NewNodeFromToken(POSTFIX_DECREMENT_NODE, NULL, NULL, NULL, remember_token, NoAnnotation());
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR_AND_EXIT_FMTMSG("Identifier(): Cannot assign to identifier '%.*s'",
                            remember_token.length,
                            remember_token.position_in_source);
    }

    Symbol stored_symbol = AddTo(SYMBOL_TABLE, NewSymbol(remember_token, symbol.annotation, DECL_DEFINED));
    return NewNodeFromSymbol(IDENTIFIER_NODE, Expression(UNUSED), array_index, NULL, stored_symbol);
  }

  if (NextTokenIs(SEMICOLON)) {
    if (symbol.declaration_type == DECL_NONE && can_assign) {
      Symbol already_declared = RetrieveFrom(SYMBOL_TABLE, remember_token);
      ERROR_AND_EXIT_FMTMSG("Identifier(): Identifier '%.*s' has been redeclared. First declared on line %d\n",
                            remember_token.length,
                            remember_token.position_in_source,
                            already_declared.annotation.declared_on_line);
    }

    return NewNodeFromToken(IDENTIFIER_NODE, NULL, array_index, NULL, remember_token, symbol.annotation);
  }

  if (NextTokenIsTerseAssignment()) {
    ConsumeAnyTerseAssignment("Identifier() Terse Assignment: How did this error message appear?");
    if (symbol.declaration_type != DECL_DEFINED) {
      ERROR_AND_EXIT_FMTMSG("Cannot perform a terse assignment on undefined variable '%.*s'",
                            remember_token.length,
                            remember_token.position_in_source);
    }
    return NewNodeFromToken(ASSIGNMENT_NODE, TerseAssignment(UNUSED), NULL, NULL, remember_token, NoAnnotation());
  }

  Symbol s = RetrieveFrom(SYMBOL_TABLE, remember_token);
  return NewNodeFromSymbol(LITERAL_NODE, NULL, array_index, NULL, s);
}

static AST_Node *Unary(bool) {
  Token remember_token = Parser.current;
  AST_Node *parse_result = Parse(UNARY);

  switch(remember_token.type) {
    case PLUS_PLUS:
      return NewNodeFromToken(PREFIX_INCREMENT_NODE, parse_result, NULL, NULL, remember_token, NoAnnotation());
    case MINUS_MINUS:
      return NewNodeFromToken(POSTFIX_INCREMENT_NODE, parse_result, NULL, NULL, remember_token, NoAnnotation());
    case LOGICAL_NOT:
    case MINUS:
      return NewNodeFromToken(UNTYPED, parse_result, NULL, NULL, remember_token, NoAnnotation());
    default:
      printf("Unary(): Unknown Unary operator '%s'\n",
          TokenTypeTranslation(remember_token.type));
      return NULL;
  }
}

static AST_Node *Binary(bool) {
  Token remember_token = Parser.current;

  Precedence precedence = Rules[Parser.current.type].precedence;
  AST_Node *parse_result = Parse(precedence + 1);

  switch(remember_token.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
    case MODULO:
    case LOGICAL_AND:
    case LOGICAL_OR:
    case LESS_THAN:
    case GREATER_THAN:
    case BITWISE_XOR:
    case BITWISE_NOT:
    case BITWISE_AND:
    case BITWISE_OR:
    case BITWISE_LEFT_SHIFT:
    case BITWISE_RIGHT_SHIFT:
      return NewNodeFromToken(UNTYPED, NULL, NULL, parse_result, remember_token, NoAnnotation());
    default:
      ERROR_AND_EXIT_FMTMSG("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
      return NULL;
  }
}

static AST_Node *TerseAssignment(bool) {
  Token remember_token = Parser.current;

  Precedence precedence = Rules[Parser.current.type].precedence;
  AST_Node *parse_result = Parse(precedence + 1);

  switch(remember_token.type) {
    case PLUS_EQUALS:
    case MINUS_EQUALS:
    case TIMES_EQUALS:
    case DIVIDE_EQUALS:
    case MODULO_EQUALS:
    case LOGICAL_NOT_EQUALS:
    case BITWISE_XOR_EQUALS:
    case BITWISE_AND_EQUALS:
    case BITWISE_OR_EQUALS:
    case BITWISE_NOT_EQUALS:
    case BITWISE_LEFT_SHIFT_EQUALS:
    case BITWISE_RIGHT_SHIFT_EQUALS:
      return NewNodeFromToken(ASSIGNMENT_NODE, NULL, NULL, parse_result, remember_token, NoAnnotation());
    default:
      ERROR_AND_EXIT_FMTMSG("TerseAssignment(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
      return NULL;
  }
}

static AST_Node *Block(bool) {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->nodes[LEFT] = Statement(UNUSED);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];
  }

  Consume(RCURLY, "Block(): Expected '}' after Block, got '%s' instead.", TokenTypeTranslation(Parser.next.type));

  return n;
}

static AST_Node *Expression(bool) {
  return Parse((Precedence)1);
}

static AST_Node *Statement(bool) {
  if (Match(IF)) return IfStmt(UNUSED);
  if (Match(WHILE)) return WhileStmt(UNUSED);

  AST_Node *expr_result = Expression(UNUSED);

  // Allow optional semicolon after Enum, Struct and Function definitions
  if (expr_result->annotation.ostensible_type == OST_ENUM   ||
      expr_result->annotation.ostensible_type == OST_STRUCT ||
      expr_result->annotation.is_function)
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
  AST_Node *condition = Expression(UNUSED);
  Consume(RPAREN, "IfStmt(): Expected ')' after IF condition, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  Consume(LCURLY, "IfStmt(): Expected '{', got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *body_if_true = Block(UNUSED);
  AST_Node *body_if_false = NULL;

  if (Match(ELSE)) {
    if (Match(IF))  {
      body_if_false = IfStmt(UNUSED);
    } else {
      Consume(LCURLY, "IfStmt(): Expected block starting with '{' after ELSE, got '%s' instead", TokenTypeTranslation(Parser.next.type));
      body_if_false = Block(UNUSED);
    }
  }

  return NewNode(IF_NODE, condition, body_if_true, body_if_false, NoAnnotation());
}

static AST_Node *TernaryIfStmt(AST_Node *condition) {
  Consume(QUESTION_MARK, "Expected '?' after Ternary Condition, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *if_true = Expression(UNUSED);

  Consume(COLON, "Expected ':' after Ternary Statement, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *if_false = Expression(UNUSED);

  return NewNode(IF_NODE, condition, if_true, if_false, NoAnnotation());
}

static AST_Node *WhileStmt(bool) {
  AST_Node *condition = Expression(UNUSED);
  Consume(LCURLY, "Expected '{' after While condition, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *block = Block(UNUSED);

  return NewNode(WHILE_NODE, condition, NULL, block, NoAnnotation());
}

static AST_Node *Break(bool) {
  if (!NextTokenIs(SEMICOLON)) {
    ERROR_AND_EXIT_FMTMSG("Expected ';' after Break, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  }

  return NewNode(BREAK_NODE, NULL, NULL, NULL, NoAnnotation());
}

static AST_Node *Continue(bool) {
  if (!NextTokenIs(SEMICOLON)) {
    ERROR_AND_EXIT_FMTMSG("Expected ';' after Continue, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  }

  return NewNode(CONTINUE_NODE, NULL, NULL, NULL, NoAnnotation());
}

static AST_Node *Return(bool) {
  AST_Node *expr = NULL;

  if (!NextTokenIs(SEMICOLON)) {
    expr = Expression(UNUSED);
  }

  return NewNode(RETURN_NODE, expr, NULL, NULL, (expr == NULL) ? AnnotateType(VOID) : expr->annotation);
}

static AST_Node *Parens(bool) {
  AST_Node *parse_result = Expression(UNUSED);
  Consume(RPAREN, "Parens(): Missing ')' after expression");

  if (NextTokenIs(QUESTION_MARK)) {
    return TernaryIfStmt(parse_result);
  }

  return parse_result;
}

static AST_Node *ArraySubscripting(bool) {
  AST_Node *return_value = NULL;

  if (Match(IDENTIFIER)) {
    Symbol symbol = RetrieveFrom(SYMBOL_TABLE, Parser.current);
    bool is_in_symbol_table = IsIn(SYMBOL_TABLE, Parser.current);

    if (!is_in_symbol_table) {
      ERROR_AND_EXIT_FMTMSG("Can't access array with undeclared identifier '%.*s'",
                            Parser.current.length,
                            Parser.current.position_in_source);
    }

    if (symbol.declaration_type != DECL_DEFINED) {
      ERROR_AND_EXIT_FMTMSG("Can't access array with uninitialized identifier '%.*s'",
                            Parser.current.length,
                            Parser.current.position_in_source);
    }

    return_value = NewNodeFromSymbol(LITERAL_NODE, NULL, NULL, NULL, symbol);
  } else if (Match(INT_LITERAL)) {
    return_value = Literal(UNUSED);
  }

  Consume(RBRACKET, "ArraySubscripting(): Where's the ']'?");

  return return_value;
}

static AST_Node *EnumBlock() {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &n;

  Consume(LCURLY, "EnumBlock(): Expected '{' after ENUM declaration, got %s", TokenTypeTranslation(Parser.current.type));

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    Symbol symbol = RetrieveFrom(SYMBOL_TABLE, Parser.next);
    bool is_in_symbol_table = IsIn(SYMBOL_TABLE, Parser.next);

    if (is_in_symbol_table) {
      ERROR_AND_EXIT_FMTMSG("EnumBlock(): Enum identifier '%.*s' already exists, declared on line %d",
                            Parser.next.length,
                            Parser.next.position_in_source,
                            symbol.annotation.declared_on_line);
    }

    Consume(IDENTIFIER, "EnumBlock(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
            TokenTypeTranslation(Parser.current.type),
            TokenTypeTranslation(Parser.next.type));
    AddTo(SYMBOL_TABLE, NewSymbol(Parser.current, NoAnnotation(), DECL_DEFINED));

    (*current)->nodes[LEFT] = Identifier(CAN_ASSIGN);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];

    Match(COMMA);
  }

  Consume(RCURLY, "EnumBlock(): Expected '}' after ENUM block, got %s", TokenTypeTranslation(Parser.current.type));

  return n;
}

static AST_Node *Enum(bool) {
  Consume(IDENTIFIER, "Enum(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(Parser.next.type),
          TokenTypeTranslation(Parser.next.type));
  AddTo(SYMBOL_TABLE, NewSymbol(Parser.current, AnnotateType(Parser.current.type), DECL_DECLARED));

  AST_Node *enum_name = Identifier(false);
  enum_name->nodes[LEFT] = EnumBlock();

  return enum_name;
}

static AST_Node *Struct() {
  Consume(IDENTIFIER, "Struct(): Expected IDENTIFIER after Type '%s, got '%s instead",
          TokenTypeTranslation(Parser.current.type),
          TokenTypeTranslation(Parser.next.type));
  Token identifier_token = Parser.current;

  if (IsIn(SYMBOL_TABLE, identifier_token)) {
    Symbol existing_struct = RetrieveFrom(SYMBOL_TABLE, identifier_token);
    ERROR_AND_EXIT_FMTMSG("Struct '%.*s' is already in symbol table, declared on line %d\n",
      identifier_token.length,
      identifier_token.position_in_source,
      existing_struct.annotation.declared_on_line);
  }
  Symbol identifier_symbol = AddTo(SYMBOL_TABLE, NewSymbol(identifier_token, AnnotateType(STRUCT), DECL_DECLARED));

  SymbolTable *remember_symbol_table = SYMBOL_TABLE;
  SYMBOL_TABLE = identifier_symbol.struct_fields;

  Consume(LCURLY, "Struct(): Expected '{' after STRUCT declaration, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &n;

  bool has_empty_body = true;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    has_empty_body = false;
    (*current)->nodes[LEFT] = Statement(UNUSED);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];
  }

  Consume(RCURLY, "Struct(): Expected '}' after STRUCT block, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));
  SYMBOL_TABLE = remember_symbol_table;

  if (has_empty_body) {
    ERROR_AND_EXIT_FMTMSG("Struct(): Struct '%.*s' has empty body",
                          identifier_symbol.token.length,
                          identifier_symbol.token.position_in_source);
  }

  Symbol stored_symbol = AddTo(SYMBOL_TABLE, NewSymbol(identifier_token, AnnotateType(STRUCT), DECL_DEFINED));
  return NewNodeFromSymbol(IDENTIFIER_NODE, n, NULL, NULL, stored_symbol);
}

static AST_Node *FunctionParams(SymbolTable *fn_params) {
  AST_Node *params = NewNodeWithArity(FUNCTION_PARAM_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &params;

  while (!NextTokenIs(RPAREN) && !NextTokenIs(TOKEN_EOF)) {
    ConsumeAnyType("FunctionParams(): Expected a type, got '%s' instead", TokenTypeTranslation(Parser.next.type));
    Token type_token = Parser.current;

    Consume(IDENTIFIER, "FunctionParams(): Expected identifier after '(', got '%s' instead",
            TokenTypeTranslation(Parser.next.type));
    Token identifier_token = Parser.current;

    if (IsIn(fn_params, identifier_token)) {
      ERROR_AND_EXIT_FMTMSG("FunctionParams(): duplicate parameter name '%.*s'",
                            identifier_token.length,
                            identifier_token.position_in_source);
    }
    Symbol stored_symbol = AddTo(fn_params, NewSymbol(identifier_token, AnnotateType(type_token.type), DECL_FN_PARAM));

    (*current)->nodes[LEFT] = NewNodeFromSymbol(IDENTIFIER_NODE, NULL, NULL, NULL, stored_symbol);
    (*current)->nodes[RIGHT] = NewNodeWithArity(FUNCTION_PARAM_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];

    Match(COMMA);
  }

  return params;
}

static AST_Node *FunctionReturnType() {
  Consume(RPAREN, "FunctionReturnType(): ')' required after function declaration");
  Consume(COLON_SEPARATOR, "FunctionReturnType(): '::' required after function declaration");
  ConsumeAnyType("FunctionReturnType(): Expected a type after '::'");

  Token fn_return_type = Parser.current;

  return NewNodeFromToken(FUNCTION_RETURN_TYPE_NODE, NULL, NULL, NULL, fn_return_type, AnnotateType(fn_return_type.type));
}

static AST_Node *FunctionBody(SymbolTable *fn_params) {
  if (NextTokenIs(SEMICOLON)) { return NULL; }

  Consume(LCURLY, "FunctionBody(): Expected '{' to begin function body, got '%s' instead", TokenTypeTranslation(Parser.next.type));

  AST_Node *body = NewNodeWithArity(FUNCTION_BODY_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &body;

  SymbolTable *remember_symbol_table = SYMBOL_TABLE;
  SYMBOL_TABLE = fn_params;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->nodes[LEFT] = Statement(UNUSED);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];
  }

  SYMBOL_TABLE = remember_symbol_table;

  Consume(RCURLY, "FunctionBody(): Expected '}' after function body");

  return body;
}

static AST_Node *FunctionDeclaration(Symbol symbol) {
  AST_Node *params = FunctionParams(symbol.fn_params);
  AST_Node *return_type = FunctionReturnType();
  AST_Node *body = FunctionBody(symbol.fn_params);

  if ((symbol.declaration_type == DECL_DECLARED) && body == NULL) {
    Symbol already_declared = RetrieveFrom(SYMBOL_TABLE, symbol.token);
    ERROR_AND_EXIT_FMTMSG("Double declaration of function '%.*s' (declared on line %d)\n",
                          symbol.token.length,
                          symbol.token.position_in_source,
                          already_declared.annotation.declared_on_line);
  }

  Symbol stored_symbol = AddTo(SYMBOL_TABLE, NewSymbol(symbol.token,
                                                      (symbol.declaration_type == DECL_DECLARED)
                                                        ? symbol.annotation
                                                        : FunctionAnnotation(return_type->token.type),
                                                      (body == NULL)
                                                        ? DECL_DECLARED
                                                        : DECL_DEFINED));

  return NewNodeFromSymbol(FUNCTION_NODE, return_type, params, body, stored_symbol);
}

static AST_Node *Literal(bool) {
  return NewNodeFromToken(LITERAL_NODE, NULL, NULL, NULL, Parser.current, NoAnnotation());
}

static AST_Node *BuildAST() {
  AST_Node *root = NewNodeWithArity(START_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

  AST_Node **current_node = &root;

  while (!Match(TOKEN_EOF)) {
    AST_Node *parse_result = Statement(UNUSED);
    if (parse_result == NULL) {
      ERROR_AND_EXIT("BuildAST(): AST could not be created");
    }

    AST_Node *next_statement = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    (*current_node)->nodes[LEFT] = parse_result;
    (*current_node)->nodes[RIGHT] = next_statement;

    current_node = &(*current_node)->nodes[RIGHT];
  }

  return root;
}

void Compile(const char *source) {
  InitLexer(source);
  InitParser();
  SYMBOL_TABLE = NewSymbolTable();

  AST_Node *ast = BuildAST();

  printf("\n[AST]\n");
  PrintAST(ast);
}
