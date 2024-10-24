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

HashTable *SymbolTable;

#define UNUSED false
#define CAN_ASSIGN true

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
  ARRAY_SUBSCRIPTING,
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
static AST_Node *Struct(bool unused);
static AST_Node *StringLiteral(bool unused);
static AST_Node *ArraySubscripting(bool unused);
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
  [STRUCT]         = { Struct,   NULL, NO_PRECEDENCE },

  [IDENTIFIER]     = { Identifier, NULL, NO_PRECEDENCE },

  // Constants
  [BINARY_CONSTANT] = {     Number, NULL, NO_PRECEDENCE },
  [HEX_CONSTANT]    = {     Number, NULL, NO_PRECEDENCE },
  [INT_CONSTANT]    = {     Number, NULL, NO_PRECEDENCE },
  [FLOAT_CONSTANT]  = {     Number, NULL, NO_PRECEDENCE },
  [ENUM_CONSTANT]   = { Identifier, NULL, NO_PRECEDENCE },
  [CHAR_CONSTANT]   = {       Char, NULL, NO_PRECEDENCE },

  [STRING_LITERAL] = { StringLiteral, NULL, NO_PRECEDENCE },

  // Punctuators
  [LPAREN]         = { Parens,              NULL,      NO_PRECEDENCE },
  [LBRACKET]       = {   NULL, ArraySubscripting, ARRAY_SUBSCRIPTING },
  [PLUS]           = {   NULL,            Binary,               TERM },
  [MINUS]          = {  Unary,            Binary,               TERM },
  [ASTERISK]       = {   NULL,            Binary,             FACTOR },
  [DIVIDE]         = {   NULL,            Binary,             FACTOR },

  // Misc
  [TOKEN_EOF]      = { NULL, NULL, PREC_EOF },
};

static ParserAnnotation Annotation(OstensibleType type, int bit_width, bool is_signed) {
  ParserAnnotation a = NoAnnotation();

  a.ostensible_type = type;
  a.bit_width = bit_width;
  a.is_signed = is_signed;

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
    case VOID: return Annotation(OST_VOID, 0, 0);
    case STRING: return Annotation(OST_STRING, 0, 0);
    case STRUCT: return Annotation(OST_STRUCT, 0, 0);

    default:
      ERROR_AND_CONTINUE_FMTMSG("AnnotateType(): Unimplemented ToOstensibleType for TokenType '%s'\n", TokenTypeTranslation(t));
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
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NoAnnotation());
}

static AST_Node *Number(bool) {
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NoAnnotation());
}

static AST_Node *Char(bool) {
  return NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, Parser.current, NoAnnotation());
}

static AST_Node *Struct() {
  Token remember_token = Parser.next;
  Consume(IDENTIFIER, "Struct(): Expected IDENTIFIER after Type '%s, got '%s instead",
          TokenTypeTranslation(Parser.current.type),
          TokenTypeTranslation(Parser.next.type));

  if (IsIn(SymbolTable, remember_token)) {
    HT_Entry existing_struct = RetrieveFrom(SymbolTable, remember_token);
    ERROR_AND_EXIT_FMTMSG("Struct '%.*s' is already in symbol table, declared on line %d\n",
      remember_token.length,
      remember_token.position_in_source,
      existing_struct.annotation.declared_on_line);
  }
  AddTo(SymbolTable, Entry(remember_token, AnnotateType(STRUCT), DECL_AWAITING_INIT));
  HT_Entry symbol = RetrieveFrom(SymbolTable, remember_token);

  HashTable *symbol_table = SymbolTable;
  SymbolTable = symbol.struct_fields;

  Consume(LCURLY, "Struct(): Expected '{' after STRUCT declaration, got '%.*s' instead",
          TokenTypeTranslation(Parser.next.type));

  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &n;

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    (*current)->nodes[LEFT] = Statement(UNUSED);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];
  }

  Consume(RCURLY, "Struct(): Expected '}' after STRUCT block, got '%.*s' instead",
          TokenTypeTranslation(Parser.next.type));

  SymbolTable = symbol_table;

  AddTo(SymbolTable, Entry(remember_token, AnnotateType(STRUCT), DECL_INITIALIZED));
  return NewNodeWithToken(IDENTIFIER_NODE, n, NULL, NULL, remember_token, AnnotateType(STRUCT));
}

static AST_Node *EnumBlock() {
  AST_Node *n = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());
  AST_Node **current = &n;

  Consume(LCURLY, "EnumBlock(): Expected '{' after ENUM declaration, got %.*s", TokenTypeTranslation(Parser.current.type));

  while (!NextTokenIs(RCURLY) && !NextTokenIs(TOKEN_EOF)) {
    HT_Entry symbol = RetrieveFrom(SymbolTable, Parser.current);
    bool is_in_symbol_table = IsIn(SymbolTable, Parser.current);

    if (is_in_symbol_table) {
      ERROR_AND_EXIT_FMTMSG("EnumBlock(): Enum identifier '%.*s' already exists, declared on line %d",
                            Parser.next.length,
                            Parser.next.position_in_source,
                            symbol.annotation.declared_on_line);
    }

    AddTo(SymbolTable, Entry(Parser.next, NoAnnotation(), DECL_NOT_APPLICABLE));
    Consume(IDENTIFIER, "EnumBlock(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
            TokenTypeTranslation(Parser.current.type),
            TokenTypeTranslation(Parser.next.type));

    (*current)->nodes[LEFT] = Identifier(CAN_ASSIGN);
    (*current)->nodes[RIGHT] = NewNodeWithArity(CHAIN_NODE, NULL, NULL, NULL, BINARY_ARITY, NoAnnotation());

    current = &(*current)->nodes[RIGHT];

    Match(COMMA);
  }

  Consume(RCURLY, "EnumBlock(): Expected '}' after ENUM block, got %.*s", TokenTypeTranslation(Parser.current.type));

  return n;
}

static AST_Node *Enum(bool) {
  ParserAnnotation a = AnnotateType(Parser.current.type);
  a.declared_on_line = Parser.next.on_line;
  AddTo(SymbolTable, Entry(Parser.next, a, DECL_AWAITING_INIT));

  Consume(IDENTIFIER, "Enum(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(Parser.next.type),
          TokenTypeTranslation(Parser.next.type));

  AST_Node *enum_name = Identifier(false);
  enum_name->nodes[LEFT] = EnumBlock();

  return enum_name;
}

static AST_Node *ArraySubscripting(bool) {
  AST_Node *return_value = NULL;

  if (Match(IDENTIFIER)) {
    HT_Entry symbol = RetrieveFrom(SymbolTable, Parser.current);
    bool is_in_symbol_table = IsIn(SymbolTable, Parser.current);

    if (!is_in_symbol_table) {
      ERROR_AND_EXIT_FMTMSG("Can't access array with undeclared identifier '%.*s'",
                            Parser.current.length,
                            Parser.current.position_in_source);
    }

    if (symbol.declaration_type != DECL_INITIALIZED) {
      ERROR_AND_EXIT_FMTMSG("Can't access array with uninitialized identifier '%.*s'",
                            Parser.current.length,
                            Parser.current.position_in_source);
    }

    return_value = NewNodeWithToken(TERMINAL_DATA, NULL, NULL, NULL, symbol.token, NoAnnotation());
  } else if (Match(INT_CONSTANT)) {
    return_value = Number(UNUSED);
  }

  Consume(RBRACKET, "ArraySubscripting(): Where's the ']'?");

  return return_value;
}

static AST_Node *Type(bool) {
  Token remember_token = Parser.current; // TODO: I don't think I need this
  bool is_array = false;
  long array_size = 0;

  if (Match(LBRACKET)) {
    if (Match(INT_CONSTANT)) {
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
    if (IsIn(SymbolTable, Parser.next)) {
      HT_Entry e = RetrieveFrom(SymbolTable, Parser.next);
      ERROR_AND_EXIT_FMTMSG("Type(): Re-declaration of identifier '%.*s', previously declared on line %d\n",
                            Parser.next.length,
                            Parser.next.position_in_source,
                            e.annotation.declared_on_line);
    }

    ParserAnnotation a = AnnotateType(remember_token.type);
    a.declared_on_line = Parser.next.on_line;
    a.is_array = is_array;
    a.array_size = array_size;
    AddTo(SymbolTable, Entry(Parser.next, a, DECL_AWAITING_INIT));
  }

  Consume(IDENTIFIER, "Type(): Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(remember_token.type),
          TokenTypeTranslation(Parser.next.type));

  return Identifier(CAN_ASSIGN);
}

static AST_Node *Identifier(bool can_assign) {
  HT_Entry symbol = RetrieveFrom(SymbolTable, Parser.current);
  bool is_in_symbol_table = IsIn(SymbolTable, Parser.current);
  AST_Node *array_index = NULL;
  Token remember_token = Parser.current;

  if (!is_in_symbol_table) {
    ERROR_AND_EXIT_FMTMSG("Identifier(): Line %d: Undeclared identifier '%.*s'",
                          remember_token.on_line,
                          remember_token.length,
                          remember_token.position_in_source);
  }

  if (Match(LBRACKET)) {
    array_index = ArraySubscripting(UNUSED);
  }

  if (Match(EQUALS)) {
    if (!can_assign) {
      ERROR_AND_EXIT_FMTMSG("Identifier(): Cannot assign to identifier '%.*s'",
                            remember_token.length,
                            remember_token.position_in_source);
    }

    AddTo(SymbolTable, Entry(remember_token, symbol.annotation, DECL_INITIALIZED));
    return NewNodeWithToken(IDENTIFIER_NODE, Expression(UNUSED), array_index, NULL, remember_token, symbol.annotation);
  }

  if (NextTokenIs(SEMICOLON)) {
    if (symbol.declaration_type == DECL_NOT_APPLICABLE && can_assign) {
      HT_Entry already_declared = RetrieveFrom(SymbolTable, remember_token);
      ERROR_AND_EXIT_FMTMSG("Identifier(): Identifier '%.*s' has been redeclared. First declared on line %d\n",
                            remember_token.length,
                            remember_token.position_in_source,
                            already_declared.annotation.declared_on_line);
    }

    return NewNodeWithToken(IDENTIFIER_NODE, NULL, array_index, NULL, remember_token, symbol.annotation);
  }

  HT_Entry e = RetrieveFrom(SymbolTable, remember_token);
  return NewNodeWithToken(TERMINAL_DATA, NULL, array_index, NULL, e.token, e.annotation);
}

static AST_Node *Unary(bool) {
  Token remember_token = Parser.current;
  AST_Node *parse_result = Parse(UNARY);

  switch(remember_token.type) {
    case MINUS:
      return NewNodeWithToken(UNTYPED, parse_result, NULL, NULL, remember_token, NoAnnotation());
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
      return NewNodeWithToken(UNTYPED, NULL, NULL, parse_result, remember_token, NoAnnotation());
    default:
      printf("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
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

  AST_Node *expr_result = Expression(UNUSED);
  Consume(SEMICOLON, "Statement(): A ';' is expected after an expression statement, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

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

static AST_Node *Parens(bool) {
  AST_Node *parse_result = Expression(UNUSED);
  Consume(RPAREN, "Parens(): Missing ')' after expression");

  return parse_result;
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
  SymbolTable = NewHashTable();

  AST_Node *ast = BuildAST();

  printf("\n[AST]\n");
  PrintAST(ast);
}
