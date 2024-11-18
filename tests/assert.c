#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "hashtable.h"

#define MAX_ERROR_MESSAGES 20
#define MAX_ERROR_MSG_SIZE 200

HashTable *ht;
const char *error_msgs[MAX_ERROR_MESSAGES];
int emi = 0;

bool ASSERT(bool predicate, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();

  char *str = malloc(sizeof(char) * MAX_ERROR_MSG_SIZE);

  if (!predicate && emi < MAX_ERROR_MESSAGES) {
    snprintf(str, MAX_ERROR_MSG_SIZE, "      %s() assertion failed", func_name);
    error_msgs[emi++] = str;
  }

  TestResults tr = GetResults(ht, file_name);
  if (predicate) {
    tr.succeeded++;
  } else {
    tr.failed++;
  }
  SetResults(ht, file_name, tr);

  return predicate;
}

bool ASSERT_EQUAL(Value v1, Value v2, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();

  char *str = malloc(sizeof(char) * MAX_ERROR_MSG_SIZE);
  bool predicate = false;

  switch(v1.type) {
    case V_INT: {
      predicate = v1.as.integer == v2.as.integer;
      if (!predicate && emi < MAX_ERROR_MESSAGES) {
        snprintf(str, MAX_ERROR_MSG_SIZE, "    %s() assertion failed, %ld != %ld",
          func_name, v1.as.integer, v2.as.integer);
        error_msgs[emi++] = str;
      }
    } break;
    case V_UINT: {
      predicate = v1.as.uinteger == v2.as.uinteger;
      if (!predicate && emi < MAX_ERROR_MESSAGES) {
        snprintf(str, MAX_ERROR_MSG_SIZE, "      %s() assertion failed, %lu != %lu",
          func_name, v1.as.uinteger, v2.as.uinteger);
        error_msgs[emi++] = str;
      }
    } break;
    case V_FLOAT: {
      // I know floating point equality is perilous. This is intended to
      // check that a literal value from a Crom source file passes through
      // the compilation pipeline and comes out the other end without
      // modification; it is NOT intended to verify the results of floating
      // point arithmetic, for example
      predicate = (v1.as.floating == v2.as.floating);
      Value v3 = NewFloatValue(v1.as.floating - v2.as.floating);
      if (!predicate && emi < MAX_ERROR_MESSAGES) {
        snprintf(str, MAX_ERROR_MSG_SIZE, "      %s() assertion failed, %f != %f",
          func_name, v1.as.floating, v2.as.floating);
        error_msgs[emi++] = str;
      }
    } break;
    default: ERROR_AND_EXIT_FMTMSG("Value type %d not implemented yet\n", v1.type);
  }

  TestResults tr = GetResults(ht, file_name);
  if (predicate) {
    tr.succeeded++;
  } else {
    tr.failed++;
  }
  SetResults(ht, file_name, tr);

  return predicate;
}

bool ASSERT_EXPECT_ERROR(AST_Node *root, ErrorCode code, const char *file_name, const char *func_name) {
  if (ht == NULL) ht = NewHashTable();

  bool predicate = root->error_code == code;

  if (!predicate && emi < MAX_ERROR_MESSAGES) {
    char *str = malloc(sizeof(char) * MAX_ERROR_MSG_SIZE);
    snprintf(str, MAX_ERROR_MSG_SIZE, "      %s() Expected '%s', got '%s'",
      func_name, ErrorCodeTranslation(code), ErrorCodeTranslation(root->error_code));
    error_msgs[emi++] = str;
  }

  TestResults tr = GetResults(ht, file_name);
  if (predicate) {
    tr.succeeded++;
  } else {
    tr.failed++;
  }
  SetResults(ht, file_name, tr);

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
