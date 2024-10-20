#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdio.h>  // for printf and friends

#include "ast.h"
#include "common.h"
#include "compiler.h"
#include "error.h"
#include "lexer.h"

typedef AST_Node* (*ParseFn)();

struct {
  Token current;
  Token next;
} Parser;

typedef enum {
  PREC_EOF = -1,
  NO_PRECEDENCE,
  TERM,
  FACTOR,
  UNARY,
} Precedence;

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static AST_Node *Type();
static AST_Node *Identifier();
static AST_Node *Number();
static AST_Node *StringLiteral();
static AST_Node *Unary();
static AST_Node *Binary();
static AST_Node *Parens();
static AST_Node *Block();
static AST_Node *Expression();
static AST_Node *Statement();
static AST_Node *IfStmt();

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
  [ENUM]           = {   Type,   NULL, NO_PRECEDENCE },
  [STRUCT]         = {   Type,   NULL, NO_PRECEDENCE },

  [IDENTIFIER]     = { Identifier, NULL, NO_PRECEDENCE },

  // Constants
  [BINARY_CONSTANT] = { Number,  NULL, NO_PRECEDENCE },
  [HEX_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [INT_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [FLOAT_CONSTANT] = { Number,   NULL, NO_PRECEDENCE },

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

  AST_Node *prefix_node = prefix_rule();

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) {
      ERROR_AND_EXIT_FMTMSG("Infix Rule for '%s' is NULL.\n",
                            TokenTypeTranslation(Parser.current.type));
    }

    AST_Node *infix_node = infix_rule();

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

static AST_Node *StringLiteral() {
  return NewNodeWithToken(UNTYPED, NULL, NULL, NULL, Parser.current);
}

static AST_Node *Number() {
  return NewNodeWithToken(UNTYPED, NULL, NULL, NULL, Parser.current);
}

static AST_Node *Type() {
  Token remember_token = Parser.current;

  Consume(IDENTIFIER, "Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(remember_token.type),
          TokenTypeTranslation(Parser.next.type));

  return NewNodeWithToken(UNTYPED, Identifier(), NULL, NULL, remember_token);
}

static AST_Node *Identifier() {
  Token remember_token = Parser.current;
  AST_Node *parse_result = NULL;

  if (Match(EQUALS)) {
    parse_result = Expression();
  } else if (NextTokenIs(SEMICOLON)) {
    // TODO: Variable declaration
  } else {
    ERROR_AND_EXIT_FMTMSG("Expected '=' or ';' after identifier '%.*s', got '%s' instead",
                          Parser.current.length,
                          Parser.current.position_in_source,
                          TokenTypeTranslation(Parser.next.type));
  }

  return NewNodeWithToken(UNTYPED, parse_result, NULL, NULL, remember_token);
}

static AST_Node *Unary() {
  Token remember_token = Parser.current;
  AST_Node *parse_result = Parse(UNARY);

  switch(remember_token.type) {
    case MINUS:
      return NewNodeWithToken(UNTYPED, parse_result, NULL, NULL, remember_token);
    default:
      printf("Unknown Unary operator '%s'\n",
          TokenTypeTranslation(remember_token.type));
      return NULL;
  }
}

static AST_Node *Binary() {
  Token remember_token = Parser.current;

  Precedence precedence = Rules[Parser.current.type].precedence;
  AST_Node *parse_result = Parse(precedence + 1);

  switch(remember_token.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
      return NewNodeWithToken(UNTYPED, NULL, NULL, parse_result, remember_token);
    default:
      printf("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
      return NULL;
  }
}

static AST_Node *Block() {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY);
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->nodes[LEFT] = Statement();
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY);

    current = &(*current)->nodes[RIGHT];
  }

  Consume(RCURLY, "Expected '}' after Block, got '%s' instead.", TokenTypeTranslation(Parser.next.type));

  return n;
}

static AST_Node *Expression() {
  return Parse((Precedence)1);
}

static AST_Node *Statement() {
  if (Match(IF)) return IfStmt();

  AST_Node *expr_result = Expression();
  Consume(SEMICOLON, "A ';' is expected after an expression statement, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  return expr_result;
}

static AST_Node *IfStmt() {
  Consume(LPAREN, "Expected '(' after IF token, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));
  AST_Node *condition = Expression();
  Consume(RPAREN, "Expected ')' after IF condition, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  Consume(LCURLY, "Expected '{', got '%s' instead", TokenTypeTranslation(Parser.next.type));
  AST_Node *body_if_true = Block();
  AST_Node *body_if_false = NULL;

  if (Match(ELSE)) {
    if (Match(IF))  {
      body_if_false = IfStmt();
    } else {
      Consume(LCURLY, "Expected block starting with '{' after ELSE, got '%s' instead", TokenTypeTranslation(Parser.next.type));
      body_if_false = Block();
    }
  }

  return NewNode(IF_NODE, condition, body_if_true, body_if_false);
}

static AST_Node *Parens() {
  AST_Node *parse_result = Expression();
  Consume(RPAREN, "Missing ')' after expression");

  return parse_result;
}

static AST_Node *BuildAST() {
  AST_Node *root = NewNodeWithArity(START_NODE, NULL, NULL, NULL, BINARY_ARITY);

  AST_Node **current_node = &root;

  while (!Match(TOKEN_EOF)) {
    AST_Node *parse_result = Statement();
    if (parse_result == NULL) {
      ERROR_AND_EXIT("AST could not be created");
    }

    AST_Node *next_statement = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY);

    (*current_node)->nodes[LEFT] = parse_result;
    (*current_node)->nodes[RIGHT] = next_statement;

    current_node = &(*current_node)->nodes[RIGHT];
  }

  return root;
}

void Compile(const char *source) {
  InitLexer(source);
  InitParser();

  AST_Node *ast = BuildAST();

  printf("\n[AST]\n");
  PrintAST(ast);
}
