#include <stddef.h> // for NULL

#include "ast.h"
#include "compiler.h"
#include "io.h"
#include "symbol_table.h"

int main(int argc, char **argv) {
  char *filename = "test.txt";

  if (argc == 2) {
    filename = argv[1];
  }

  char *contents = NULL;
  ReadFile(filename, &contents);

  SymbolTable *st = NewSymbolTable();
  AST_Node *compiled_code = Compile(filename, contents, st);

  DebugReportErrorCode();
  return 0;
}
