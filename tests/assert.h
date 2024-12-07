#ifndef ASSERT_H
#define ASSERT_H

typedef struct {
  int succeeded;
  int failed;
} TestResults;

void Assert(int expected_code, int actual_code, char *file_name, char *group_name);
void PrintAssertionResults(char *group_name);
void PrintResults(TestResults t, const char *test_group_name);

#endif
