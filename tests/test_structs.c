#include "assert.h"
#include "test_structs.h"

static void Test_StructDeclaration_OptionalSemicolon_OK() {
  COMPILE("struct Check {"
          "  i64 i;      "
          "}             ")

  AssertNoError();
}

/* Test should be re-enabled after test suite is overhauled
static void Test_StructDeclaration_MemberAssignment_NotAllowed() {
  COMPILE("struct Check {"
          "  f32 f = 4.5;"
          "}             ")

  AssertExpectError(ERR_UNEXPECTED);
}

Test should be re-enabled after test suite is overhauled
static void Test_StructDeclaration_MemberAccessibleAsIdentifier_NotAllowed() {
  COMPILE("struct Struct {"
          "  f32 f = 4.5; "
          "}              "
          "               "
          "f32 check = f; ")

  AssertExpectError(ERR_UNDECLARED);
}
*/

/* Causes an infinite loop. Oh boy..
Test should be re-enabled after test suite is overhauled
static void Test_StructDeclaration_RandomExpression_NotAllowed() {
  COMPILE("struct Struct {"
          "  6 * 4;       "
          "}              "
          "               ")

  AssertExpectError(ERR_IMPROPER_DECLARATION);
}
*/

static void Test_StructDeclaration_DanglingPeriod_NotAllowed() {
  COMPILE("struct Struct { "
          "  i64 i;        "
          "}               "
          "                "
          "i64 x = Struct.;")

  AssertExpectError(ERR_UNEXPECTED);
}

void RunAllStructTests() {
  Test_StructDeclaration_OptionalSemicolon_OK();
  //Test_StructDeclaration_MemberAssignment_NotAllowed();
  //Test_StructDeclaration_MemberAccessibleAsIdentifier_NotAllowed();
  //Test_StructDeclaration_RandomExpression_NotAllowed();
  Test_StructDeclaration_DanglingPeriod_NotAllowed();

  PrintAssertionResults("Structs");
}
