#include "assert.h"
#include "test_terse_assignments.h"

static void Test_PlusEquals_OK() {
  COMPILE("i64 x = 10;"
          "x += 5;    ")

  AssertNoError();
  AssertEqual(NewIntValue(15));
}

static void Test_MinusEquals_OK() {
  COMPILE("i64 x = 10;"
          "x -= 5;    ")

  AssertNoError();
  AssertEqual(NewIntValue(5));
}

static void Test_TimesEquals_OK() {
  COMPILE("i64 x = 10;"
          "x *= 5;    ")

  AssertNoError();
  AssertEqual(NewIntValue(50));
}

static void Test_DivideEquals_OK() {
  COMPILE("i64 x = 10;"
          "x /= 5;    ")

  AssertNoError();
  AssertEqual(NewIntValue(2));
}

static void Test_ModuloEquals_OK() {
  COMPILE("i64 x = 10;"
          "x %= 5;    ")

  AssertNoError();
  AssertEqual(NewIntValue(0));
}

static void Test_OREquals_OK() {
  COMPILE("u8 x = b'11110000';"
          "x   |= b'00001111';")

  AssertNoError();
  AssertEqual(NewUintValue(0xFF));
}

static void Test_ANDEquals_OK() {
  COMPILE("u8 x = b'10101010';             "
          "x   &= b'00101000';             "
          "bool check = (x == b'00101000');")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_XOREquals_OK() {
  COMPILE("u8 x = b'10101010';"
          "x   ^= b'01010101';")

  AssertNoError();
  AssertEqual(NewUintValue(0xFF));
}

/* >>=
   <<= */

// Test invalid types
// Test inapproprate places to put a terse assignment, e.g. in an expression

void RunAllTerseAssignmentTests() {
  Test_PlusEquals_OK();
  Test_MinusEquals_OK();
  Test_TimesEquals_OK();
  Test_DivideEquals_OK();
  Test_ModuloEquals_OK();

  Test_OREquals_OK();
  Test_ANDEquals_OK();
  Test_XOREquals_OK();

  PrintAssertionResults("Terse Assignments");
}
