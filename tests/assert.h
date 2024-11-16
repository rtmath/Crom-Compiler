#ifndef ASSERT_H
#define ASSERT_H

#include <stdbool.h>

#include "common.h"
#include "../src/value.h"

typedef struct {
  int tests_passed;
  int tests_failed;
} TestResults;

#define Assert(predicate) ASSERT(predicate, __FILE__, __func__)
bool ASSERT(bool predicate, const char *file_name, const char *func_name);

#define AssertEqual(v1, v2) ASSERT_EQUAL(v1, v2, __FILE__, __func__)
bool ASSERT_EQUAL(Value v1, Value v2, const char *file_name, const char *func_name);

#define PrintAssertionResults(test_group_name) PRINT_ASSERTION_RESULTS(test_group_name, __FILE__)
void PRINT_ASSERTION_RESULTS(const char *test_group_name, const char *file_name);

void PrintResults(TestResults t, const char *test_group_name);

#endif
