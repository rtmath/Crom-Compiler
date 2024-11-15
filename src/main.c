#include <stddef.h> // for NULL

#include "ast.h"
#include "compiler.h"
#include "interpreter.h"
#include "io.h"

int main(void) {
  const char *filename = "test.txt";
  char *contents = NULL;
  ReadFile(filename, &contents);

  AST_Node *compiled_code = Compile(filename, contents);

  Interpret(compiled_code);
  PrintAST(compiled_code);

  return 0;
}
