#include "compiler.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"
#include "type_checker.h"

AST_Node *Compile(const char *filename, const char *source, SymbolTable *st) {
  InitLexer(filename, source);
  InitParser(st);
  DebugRegisterSymbolTable(st);
  AST_Node *ast = ParserBuildAST();

  CheckTypes(ast, st);

  return ast;
}
