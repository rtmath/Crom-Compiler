#include "assert.h"
#include "test_numbers.h"

static void Test_PositiveFloatLiteral() {
  const char *source = "f32 check = 3.14;";

  AST_Node *ast = Compile(__func__, source);
  Interpret(ast);

  Assert(ast->error_code == OK);
  AssertEqual(ast->value, NewFloatValue(3.14));
}

static void Test_NegativeFloatLiteral() {
  const char *source = "f32 check = -75.00;";

  AST_Node *ast = Compile(__func__, source);
  Interpret(ast);

  Assert(ast->error_code == OK);
  AssertEqual(ast->value, NewFloatValue(-75.00));
}

static void Test_UnexpectedLeadingDecimalFloatLiteral() {
  const char *source = ".12345;";

  AST_Node *ast = Compile(__func__, source);

  Assert(ast->error_code == ERR_UNEXPECTED);
}

void RunAllNumberTests() {
  Test_PositiveFloatLiteral();
  Test_NegativeFloatLiteral();
  Test_UnexpectedLeadingDecimalFloatLiteral();

  PrintAssertionResults("Numbers");
}
