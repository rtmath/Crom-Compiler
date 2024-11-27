#include "assert.h"
#include "test_structs.h"

static void Test_StructDeclaration_OptionalSemicolon_OK() {
  COMPILE("struct Check {"
          "  i64 i;      "
          "}             ")

  AssertNoError();
}

static void Test_StructDeclaration_MemberAssignment_OK() {
  COMPILE("struct Check {"
          "  f32 f = 4.5;"
          "}             ")

  AssertNoError();
}

static void Test_StructDeclaration_MemberAssignment_AssignedValue_OK() {
  COMPILE("struct Test {         "
          "  f32 f = 4.5;        "
          "}                     "
          "                      "
          "f32 check = Test.f; ")

  AssertNoError();
  AssertEqual(NewFloatValue(4.5));
}

static void Test_StructDeclaration_MemberAccessibleAsIdentifier_NotAllowed() {
  COMPILE("struct Struct {"
          "  f32 f = 4.5; "
          "}              "
          "               "
          "f32 check = f; ")

  AssertExpectError(ERR_UNDECLARED);
}

/* Causes an infinite loop. Oh boy..
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
  Test_StructDeclaration_MemberAssignment_OK();
  Test_StructDeclaration_MemberAssignment_AssignedValue_OK();
  Test_StructDeclaration_MemberAccessibleAsIdentifier_NotAllowed();
  //Test_StructDeclaration_RandomExpression_NotAllowed();
  Test_StructDeclaration_DanglingPeriod_NotAllowed();

  PrintAssertionResults("Structs");
}
