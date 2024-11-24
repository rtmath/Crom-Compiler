#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "hashtable.h"

#define MAX_ERROR_MESSAGES 50
#define MAX_ERROR_MSG_SIZE 200

HashTable *ht;
const char *error_msgs[MAX_ERROR_MESSAGES];
int emi = 0;

static void LogError(bool predicate, const char *msg, ...) {
  if (!predicate && emi < MAX_ERROR_MESSAGES) {
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
  } else {
    tr.failed++;
  }
  SetResults(ht, file_name, tr);
}

bool ASSERT(bool predicate, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();

  LogError(predicate, "      %s() assertion failed", func_name);
  LogResults(predicate, file_name);

  return predicate;
}

bool ASSERT_EQUAL(Value v1, Value v2, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();

  bool predicate = false;
  switch(v1.type) {
    case V_NONE: {
      LogError(false, "    %s(): Value type from AST is NONE\n", func_name);
    } break;
    case V_INT: {
      predicate = v1.as.integer == v2.as.integer;
      LogError(predicate,
               "    %s() assertion failed, %ld != %ld",
               func_name,
               v1.as.integer,
               v2.as.integer);
    } break;
    case V_UINT: {
      predicate = v1.as.uinteger == v2.as.uinteger;
      LogError(predicate,
               "      %s() assertion failed, %lu != %lu",
               func_name,
               v1.as.uinteger,
               v2.as.uinteger);
    } break;
    case V_FLOAT: {
      // I know floating point equality is perilous. This is intended to
      // check that a literal value from a Crom source file passes through
      // the compilation pipeline and comes out the other end without
      // modification; it is NOT intended to verify the results of floating
      // point arithmetic, for example
      predicate = (v1.as.floating == v2.as.floating);
      LogError(predicate,
               "      %s() assertion failed, %f != %f",
               func_name,
               v1.as.floating,
               v2.as.floating);
    } break;
    case V_BOOL: {
      predicate = (v1.as.boolean == v2.as.boolean);
      LogError(predicate,
               "      %s() assertion failed, %s != %s",
               func_name,
               (v1.as.floating) ? "true" : "false",
               (v2.as.floating) ? "true" : "false");
    } break;
    default: {
      LogError(false, "[%s:%s] Assert: Value type %d not implemented yet\n", file_name, func_name, v1.type);
      ERROR_AND_EXIT_FMTMSG("[%s:%s] Assert: Value type %d not implemented yet\n", file_name, func_name, v1.type);
    } break;
  }

  LogResults(predicate, file_name);

  return predicate;
}

bool ASSERT_EXPECT_ERROR(AST_Node *root, ErrorCode code, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();

  bool predicate = root->error_code == code;
  LogError(predicate,
           "      %s() Expected '%s', got '%s'",
           func_name,
           ErrorCodeTranslation(code),
           ErrorCodeTranslation(root->error_code));

  LogResults(predicate, file_name);

  return predicate;
}

void PRINT_ASSERTION_RESULTS(const char *test_group_name, const char *file_name) {
  if (ht == NULL) return;

  TestResults tr = GetResults(ht, file_name);

  if (tr.succeeded == 0 && tr.failed == 0) return;

  PrintResults(tr, test_group_name);
}

void PrintResults(TestResults t, const char *test_group_name) {
  int total = t.succeeded + t.failed;
  bool error_occurred = t.succeeded < total;

  const char *coloration = (error_occurred)
                              ? "\x1b[31m"  // Set color: Red
                              : "\x1b[34m"; // Set color: Blue
  const char *stop_coloration = "\x1b[39m"; // Set color: Default

  printf("%10s: %s%2d / %2d assertions succeeded.%s\n",
         test_group_name,
         coloration,
         t.succeeded,
         total,
         stop_coloration);

  if (error_occurred) {
    for (int i = 0; i < emi; i++) {
      printf("%s\n", error_msgs[i]);
    }
  }
}
