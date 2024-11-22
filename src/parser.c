#include <stdio.h>

#include <limits.h> // for LONG_MIN and LONG_MAX (strtol error checking)
#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdlib.h> // for malloc

#include "ast.h"
#include "error.h"
#include "io.h"
#include "lexer.h"

/* Global Variables */
static SymbolTable *shadowed_symbol_table;
ErrorCode error_code = ERR_UNSET;

/* Forward Declarations */
static SymbolTable *SYMBOL_TABLE();
static AST_Node *StructField(Token struct_name);
static AST_Node *FunctionDeclaration(Symbol symbol);
static AST_Node *FunctionCall(Token identifier);
static AST_Node *InitializerList(ParserAnnotation expected_type);

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

/* Rules table Forward Declarations */
#define _ false
#define CAN_ASSIGN true

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

/*= == Scope Related === */
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
    ERROR_AND_EXIT("EndScope(): Please don't EndScope() when depth is 0.");
    SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
  }

  DeleteSymbolTable(Scope.locals[Scope.depth]);
  Scope.locals[Scope.depth] = NULL;
  Scope.depth--;
}

static void ShadowSymbolTable(SymbolTable *st) {
  shadowed_symbol_table = st;
}

static void UnshadowSymbolTable() {
  shadowed_symbol_table = NULL;
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

  return NewSymbol(SYMBOL_NOT_FOUND, NoAnnotation(), DECL_NONE);
}
/* === End Scope Related === */

static SymbolTable *SYMBOL_TABLE() {
  return (shadowed_symbol_table != NULL)
           ? shadowed_symbol_table
           : Scope.locals[Scope.depth];
}

static void Advance() {
  Parser.current = Parser.next;
  Parser.next = Parser.after_next;
  Parser.after_next = ScanToken();

  if (Parser.next.type != ERROR) return;

  // In case the very first token is an error token
  if (Parser.current.type == UNINITIALIZED) {
    ERROR_AT_TOKEN(Parser.next,
                   "Advance(): Error token encountered: '%.*s'",
                   Parser.next.length,
                   Parser.next.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_LEXER_ERROR);
  }

  ERROR_AT_TOKEN(Parser.current,
      "Advance(): Error token encountered after token '%s': %.*s",
      TokenTypeTranslation(Parser.current.type),
      Parser.next.length,
      Parser.next.position_in_source);
  SetErrorCodeIfUnset(&error_code, ERR_LEXER_ERROR);
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

  SetErrorCodeIfUnset(&error_code, (type == SEMICOLON)
                                     ? ERR_MISSING_SEMICOLON
                                     : ERR_UNEXPECTED);
  ERROR_AT_TOKEN_VALIST(Parser.next, msg, args);

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

  SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
  ERROR_AT_TOKEN_VALIST(Parser.next, msg, args);

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

  SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
  ERROR_AT_TOKEN_VALIST(Parser.next, msg, args);

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

  SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
  ERROR_AT_TOKEN_VALIST(Parser.next, msg, args);

  va_end(args);
}

void InitParser(SymbolTable **st) {
  /** Clean up / reset static variables, for running unit tests */
  /**/ if (*st != NULL) DeleteSymbolTable(*st);
  /**/ UnsetErrorCode(&error_code);

  *st = NewSymbolTable();

  Scope.depth = 0;
  Scope.locals[Scope.depth] = *st;

  /* Two calls to Advance() will prime the parser, such that
   * Parser.current will still be zeroed out, and
   * Parser.next will hold the First Token(TM) from the lexer.
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
    SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
    ERROR_AT_TOKEN(
      Parser.current,
      "Parse(): Unexpected token '%.*s'",
      Parser.current.length,
      Parser.current.position_in_source);
    return NewNode(UNTYPED, NULL, NULL, NULL, NoAnnotation());
  }

  bool can_assign = PrecedenceLevel <= ASSIGNMENT;
  AST_Node *prefix_node = prefix_rule(can_assign);

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) {
      SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
      ERROR_AT_TOKEN(
        Parser.current,
        "Parse(): Unexpected token '%.*s'",
        Parser.current.length,
        Parser.current.position_in_source);
      return NewNode(UNTYPED, NULL, NULL, NULL, NoAnnotation());
    }

    AST_Node *infix_node = infix_rule(can_assign);

    if (return_node == NULL) {
      LEFT_NODE(infix_node) = prefix_node;
    } else {
      LEFT_NODE(infix_node) = return_node;
      return_node = infix_node;
    }

    return_node = infix_node;
  }

  return (return_node == NULL) ? prefix_node : return_node;
}

static AST_Node *Type(bool) {
  Token type_token = Parser.current;
  bool is_array = false;
  long array_size = 0;

  if (Match(LBRACKET)) {
    if (Match(INT_LITERAL)) {
      array_size = strtol(Parser.current.position_in_source, NULL, 10);
      if (array_size == LONG_MIN) {
        ERROR_AND_EXIT("Type(): strtol underflowed");
        SetErrorCodeIfUnset(&error_code, ERR_UNDERFLOW);
      }
      if (array_size == LONG_MAX) {
        ERROR_AND_EXIT("Type(): strtol overflowed");
        SetErrorCodeIfUnset(&error_code, ERR_OVERFLOW);
      }
    }

    Consume(RBRACKET, "Type(): Expected ] after '%s', got '%s' instead.",
            TokenTypeTranslation(Parser.current.type),
            TokenTypeTranslation(Parser.next.type));

    is_array = true;
  }

  if (NextTokenIs(IDENTIFIER)) {
    if (type_token.type == VOID) {
      ERROR_AT_TOKEN(
        Parser.next,
        "Type(): Cannot use VOID as a type declaration", "");
      SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
    }

    if (IsIn(SYMBOL_TABLE(), Parser.next)) {
      Symbol s = RetrieveFrom(SYMBOL_TABLE(), Parser.next);
      REDECLARATION_AT_TOKEN(Parser.next,
                             s.token,
                             "Type(): Redeclaration of identifier '%.*s', previously declared on line %d\n",
                             Parser.next.length,
                             Parser.next.position_in_source,
                             s.annotation.declared_on_line);
      SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
    }

    if (TokenAfterNextIs(LPAREN)) {
      ERROR_AT_TOKEN(
        Parser.current,
        "Type(): Function declarations cannot be preceded by a type", "");
      SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
    }

    ParserAnnotation a = (is_array) ? ArrayAnnotation(type_token.type, array_size) : AnnotateType(type_token.type);
    AddTo(SYMBOL_TABLE(), NewSymbol(Parser.next, a, DECL_DECLARED));
  }

  Consume(IDENTIFIER, "Type(): Expected IDENTIFIER after Type '%s%s', got '%s' instead.",
          TokenTypeTranslation(type_token.type),
          (is_array) ? "[]" : "",
          TokenTypeTranslation(Parser.next.type));

  return Identifier(CAN_ASSIGN);
}

static AST_Node *Identifier(bool can_assign) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE(), Parser.current);
  bool is_in_symbol_table = IsIn(SYMBOL_TABLE(), Parser.current);
  AST_Node *array_index = NULL;
  Token identifier_token = Parser.current;

  if (Match(LPAREN)) {
    if (NextTokenIsAnyType() ||
        (NextTokenIs(RPAREN) && TokenAfterNextIs(COLON_SEPARATOR)))
    { // Declaration
      if (is_in_symbol_table && symbol.declaration_state != DECL_DECLARED) {
        REDECLARATION_AT_TOKEN(identifier_token,
                               symbol.token,
                               "Identifier(): Function '%.*s' has been redeclared, original declaration on line %d\n",
                               identifier_token.length,
                               identifier_token.position_in_source,
                               symbol.annotation.declared_on_line);
        SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
      }

      // TODO: Check for function definition in outer scope
      if (!is_in_symbol_table) AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, FunctionAnnotation(VOID), DECL_UNINITIALIZED));
      symbol = RetrieveFrom(SYMBOL_TABLE(), identifier_token);

      return FunctionDeclaration(symbol);
    } else { // Function call
      if (!is_in_symbol_table) {
        ERROR_AT_TOKEN(
          identifier_token,
          "Identifier(): Undeclared function", "");
        SetErrorCodeIfUnset(&error_code, ERR_UNDECLARED);
      } else if (symbol.declaration_state != DECL_DEFINED) {
        ERROR_AT_TOKEN(
          identifier_token,
          "Identifier(): Can't call an undefined function", "");
        SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
      }

      return FunctionCall(identifier_token);
    }
  }

  if (!is_in_symbol_table) {
    Symbol s = ExistsInOuterScope(identifier_token);
    if (s.token.type == ERROR) {
      ERROR_AT_TOKEN(identifier_token,
                     "Identifier(): Line %d: Undeclared identifier '%.*s'",
                     identifier_token.on_line,
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_UNDECLARED);
    }

    symbol = s;
    is_in_symbol_table = true;
  }

  if (symbol.declaration_state == DECL_NONE && can_assign) {
    Symbol already_declared = RetrieveFrom(SYMBOL_TABLE(), identifier_token);
    REDECLARATION_AT_TOKEN(identifier_token,
                           already_declared.token,
                           "Identifier(): Identifier '%.*s' has been redeclared. First declared on line %d\n",
                           identifier_token.length,
                           identifier_token.position_in_source,
                           already_declared.annotation.declared_on_line);
    SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
  }

  if (Match(LBRACKET)) {
    array_index = ArraySubscripting(_);
  }

  if (Match(PLUS_PLUS)) {
    if (symbol.declaration_state != DECL_DEFINED) {
      ERROR_AT_TOKEN(identifier_token,
                     "Identifier(): Cannot increment undefined variable '%.*s'",
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
    }

    return NewNodeFromToken(POSTFIX_INCREMENT_NODE, NULL, NULL, NULL, identifier_token, symbol.annotation);
  }

  if (Match(MINUS_MINUS)) {
    if (symbol.declaration_state != DECL_DEFINED) {
      ERROR_AT_TOKEN(identifier_token,
                     "Identifier(): Cannot decrement undefined variable '%.*s'",
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
    }

    return NewNodeFromToken(POSTFIX_DECREMENT_NODE, NULL, NULL, NULL, identifier_token, symbol.annotation);
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR_AT_TOKEN(identifier_token,
                     "Identifier(): Cannot assign to identifier '%.*s'",
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
    }

    if (symbol.annotation.is_array) {
      if (Match(LCURLY)) {
        AST_Node *initializer_list = InitializerList(symbol.annotation);
        symbol.declaration_state = DECL_DEFINED;
        symbol = AddTo(SYMBOL_TABLE(), symbol);
        return NewNodeFromSymbol(ASSIGNMENT_NODE, initializer_list, array_index, NULL, symbol);
      } else {
        ERROR_AT_TOKEN(identifier_token, "Identifier(): Arrays are not assignable", "");
        SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
      }
    }

    Symbol stored_symbol = AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, symbol.annotation, DECL_DEFINED));
    return NewNodeFromSymbol(ASSIGNMENT_NODE, Expression(_), array_index, NULL, stored_symbol);
  }

  if (NextTokenIsTerseAssignment()) {
    ConsumeAnyTerseAssignment("Identifier() Terse Assignment: How did this error message appear?");
    if (symbol.declaration_state != DECL_DEFINED) {
      ERROR_AT_TOKEN(identifier_token,
                     "Identifier(): Cannot perform a terse assignment on undefined variable '%.*s'",
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
    }

    AST_Node *terse_assignment = TerseAssignment(_);
    LEFT_NODE(terse_assignment) = NewNodeFromSymbol(IDENTIFIER_NODE, NULL, NULL, NULL, symbol);
    return terse_assignment;
  }

  if (symbol.annotation.ostensible_type == OST_STRUCT && Match(PERIOD)) {
    return StructField(identifier_token);
  }

  // Retrieve symbol from the table to use its declaration type and
  // annotation, but use the identifier_token to preserve the line number
  // for future error messages
  Symbol s = RetrieveFrom(SYMBOL_TABLE(), identifier_token);

  // Check for invalid syntax like "i64 i + 1;"
  if (s.declaration_state == DECL_DECLARED && !NextTokenIs(SEMICOLON)) {
    ERROR_AT_TOKEN(Parser.next, "Improper declaration", "");
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
  }

  return NewNodeFromToken(
    (s.declaration_state == DECL_DECLARED) ? DECLARATION_NODE
                                           : IDENTIFIER_NODE,
    NULL, array_index, NULL, identifier_token, s.annotation
  );
}

static AST_Node *Unary(bool) {
  if (NextTokenIsAnyType()) {
    ERROR_AT_TOKEN(
      Parser.next,
      "Unary(): Can't declare variable in the middle of an expression", "");
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
  }

  Token operator_token = Parser.current;
  Token token_after_operator = Parser.next; // for error messages
  AST_Node *parse_result = Parse(UNARY);

  switch(operator_token.type) {
    case PLUS_PLUS: {
      if (token_after_operator.type != IDENTIFIER) {
        ERROR_AT_TOKEN(token_after_operator,
          "Unary(): Expected Identifier, got '%s' instead\n",
          TokenTypeTranslation(token_after_operator.type));
        SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
      }

      Symbol s = RetrieveFrom(SYMBOL_TABLE(), token_after_operator);
      if (s.declaration_state != DECL_DEFINED) {
        ERROR_AT_TOKEN(
          token_after_operator,
          "Unary(): Can't increment undefined operator '%.*s'",
          token_after_operator.length, token_after_operator.position_in_source);
        SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
      }

      return NewNodeFromToken(PREFIX_INCREMENT_NODE, parse_result, NULL, NULL, operator_token, NoAnnotation());
    } break;
    case MINUS_MINUS: {
      if (token_after_operator.type != IDENTIFIER) {
        ERROR_AT_TOKEN(token_after_operator,
          "Unary(): Expected Identifier, got '%s' instead\n",
          TokenTypeTranslation(token_after_operator.type));
        SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
      }

      Symbol s = RetrieveFrom(SYMBOL_TABLE(), token_after_operator);
      if (s.declaration_state != DECL_DEFINED) {
        ERROR_AT_TOKEN(
          token_after_operator,
          "Unary(): Can't decrement undefined operator '%.*s'",
          token_after_operator.length, token_after_operator.position_in_source);
        SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
      }

      return NewNodeFromToken(PREFIX_DECREMENT_NODE, parse_result, NULL, NULL, operator_token, NoAnnotation());
    } break;
    case LOGICAL_NOT:
    case MINUS:
      return NewNodeFromToken(UNARY_OP_NODE, parse_result, NULL, NULL, operator_token, NoAnnotation());
    default:
      ERROR_AT_TOKEN(operator_token,
                     "Unary(): Unknown Unary operator '%s'\n",
                     TokenTypeTranslation(operator_token.type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
      return NULL;
  }
}

static AST_Node *Binary(bool) {
  Token operator_token = Parser.current;

  if (NextTokenIsAnyType()) {
    ERROR_AT_TOKEN(
      Parser.next,
      "Binary(): Can't declare variable in the middle of an expression", "");
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
  }

  Precedence precedence = Rules[Parser.current.type].precedence;
  AST_Node *parse_result = Parse(precedence + 1);

  switch(operator_token.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
    case MODULO:
      return NewNodeFromToken(BINARY_ARITHMETIC_NODE, NULL, NULL, parse_result, operator_token, NoAnnotation());
    case EQUALITY:
    case LOGICAL_AND:
    case LOGICAL_OR:
    case LOGICAL_NOT_EQUALS:
    case LESS_THAN:
    case GREATER_THAN:
    case LESS_THAN_EQUALS:
    case GREATER_THAN_EQUALS:
      return NewNodeFromToken(BINARY_LOGICAL_NODE, NULL, NULL, parse_result, operator_token, AnnotateType(BOOL));
    case BITWISE_XOR:
    case BITWISE_NOT:
    case BITWISE_AND:
    case BITWISE_OR:
    case BITWISE_LEFT_SHIFT:
    case BITWISE_RIGHT_SHIFT:
      return NewNodeFromToken(BINARY_BITWISE_NODE, NULL, NULL, parse_result, operator_token, NoAnnotation());
    default:
      ERROR_AT_TOKEN(operator_token,
                     "Binary(): Unknown operator '%s'\n",
                     TokenTypeTranslation(operator_token.type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
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
    case BITWISE_NOT_EQUALS:
    case BITWISE_LEFT_SHIFT_EQUALS:
    case BITWISE_RIGHT_SHIFT_EQUALS:
      return NewNodeFromToken(TERSE_ASSIGNMENT_NODE, NULL, NULL, parse_result, operator_token, NoAnnotation());
    default:
      ERROR_AT_TOKEN(operator_token,
                     "TerseAssignment(): Unknown operator '%s'\n",
                     TokenTypeTranslation(operator_token.type));
      SetErrorCodeIfUnset(&error_code, ERR_PEBCAK);
      return NULL;
  }
}

static AST_Node *Block(bool) {
  AST_Node *n = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    LEFT_NODE(*current) = Statement(_);
    RIGHT_NODE(*current) = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    current = &RIGHT_NODE(*current);
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

  return NewNode(IF_NODE, condition, body_if_true, body_if_false, NoAnnotation());
}

static AST_Node *TernaryIfStmt(AST_Node *condition) {
  Consume(QUESTION_MARK, "TernaryIfStmt(): Expected '?' after Ternary Condition, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *if_true = Expression(_);

  Consume(COLON, "TernaryIfStmt(): Expected ':' after Ternary Statement, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *if_false = Expression(_);

  return NewNode(IF_NODE, condition, if_true, if_false, NoAnnotation());
}

static AST_Node *WhileStmt(bool) {
  AST_Node *condition = Expression(_);
  Consume(LCURLY, "WhileStmt(): Expected '{' after While condition, got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *block = Block(_);
  Match(SEMICOLON);
  return NewNode(WHILE_NODE, condition, NULL, block, NoAnnotation());
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

  while (RIGHT_NODE(*find_last_body_statement) != NULL) find_last_body_statement = &RIGHT_NODE(*find_last_body_statement);

  LEFT_NODE(*find_last_body_statement) = after_loop;

  AST_Node *while_node = NewNode(WHILE_NODE, condition, NULL, body, NoAnnotation());
  return NewNode(FOR_NODE, initialization, NULL, while_node, NoAnnotation());
}

static AST_Node *Break(bool) {
  if (!NextTokenIs(SEMICOLON)) {
    ERROR_AT_TOKEN(Parser.next,
                   "Break(): Expected ';' after Break, got '%s' instead",
                   TokenTypeTranslation(Parser.next.type));
    SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
  }

  return NewNode(BREAK_NODE, NULL, NULL, NULL, NoAnnotation());
}

static AST_Node *Continue(bool) {
  if (!NextTokenIs(SEMICOLON)) {
    ERROR_AT_TOKEN(Parser.next,
                   "Continue(): Expected ';' after Continue, got '%s' instead",
                   TokenTypeTranslation(Parser.next.type));
    SetErrorCodeIfUnset(&error_code, ERR_UNEXPECTED);
  }

  return NewNode(CONTINUE_NODE, NULL, NULL, NULL, NoAnnotation());
}

static AST_Node *Return(bool) {
  AST_Node *expr = NULL;

  if (!NextTokenIs(SEMICOLON)) {
    expr = Expression(_);
  }

  return NewNode(RETURN_NODE, expr, NULL, NULL, (expr == NULL) ? AnnotateType(VOID) : expr->annotation);
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
      ERROR_AT_TOKEN(Parser.current,
                     "ArraySubscripting(): Can't access array with undeclared identifier '%.*s'",
                     Parser.current.length,
                     Parser.current.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_UNDECLARED);
    }

    if (symbol.declaration_state != DECL_DEFINED) {
      ERROR_AT_TOKEN(Parser.current,
                     "ArraySubscripting(): Can't access array with uninitialized identifier '%.*s'",
                     Parser.current.length,
                     Parser.current.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_UNINITIALIZED);
    }

    return_value = NewNodeFromSymbol(ARRAY_SUBSCRIPT_NODE, NULL, NULL, NULL, symbol);
  } else if (Match(INT_LITERAL)) {
    return_value = NewNodeFromToken(ARRAY_SUBSCRIPT_NODE, NULL, NULL, NULL, Parser.current, AnnotateType(Parser.current.type));
  }

  Consume(RBRACKET, "ArraySubscripting(): Where's the ']'?");

  return return_value;
}

static AST_Node *EnumIdentifier(bool can_assign) {
  Symbol symbol = RetrieveFrom(SYMBOL_TABLE(), Parser.current);
  bool is_in_symbol_table = IsIn(SYMBOL_TABLE(), Parser.current);
  Token identifier_token = Parser.current;

  if (!is_in_symbol_table) {
    ERROR_AT_TOKEN(identifier_token,
                   "EnumIdentifier(): Line %d: Undeclared identifier '%.*s'",
                   identifier_token.on_line,
                   identifier_token.length,
                   identifier_token.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_UNDECLARED);
  }

  if (symbol.declaration_state == DECL_NONE && can_assign) {
    Symbol already_declared = RetrieveFrom(SYMBOL_TABLE(), identifier_token);
    REDECLARATION_AT_TOKEN(identifier_token,
                           already_declared.token,
                           "EnumIdentifier(): Identifier '%.*s' has been redeclared. First declared on line %d\n",
                           identifier_token.length,
                           identifier_token.position_in_source,
                           already_declared.annotation.declared_on_line);
    SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR_AT_TOKEN(identifier_token,
                     "EnumIdentifier(): Cannot assign to identifier '%.*s'",
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
    }

    Symbol stored_symbol = AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, symbol.annotation, DECL_DEFINED));
    return NewNodeFromSymbol(ENUM_ASSIGNMENT_NODE, Expression(_), NULL, NULL, stored_symbol);
  }

  return NewNodeFromToken(ENUM_IDENTIFIER_NODE, NULL, NULL, NULL, identifier_token, AnnotateType(ENUM_LITERAL));
}

static AST_Node *EnumBlock() {
  AST_Node *n = NULL;
  AST_Node **current = &n;

  Consume(LCURLY, "EnumBlock(): Expected '{' after ENUM declaration, got %s", TokenTypeTranslation(Parser.current.type));

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    if (n == NULL) n = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    Symbol symbol = RetrieveFrom(SYMBOL_TABLE(), Parser.next);
    bool is_in_symbol_table = IsIn(SYMBOL_TABLE(), Parser.next);

    if (is_in_symbol_table) {
      ERROR_AT_TOKEN(Parser.next,
                     "EnumBlock(): Enum identifier '%.*s' already exists, declared on line %d",
                     Parser.next.length,
                     Parser.next.position_in_source,
                     symbol.annotation.declared_on_line);
      SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
    }

    Consume(IDENTIFIER, "EnumBlock(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
            TokenTypeTranslation(Parser.current.type),
            TokenTypeTranslation(Parser.next.type));
    AddTo(SYMBOL_TABLE(), NewSymbol(Parser.current, AnnotateType(ENUM_LITERAL), DECL_DEFINED));

    LEFT_NODE(*current) = EnumIdentifier(CAN_ASSIGN);
    RIGHT_NODE(*current) = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    current = &RIGHT_NODE(*current);

    Match(COMMA);
  }

  Consume(RCURLY, "EnumBlock(): Expected '}' after ENUM block, got %s", TokenTypeTranslation(Parser.current.type));

  if (n == NULL) {
     SetErrorCodeIfUnset(&error_code, ERR_EMPTY_BODY);
     ERROR_AT_TOKEN(Parser.current,
                    "EnumBlock(): Enum body cannot be declared", "");
  }

  return n;
}

static AST_Node *Enum(bool) {
  Consume(IDENTIFIER, "Enum(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(Parser.next.type),
          TokenTypeTranslation(Parser.next.type));

  Token enum_identifier = Parser.current;
  Symbol stored_symbol = RetrieveFrom(SYMBOL_TABLE(), enum_identifier);

  if (stored_symbol.declaration_state == DECL_DEFINED) {
    REDECLARATION_AT_TOKEN(
      enum_identifier,
      stored_symbol.token,
      "Redeclaration of Enum '%.*s', original declaration on Line %d",
      stored_symbol.token.length,
      stored_symbol.token.position_in_source,
      stored_symbol.annotation.declared_on_line);
    SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
  }

  AddTo(SYMBOL_TABLE(), NewSymbol(enum_identifier, AnnotateType(ENUM), DECL_UNINITIALIZED));

  AST_Node *enum_name = Identifier(false);
  LEFT_NODE(enum_name) = EnumBlock();

  AddTo(SYMBOL_TABLE(), NewSymbol(enum_identifier, AnnotateType(ENUM), DECL_DEFINED));
  return enum_name;
}

static AST_Node *StructField(Token struct_name) {
  AST_Node *expr = NULL;
  AST_Node *array_index = NULL;
  Symbol struct_symbol = RetrieveFrom(SYMBOL_TABLE(), struct_name);
  if (struct_symbol.declaration_state != DECL_DEFINED) {
    ERROR_AT_TOKEN(
      Parser.next,
      "StructField(): Can't use field from undefined struct", "");
    SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
  }

  BeginScope();
  ShadowSymbolTable(struct_symbol.struct_fields);

  Consume(IDENTIFIER, "StructField(): Expected identifier", "");
  Token field_name = Parser.current;
  if (!IsIn(SYMBOL_TABLE(), field_name)) {
    ERROR_AT_TOKEN(
      field_name,
      "StructField(): Struct '%.*s' has no field '%.*s'",
      struct_name.length, struct_name.position_in_source,
      field_name.length, field_name.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
  }

  if (Match(LBRACKET)) {
    array_index = ArraySubscripting(_);
  }

  if (Match(EQUALS)) {
    expr = Expression(_);
    Symbol s = RetrieveFrom(SYMBOL_TABLE(), field_name);
    s.declaration_state = DECL_DEFINED;
    AddTo(SYMBOL_TABLE(), s);
  }

  Symbol field_symbol = RetrieveFrom(SYMBOL_TABLE(), field_name);
  if (field_symbol.declaration_state != DECL_DEFINED) {
    ERROR_AT_TOKEN(
      field_name,
      "StructField(): Field '%.*s' has not been defined",
      field_name.length,
      field_name.position_in_source);
    SetErrorCodeIfUnset(&error_code, ERR_UNDEFINED);
  }

  UnshadowSymbolTable();
  EndScope();

  return NewNodeFromToken(STRUCT_FIELD_IDENTIFIER_NODE, expr, array_index, NULL, field_name, field_symbol.annotation);
}

static AST_Node *Struct() {
  Consume(IDENTIFIER, "Struct(): Expected IDENTIFIER after Type '%s, got '%s instead",
          TokenTypeTranslation(Parser.current.type),
          TokenTypeTranslation(Parser.next.type));
  Token identifier_token = Parser.current;

  if (IsIn(SYMBOL_TABLE(), identifier_token)) {
    Symbol existing_struct = RetrieveFrom(SYMBOL_TABLE(), identifier_token);
    ERROR_AT_TOKEN(identifier_token,
                   "Struct(): Struct '%.*s' is already in symbol table, declared on line %d\n",
                   identifier_token.length,
                   identifier_token.position_in_source,
                   existing_struct.annotation.declared_on_line);
    SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
  }
  Symbol identifier_symbol = AddTo(SYMBOL_TABLE(), NewSymbol(identifier_token, AnnotateType(STRUCT), DECL_DECLARED));

  ShadowSymbolTable(identifier_symbol.struct_fields);

  Consume(LCURLY, "Struct(): Expected '{' after STRUCT declaration, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  AST_Node *n = NULL;
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    if (n == NULL) n = NewNodeFromSymbol(STRUCT_FIELD_IDENTIFIER_NODE, NULL, NULL, NULL, identifier_symbol);

    LEFT_NODE(*current) = Statement(_);
    RIGHT_NODE(*current) = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    current = &RIGHT_NODE(*current);
  }

  Consume(RCURLY, "Struct(): Expected '}' after STRUCT block, got '%s' instead",
          TokenTypeTranslation(Parser.next.type));

  UnshadowSymbolTable();

  if (n == NULL) {
    SetErrorCodeIfUnset(&error_code, ERR_EMPTY_BODY);
    ERROR_AT_TOKEN(identifier_symbol.token,
                   "Struct(): Struct '%.*s' has empty body",
                   identifier_symbol.token.length,
                   identifier_symbol.token.position_in_source);
  }

  identifier_symbol.declaration_state = DECL_DEFINED;
  AddTo(SYMBOL_TABLE(), identifier_symbol);

  return n;
}

static AST_Node *InitializerList(ParserAnnotation expected_type) {
  AST_Node *n = NULL;
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    if (n == NULL) n = NewNode(ARRAY_INITIALIZER_LIST_NODE, NULL, NULL, NULL, expected_type);

    LEFT_NODE(*current) = Expression(_);
    RIGHT_NODE(*current) = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    current = &RIGHT_NODE(*current);

    Match(COMMA);
  }

  Consume(RCURLY, "InitializerList(): Expected '}' after Initializer List", "");

  if (n == NULL) {
    ERROR_AT_TOKEN(
      Parser.current,
      "InitializerList(): Initializer List cannot be empty", "");
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_ASSIGNMENT);
  }

  return n;
}

static AST_Node *FunctionParams(SymbolTable *fn_params, Symbol identifier) {
  AST_Node *params = NewNode(FUNCTION_PARAM_NODE, NULL, NULL, NULL, NoAnnotation());
  AST_Node **current = &params;

  while (!NextTokenIs(RPAREN) && !NextTokenIs(TOKEN_EOF)) {
    identifier = RetrieveFrom(SYMBOL_TABLE(), identifier.token);

    ConsumeAnyType("FunctionParams(): Expected a type, got '%s' instead", TokenTypeTranslation(Parser.next.type));
    Token type_token = Parser.current;

    if (type_token.type == VOID) {
      ERROR_AT_TOKEN(
        Parser.current,
        "FunctionParams(): Cannot declare a function parameter VOID", "");
      SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
    }

    bool is_array = false;
    if (Match(LBRACKET)) {
      Consume(RBRACKET, "FunctionParams(): Expected ']' after '['");
      is_array = true;
    }

    Consume(IDENTIFIER, "FunctionParams(): Expected identifier after '(', got '%s' instead",
            TokenTypeTranslation(Parser.next.type));
    Token identifier_token = Parser.current;

    Symbol existing_symbol = RetrieveFrom(SYMBOL_TABLE(), identifier.token);
    if (IsIn(fn_params, identifier_token) && existing_symbol.declaration_state != DECL_DECLARED) {
      ERROR_AT_TOKEN(identifier_token,
                     "FunctionParams(): Duplicate parameter name '%.*s'",
                     identifier_token.length,
                     identifier_token.position_in_source);
      SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
    }
    Symbol stored_symbol = AddTo(fn_params,
                                 NewSymbol(identifier_token,
                                           (is_array)
                                             ? ArrayAnnotation(type_token.type, 0)
                                             : AnnotateType(type_token.type),
                                           DECL_DEFINED)
    );
    RegisterFnParam(SYMBOL_TABLE(), identifier, stored_symbol);

    (*current)->annotation = stored_symbol.annotation;
    (*current)->token = identifier_token;

    if (Match(COMMA) || !NextTokenIs(RPAREN)) {
      LEFT_NODE(*current) = NewNode(FUNCTION_PARAM_NODE, NULL, NULL, NULL, NoAnnotation());

      current = &LEFT_NODE(*current);
    }
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

  AST_Node *body = NewNode(FUNCTION_BODY_NODE, NULL, NULL, NULL, NoAnnotation());
  AST_Node **current = &body;

  BeginScope();
  ShadowSymbolTable(fn_params);

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    LEFT_NODE(*current) = Statement(_);
    RIGHT_NODE(*current) = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    current = &RIGHT_NODE(*current);
  }


  Consume(RCURLY, "FunctionBody(): Expected '}' after function body");

  EndScope();
  UnshadowSymbolTable();

  if (LEFT_NODE(body) == NULL) { // Insert a Void Return if there's no function body
    LEFT_NODE(body) = NewNode(RETURN_NODE, NULL, NULL, NULL, AnnotateType(VOID));
  }

  return body;
}

static AST_Node *FunctionDeclaration(Symbol symbol) {
  if (Scope.depth != 0) {
    ERROR_AT_TOKEN(symbol.token, "FunctionDeclaration(): Functions must be declared in global scope.", "");
    SetErrorCodeIfUnset(&error_code, ERR_IMPROPER_DECLARATION);
  }

  AST_Node *params = FunctionParams(symbol.fn_params, symbol);
  AST_Node *return_type = FunctionReturnType();
  AST_Node *body = FunctionBody(symbol.fn_params);

  if ((symbol.declaration_state == DECL_DECLARED) && body == NULL) {
    Symbol already_declared = RetrieveFrom(SYMBOL_TABLE(), symbol.token);
    ERROR_AT_TOKEN(symbol.token,
                   "FunctionDeclaration(): Redeclaration of function '%.*s' (declared on line %d)\n",
                   symbol.token.length,
                   symbol.token.position_in_source,
                   already_declared.annotation.declared_on_line);
    SetErrorCodeIfUnset(&error_code, ERR_REDECLARED);
  }
  // Retrieve updated reference to symbol before modifying
  symbol = RetrieveFrom(SYMBOL_TABLE(), symbol.token);

  symbol.annotation = (symbol.declaration_state == DECL_DECLARED)
                        ? symbol.annotation
                        : FunctionAnnotation(return_type->token.type);
  symbol.declaration_state = (body == NULL) ? DECL_DECLARED : DECL_DEFINED;
  Symbol updated_symbol = AddTo(SYMBOL_TABLE(), symbol);

  return NewNodeFromSymbol((body == NULL) ? DECLARATION_NODE : FUNCTION_NODE, return_type, params, body, updated_symbol);
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

      (*current) = NewNodeFromToken(FUNCTION_ARGUMENT_NODE, NULL, NULL, NULL, literal, AnnotateType(literal.type));
    }

    if (Match(COMMA)) {
      if (NextTokenIs(RPAREN)) { break; }

      RIGHT_NODE(*current) = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());
      current = &RIGHT_NODE(*current);
    }
  }

  Consume(RPAREN, "FunctionCall(): Expected ')'");

  Symbol fn_definition = RetrieveFrom(SYMBOL_TABLE(), function_name);

  return NewNodeFromToken(FUNCTION_CALL_NODE, NULL, args, NULL, function_name, fn_definition.annotation);
}

static AST_Node *Literal(bool) {
  return NewNodeFromToken(LITERAL_NODE, NULL, NULL, NULL, Parser.current, AnnotateType(Parser.current.type));
}

AST_Node *ParserBuildAST() {
  AST_Node *root = NewNode(START_NODE, NULL, NULL, NULL, NoAnnotation());

  AST_Node **current_node = &root;

  while (!Match(TOKEN_EOF)) {
    AST_Node *parse_result = Statement(_);
    if (parse_result == NULL) {
      SetErrorCodeIfUnset(&error_code, ERR_MISC);
      ERROR_AND_EXIT("ParserBuildAST(): AST could not be created");
    }

    AST_Node *next_statement = NewNode(CHAIN_NODE, NULL, NULL, NULL, NoAnnotation());

    LEFT_NODE(*current_node) = parse_result;
    RIGHT_NODE(*current_node) = next_statement;

    current_node = &RIGHT_NODE(*current_node);
  }

  SetErrorCodeIfUnset(&root->error_code, error_code);
  return root;
}
