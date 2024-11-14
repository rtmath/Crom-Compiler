#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "hashtable.h"

#define MAX_ERROR_MESSAGES 20

HashTable *ht;
const char *error_msgs[MAX_ERROR_MESSAGES];
int emi = 0;

bool ASSERT(bool predicate, const char *msg, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();
  TestResults tr = GetResults(ht, file_name);

  char *str = malloc(sizeof(char) * 100);

  if (!predicate) {
    if (emi < MAX_ERROR_MESSAGES) {
      snprintf(str, 99, "%s() Assertion failed: %s", func_name, msg);
      error_msgs[emi++] = str;
    }
  }

  if (predicate) {
    tr.tests_passed++;
  } else {
    tr.tests_failed++;
  }

  SetResults(ht, file_name, tr);

  return predicate;
}

void PRINT_ASSERTION_RESULTS(const char *test_group_name, const char *file_name) {
  TestResults tr = GetResults(ht, file_name);
  PrintResults(tr, test_group_name);
}

void PrintResults(TestResults t, const char *test_group_name) {
  int total_tests = t.tests_passed + t.tests_failed;
  bool error_occurred = t.tests_passed < total_tests;

  const char *coloration = (error_occurred)
                              ? "\x1b[31m"  // Set color: Red
                              : "\x1b[34m"; // Set color: Blue
  const char *stop_coloration = "\x1b[39m"; // Set color: Default

  printf("%10s: %s%3d / %3d tests passed.%s",
         test_group_name,
         coloration,
         t.tests_passed,
         total_tests,
         stop_coloration);

  if (error_occurred) {
    printf("\nTests that failed (maximum of %d are shown):\n", MAX_ERROR_MESSAGES);
    for (int i = 0; i < emi; i++) {
      printf("%s\n", error_msgs[i]);
    }
  }
}
