#include <stddef.h> // for NULL

#include "ast.h"
#include "compiler.h"
#include "interpreter.h"
#include "io.h"
#include "symbol_table.h"

int main(void) {
  const char *filename = "test.txt";
  char *contents = NULL;
  ReadFile(filename, &contents);

  SymbolTable *st = NewSymbolTable();
  AST_Node *compiled_code = Compile(filename, contents, &st);

  Interpret(compiled_code, st);
  PrintAST(compiled_code);

  return 0;
}
