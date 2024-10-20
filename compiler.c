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

static ParserAnnotation NO_ANNOTATION = {
  .ostensible_type = OST_UNKNOWN,
  .bit_width = 0,
  .is_signed = 0
};

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

typedef AST_Node* (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static AST_Node *Type();
static AST_Node *Identifier(ParserAnnotation type);
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
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NO_ANNOTATION);
}

static AST_Node *Number() {
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NO_ANNOTATION);
}

static AST_Node *Type() {
  Token remember_token = Parser.current;

  Consume(IDENTIFIER, "Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(remember_token.type),
          TokenTypeTranslation(Parser.next.type));

  return Identifier(AnnotateType(remember_token.type));
}

static AST_Node *Identifier(ParserAnnotation type) {
  Token remember_token = Parser.current;
  bool identifier_exists = IsInSymbolTable(SymbolTable, remember_token);
  AST_Node *parse_result = NULL;

  if (Match(EQUALS)) {
    /*
    if (!identifier_exists) {
      ERROR_AND_EXIT_FMTMSG("Cannot assign to undeclared identifier '%.*s'",
                            remember_token.length,
                            remember_token.position_in_source);
    }
    */

    parse_result = Expression();
  } else if (NextTokenIs(SEMICOLON)) {
    if (identifier_exists) {
      Token already_declared = RetrieveFromSymbolTable(SymbolTable, remember_token);
      ERROR_AND_EXIT_FMTMSG("Identifier '%.*s' has been redeclared. First declared on line %d\n",
                            remember_token.length,
                            remember_token.position_in_source,
                            already_declared.on_line);
    }

    // TODO: Variable declaration
  } else if (identifier_exists) {
    Token t = ResolveIdentifierAsValue(SymbolTable, remember_token);
    return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, t, AnnotateType(t.type));
  } else {
    ERROR_AND_EXIT_FMTMSG("Undeclared identifier '%.*s'",
                          remember_token.length,
                          remember_token.position_in_source);
  }

  // TODO: This is kind of a variable declaration,
  // but variable declaration should happen up above
  AddToSymbolTable(SymbolTable, remember_token);
  return NewNodeWithToken(IDENTIFIER_NODE, parse_result, NULL, NULL, remember_token, type);
}

static AST_Node *Unary() {
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

static AST_Node *Binary() {
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

static AST_Node *Block() {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->nodes[LEFT] = Statement();
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);

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

  return NewNode(IF_NODE, condition, body_if_true, body_if_false, NO_ANNOTATION);
}

static AST_Node *Parens() {
  AST_Node *parse_result = Expression();
  Consume(RPAREN, "Missing ')' after expression");

  return parse_result;
}

static AST_Node *BuildAST() {
  AST_Node *root = NewNodeWithArity(START_NODE, NULL, NULL, NULL, BINARY_ARITY, NO_ANNOTATION);

  AST_Node **current_node = &root;

  while (!Match(TOKEN_EOF)) {
    AST_Node *parse_result = Statement();
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
