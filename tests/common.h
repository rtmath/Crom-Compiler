#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#include "../compiler.h"
#include "../interpreter.h"
#include "../parser.h"
#include "../token.h"
#include "../type_checker.h"
#include "../value.h"

#define MAX_ERROR_MESSAGES 20

typedef struct {
  int tests_passed;
  int tests_failed;
} TestResult;

#define Assert(predicate, msg) ASSERT(predicate, msg, __FILE__, __LINE__)
bool ASSERT(bool predicate, const char *msg, const char *file, int line);

void PrintErrors();

#endif
