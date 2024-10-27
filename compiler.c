#include <stdio.h> // for printf

#include "compiler.h"
#include "lexer.h"
#include "parser.h"

void Compile(const char *source) {
  InitLexer(source);
  InitParser();
  AST_Node *ast = ParserBuildAST();

  printf("\n[AST]\n\n");
  PrintAST(ast);
}
