#include <stdio.h>

#include "common.h"
#include "test_integers.h"

void PrintResults(TestResult t, const char *test_group_name) {
  int total_tests = t.tests_passed + t.tests_failed;
  bool error_occurred = t.tests_passed < total_tests;

  const char *coloration =
    (error_occurred)
    ? "\x1b[31m"
    : "\x1b[34m";
  const char *stop_coloration = "\x1b[39m";

  printf(
    "%s: %s%d / %d tests passed.%s\n",
    test_group_name,
    coloration,
    t.tests_passed,
    total_tests,
    stop_coloration);

  if (error_occurred) {
    printf("\nTests that failed (maximum of %d are shown):\n", MAX_ERROR_MESSAGES);
    PrintErrors();
  }
}

void RunTests() {
  PrintResults(TestIntegers(), "Integers");
}

int main() {
  RunTests();
  return 0;
}
