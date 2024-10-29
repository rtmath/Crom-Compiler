#include <stdio.h> // for printf

#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "type_checker.h"

void Compile(const char *filename, const char *source) {
  InitLexer(filename, source);
  InitParser();
  AST_Node *ast = ParserBuildAST();

  CheckTypes(ast);

  printf("\n[AST]\n\n");
  PrintAST(ast);
}
