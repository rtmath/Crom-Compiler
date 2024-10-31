#include <stdio.h> // for printf

#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

static SymbolTable *SYMBOL_TABLE;

void Compile(const char *filename, const char *source) {
  InitLexer(filename, source);
  InitParser(&SYMBOL_TABLE);
  AST_Node *ast = ParserBuildAST();

  CheckTypes(ast, SYMBOL_TABLE);

  PrintAST(ast);
}
