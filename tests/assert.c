#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/error.h"
#include "assert.h"
#include "hashtable.h"

#define MAX_ERROR_MESSAGES 50
#define MAX_ERROR_MSG_SIZE 200
#define MSG_SPACER "               "

HashTable *ht;
char *error_msgs[MAX_ERROR_MESSAGES];
int emi = 0;

static int total_succeeded;
static int total_failed;

static void LogError(const char *msg, ...) {
  if (emi < MAX_ERROR_MESSAGES) {
    char *str = malloc(sizeof(char) * MAX_ERROR_MSG_SIZE);

    va_list args;
    va_start(args, msg);
    vsnprintf(str, MAX_ERROR_MSG_SIZE, msg, args);
    va_end(args);

    error_msgs[emi++] = str;
  }
}

static void LogResults(bool predicate, const char *file_name) {
  TestResults tr = GetResults(ht, file_name);

  if (predicate) {
    tr.succeeded++;
    total_succeeded++;
  } else {
    tr.failed++;
    total_failed++;
  }

  SetResults(ht, file_name, tr);
}

void Assert(int expected_code, int actual_code, char *file_name, char *group_name) {
  if (ht == NULL) ht = NewHashTable();

  bool predicate = expected_code == actual_code;
  if (!predicate) {
    LogError(MSG_SPACER "[%s]\n" MSG_SPACER "    Expected '%s', got '%s'",
             file_name,
             ErrorCodeTranslation(expected_code),
             ErrorCodeTranslation(actual_code));
  }

  LogResults(predicate, group_name);
}

void AssertPrintResult(bool strings_match, char *test_stdout, char *expected_stdout, char *file_name, char *group_name) {
  if (ht == NULL) ht = NewHashTable();

  bool predicate = strings_match;
  if (!predicate) {
    LogError(MSG_SPACER "[%s]\n" MSG_SPACER "    Expected '%s', got '%s'",
             file_name,
             test_stdout,
             expected_stdout);
  }

  LogResults(predicate, group_name);
}

void PrintAssertionResults(char *group_name) {
  if (ht == NULL) return;

  TestResults tr = GetResults(ht, group_name);

  if (tr.succeeded == 0 && tr.failed == 0) return;

  PrintResults(tr, group_name);
}

void PrintResults(TestResults t, const char *test_group_name) {
  int total = t.succeeded + t.failed;
  bool error_occurred = t.succeeded < total;

  const char *coloration = (error_occurred)
                              ? "\x1b[31m"  // Set color: Red
                              : "\x1b[34m"; // Set color: Blue
  const char *stop_coloration = "\x1b[39m"; // Set color: Default

  printf("%17s: %s%3d / %3d assertions succeeded.%s\n",
         test_group_name,
         coloration,
         t.succeeded,
         total,
         stop_coloration);

  if (error_occurred) {
    for (int i = 0; i < emi; i++) {
      printf("%s\n", error_msgs[i]);
      free(error_msgs[i]);
    }

    emi = 0;
  }
}

void PrintResultTotals() {
  bool error_occurred = total_succeeded < total_succeeded + total_failed;

  const char *coloration = (error_occurred)
                              ? "\x1b[31m"  // Set color: Red
                              : "\x1b[34m"; // Set color: Blue
  const char *stop_coloration = "\x1b[39m"; // Set color: Default

  printf("%s%22d / %3d total assertions succeeded.%s\n", coloration, total_succeeded, total_succeeded + total_failed, stop_coloration);
}
