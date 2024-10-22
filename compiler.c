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

HashTable *SymbolTable;

#define UNUSED false
#define CAN_ASSIGN true

static ParserAnnotation NO_ANNOTATION = {
  .ostensible_type = OST_UNKNOWN,
  .bit_width = 0,
  .is_signed = 0,
  .declared_on_line = -1
};

struct {
  Token current;
  Token next;
} Parser;

typedef enum {
  PREC_EOF = -1,
  NO_PRECEDENCE,
  ASSIGNMENT,
  TERM,
  FACTOR,
  UNARY,
} Precedence;

typedef AST_Node* (*ParseFn)(bool);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static AST_Node *Type(bool unused);
static AST_Node *Identifier(bool can_assign);
static AST_Node *Number(bool unused);
static AST_Node *Char(bool unused);
static AST_Node *Enum(bool unused);
static AST_Node *StringLiteral(bool unused);
static AST_Node *Unary(bool unused);
static AST_Node *Binary(bool unused);
static AST_Node *Parens(bool unused);
static AST_Node *Block(bool unused);
static AST_Node *Expression(bool unused);
static AST_Node *Statement(bool unused);
static AST_Node *IfStmt(bool unused);

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
  [STRUCT]         = {   Type,   NULL, NO_PRECEDENCE },

  [IDENTIFIER]     = { Identifier, NULL, NO_PRECEDENCE },

  // Constants
  [BINARY_CONSTANT] = { Number,  NULL, NO_PRECEDENCE },
  [HEX_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [INT_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [FLOAT_CONSTANT] = { Number,   NULL, NO_PRECEDENCE },
  [ENUM_CONSTANT]  = { Identifier, NULL, NO_PRECEDENCE },
  [CHAR_CONSTANT]  = {   Char,   NULL, NO_PRECEDENCE },

  [STRING_LITERAL] = { StringLiteral,   NULL, NO_PRECEDENCE },

  // Punctuators
  [LPAREN]         = { Parens,   NULL, NO_PRECEDENCE },
  [PLUS]           = {   NULL, Binary,          TERM },
  [MINUS]          = {  Unary, Binary,          TERM },
  [ASTERISK]       = {   NULL, Binary,        FACTOR },
  [DIVIDE]         = {   NULL, Binary,        FACTOR },

  // Misc
  [TOKEN_EOF]      = {   NULL,   NULL,      PREC_EOF },
};

static ParserAnnotation Annotation(OstensibleType type, int bit_width, bool is_signed) {
  ParserAnnotation a = {
    .ostensible_type = type,
    .bit_width = bit_width,
    .is_signed = is_signed
  };

  return a;
}

static ParserAnnotation AnnotateType(TokenType t) {
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
    case STRING: return Annotation(OST_STRING, 0, 0);

    default:
      printf("Unimplemented ToOstensibleType for TokenType '%s'\n", TokenTypeTranslation(t));
      return Annotation(OST_UNKNOWN, 0, 0);
  }
}

static void Advance() {
  Parser.current = Parser.next;
  Parser.next = ScanToken();

  if (Parser.next.type != ERROR) return;

  ERROR_AND_EXIT_FMTMSG("Advance(): Error token encountered after token '%s': %.*s",
      TokenTypeTranslation(Parser.current.type),
      Parser.next.length,
      Parser.next.position_in_source);
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

static bool NextTokenIs(TokenType type) {
  return (Parser.next.type == type);
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

static AST_Node *StringLiteral(bool) {
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NO_ANNOTATION);
}

static AST_Node *Number(bool) {
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NO_ANNOTATION);
}

static AST_Node *Char(bool) {
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NO_ANNOTATION);
}

static AST_Node *EnumBlock() {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);
  AST_Node **current = &n;

  Consume(LCURLY, "Expected '{' after ENUM declaration, got %.*s", TokenTypeTranslation(Parser.current.type));

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    HT_Entry symbol = RetrieveFromSymbolTable(SymbolTable, Parser.current);
    bool is_in_symbol_table = IsInSymbolTable(SymbolTable, Parser.current);

    if (is_in_symbol_table) {
      ERROR_AND_EXIT_FMTMSG("Enum identifier '%.*s' already exists, declared on line %d",
                            Parser.next.length,
                            Parser.next.position_in_source,
                            symbol.annotation.declared_on_line);
    }

    AddToSymbolTable(SymbolTable, Entry(Parser.next, NO_ANNOTATION, DECL_NOT_APPLICABLE));
    Consume(IDENTIFIER, "Expected IDENTIFIER after Type '%s', got '%s' instead.",
            TokenTypeTranslation(Parser.current.type),
            TokenTypeTranslation(Parser.next.type));

    (*current)->nodes[LEFT] = Identifier(CAN_ASSIGN);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);

    current = &(*current)->nodes[RIGHT];

    if (NextTokenIs(COMMA)) Consume(COMMA, "");
  }

  Consume(RCURLY, "Expected '}' after ENUM block, got %.*s", TokenTypeTranslation(Parser.current.type));

  return n;
}

static AST_Node *Enum(bool) {
  ParserAnnotation a = AnnotateType(Parser.current.type);
  a.declared_on_line = Parser.next.on_line;
  AddToSymbolTable(SymbolTable, Entry(Parser.next, a, DECL_AWAITING_INIT));

  Consume(IDENTIFIER, "Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(Parser.next.type),
          TokenTypeTranslation(Parser.next.type));

  AST_Node *enum_name = Identifier(false);
  enum_name->nodes[LEFT] = EnumBlock();

  return enum_name;
}

static AST_Node *Type(bool) {
  Token remember_token = Parser.current;

  if (NextTokenIs(IDENTIFIER)) {
    if (IsInSymbolTable(SymbolTable, Parser.next)) {
      HT_Entry e = RetrieveFromSymbolTable(SymbolTable, Parser.next);
      ERROR_AND_EXIT_FMTMSG("Re-declaration of identifier '%.*s', previously declared on line %d\n",
                            Parser.next.length,
                            Parser.next.position_in_source,
                            e.annotation.declared_on_line);
    }

    ParserAnnotation a = AnnotateType(Parser.current.type);
    a.declared_on_line = Parser.next.on_line;
    AddToSymbolTable(SymbolTable, Entry(Parser.next, a, DECL_AWAITING_INIT));
  }

  Consume(IDENTIFIER, "Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(remember_token.type),
          TokenTypeTranslation(Parser.next.type));

  return Identifier(CAN_ASSIGN);
}

static AST_Node *Identifier(bool can_assign) {
  HT_Entry symbol = RetrieveFromSymbolTable(SymbolTable, Parser.current);
  bool is_in_symbol_table = IsInSymbolTable(SymbolTable, Parser.current);
  bool awaiting_init = symbol.declaration_type == DECL_AWAITING_INIT;

  Token remember_token = Parser.current;

  if (!is_in_symbol_table) {
    ERROR_AND_EXIT_FMTMSG("Line %d: Undeclared identifier '%.*s'",
                          remember_token.on_line,
                          remember_token.length,
                          remember_token.position_in_source);
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR_AND_EXIT_FMTMSG("Cannot assign to identifier '%.*s'",
                            remember_token.length,
                            remember_token.position_in_source);
    }

    AddToSymbolTable(SymbolTable, Entry(remember_token, symbol.annotation, DECL_INITIALIZED));
    return NewNodeWithToken(IDENTIFIER_NODE, Expression(UNUSED), NULL, NULL, remember_token, symbol.annotation);
  }

  if (NextTokenIs(SEMICOLON)) {
    if (!awaiting_init && can_assign) {
      HT_Entry already_declared = RetrieveFromSymbolTable(SymbolTable, remember_token);
      ERROR_AND_EXIT_FMTMSG("Identifier '%.*s' has been redeclared. First declared on line %d\n",
                            remember_token.length,
                            remember_token.position_in_source,
                            already_declared.annotation.declared_on_line);
    }

    return NewNodeWithToken(IDENTIFIER_NODE, NULL, NULL, NULL, remember_token, NO_ANNOTATION);
  }

  HT_Entry e = RetrieveFromSymbolTable(SymbolTable, remember_token);
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, e.token, e.annotation);
}

static AST_Node *Unary(bool) {
  Token remember_token = Parser.current;
  AST_Node *parse_result = Parse(UNARY);

  switch(remember_token.type) {
    case MINUS:
      return NewNodeWithToken(UNTYPED, parse_result, NULL, NULL, remember_token, NO_ANNOTATION);
    default:
      printf("Unknown Unary operator '%s'\n",
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
      return NewNodeWithToken(UNTYPED, NULL, NULL, parse_result, remember_token, NO_ANNOTATION);
    default:
      printf("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
      return NULL;
  }
}

static AST_Node *Block(bool) {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->nodes[LEFT] = Statement(UNUSED);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);

    current = &(*current)->nodes[RIGHT];
  }

  Consume(RCURLY, "Expected '}' after Block, got '%s' instead.", TokenTypeTranslation(Parser.next.type));

  return n;
}

static AST_Node *Expression(bool) {
  return Parse((Precedence)1);
}

static AST_Node *Statement(bool) {
  if (Match(IF)) return IfStmt(UNUSED);

  AST_Node *expr_result = Expression(UNUSED);
  Consume(SEMICOLON, "A ';' is expected after an expression statement, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  return expr_result;
}

static AST_Node *IfStmt(bool) {
  Consume(LPAREN, "Expected '(' after IF token, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));
  AST_Node *condition = Expression(UNUSED);
  Consume(RPAREN, "Expected ')' after IF condition, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  Consume(LCURLY, "Expected '{', got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *body_if_true = Block(UNUSED);
  AST_Node *body_if_false = NULL;

  if (Match(ELSE)) {
    if (Match(IF))  {
      body_if_false = IfStmt(UNUSED);
    } else {
      Consume(LCURLY, "Expected block starting with '{' after ELSE, got '%s' instead", TokenTypeTranslation(Parser.next.type));
      body_if_false = Block(UNUSED);
    }
  }

  return NewNode(IF_NODE, condition, body_if_true, body_if_false, NO_ANNOTATION);
}

static AST_Node *Parens(bool) {
  AST_Node *parse_result = Expression(UNUSED);
  Consume(RPAREN, "Missing ')' after expression");

  return parse_result;
}

static AST_Node *BuildAST() {
  AST_Node *root = NewNodeWithArity(START_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);

  AST_Node **current_node = &root;

  while (!Match(TOKEN_EOF)) {
    AST_Node *parse_result = Statement(UNUSED);
    if (parse_result == NULL) {
      ERROR_AND_EXIT("AST could not be created");
    }

    AST_Node *next_statement = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);

    (*current_node)->nodes[LEFT] = parse_result;
    (*current_node)->nodes[RIGHT] = next_statement;

    current_node = &(*current_node)->nodes[RIGHT];
  }

  return root;
}

void Compile(const char *source) {
  InitLexer(source);
  InitParser();
  SymbolTable = NewHashTable();

  AST_Node *ast = BuildAST();

  printf("\n[AST]\n");
  PrintAST(ast);
}
