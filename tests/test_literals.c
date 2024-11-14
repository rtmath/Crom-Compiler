#include "assert.h"
#include "test_literals.h"

static void LeadingPeriodLiteral() {
  const char *source = "f32 x = .123";

  AST_Node *compilation = Compile("LiteralTests", source);
  Interpret(compilation);

  Assert(compilation->exit_code != OK, "Leading Period Literal should not compile.");
}

void RunAllLiteralTests() {
  LeadingPeriodLiteral();

  PrintAssertionResults("Literals");
}
