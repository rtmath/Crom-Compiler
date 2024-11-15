#include "assert.h"
#include "test_literals.h"

static void Successful1() {
  const char *source = "i8 a = 1;";

  AST_Node *compilation = Compile("LiteralTests", source);
  Interpret(compilation);

  Assert(compilation->exit_code == OK, "");
}

static void Successful2() {
  const char *source = "i8 b = 2;";

  AST_Node *compilation = Compile("LiteralTests", source);
  Interpret(compilation);

  Assert(compilation->exit_code == OK, "");
}

static void Successful3() {
  const char *source = "i8 c = 3;";

  AST_Node *compilation = Compile("LiteralTests", source);
  Interpret(compilation);

  Assert(compilation->exit_code == OK, "");
}

static void Failure1() {
  const char *source = "i8 d = 4;";

  AST_Node *compilation = Compile("LiteralTests", source);
  Interpret(compilation);

  Assert(compilation->exit_code != OK, "");
}

static void ParsingError() {
  const char *source = "d = 4;";

  AST_Node *compilation = Compile("LiteralTests", source);
  Interpret(compilation);

  Assert(compilation->exit_code != OK, "");
}

void RunAllLiteralTests() {
  Successful1();
  ParsingError();
  Successful2();
  Failure1();
  Successful3();

  PrintAssertionResults("Literals");
}
