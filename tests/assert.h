#ifndef ASSERT_H
#define ASSERT_H

#include <stdbool.h>

typedef struct {
  int succeeded;
  int failed;
} TestResults;

void Assert(int expected_code, int actual_code, char *file_name, char *group_name);
void AssertPrintResult(bool strings_match, char *test_stdout, char *expected_stdout, char *file_name, char *group_name);
void PrintAssertionResults(char *group_name);
void PrintResults(TestResults t, const char *test_group_name);
void PrintResultTotals();

#endif
