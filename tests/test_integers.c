#include "test_integers.h"

static int tests_passed = 0;
static int tests_failed = 0;

void RecordResult(bool predicate) {
  if (predicate) {
    tests_passed++;
  } else {
    tests_failed++;
  }
}

TestResult TestIntegers() {
  const char *source = "i8 i = 10;";

  AST_Node *compilation = Compile("TestIntegers()", source);

  Interpret(compilation);
  RecordResult(
    Assert(compilation->exit_code == OK, "Check Exit Code is OK")
  );

  return (TestResult){
    .tests_passed = tests_passed,
    .tests_failed = tests_failed,
  };
}
