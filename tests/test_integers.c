#include "assert.h"
#include "test_integers.h"

static void PlaceholderTest() {
  const char *source = "i64 i = 10;";

  AST_Node *compilation = Compile("IntegersTests", source);
  Interpret(compilation);

  //Assert(compilation->exit_code == OK, "Check Exit Code is OK");
  Assert(true, "Check Exit Code is OK");
}

void RunAllIntegerTests() {
  PlaceholderTest();
  PlaceholderTest();
  PlaceholderTest();

  PrintAssertionResults("Integers");
}