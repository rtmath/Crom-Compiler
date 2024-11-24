#ifndef ASSERT_H
#define ASSERT_H

#include <stdbool.h>

#include "common.h"
#include "../src/value.h"

typedef struct {
  int succeeded;
  int failed;
} TestResults;

#define COMPILE(source)                          \
  SymbolTable *st = NewSymbolTable();            \
  AST_Node *ast = Compile(__func__, source, st); \
  if (ast->error_code == ERR_UNSET) {            \
    Interpret(ast, st);                          \
  }

#define AssertNoError() ASSERT_EXPECT_ERROR(ast, OK, __FILE__, __func__)
#define AssertExpectError(error_code) ASSERT_EXPECT_ERROR(ast, error_code, __FILE__, __func__)
#define AssertEqual(expected_value) ASSERT_EQUAL(ast->value, expected_value, __FILE__, __func__)

#define Assert(predicate) ASSERT(predicate, __FILE__, __func__)

bool ASSERT(bool predicate, const char *file_name, const char *func_name);
bool ASSERT_EQUAL(Value v1, Value v2, const char *file_name, const char *func_name);
bool ASSERT_EXPECT_ERROR(AST_Node *root, ErrorCode code, const char *file_name, const char *func_name);

#define PrintAssertionResults(test_group_name) PRINT_ASSERTION_RESULTS(test_group_name, __FILE__)
void PRINT_ASSERTION_RESULTS(const char *test_group_name, const char *file_name);

void PrintResults(TestResults t, const char *test_group_name);

#endif
