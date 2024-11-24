#include "assert.h"
#include "test_enums.h"

static void Test_Enum_EmptyDeclaration_NotAllowed() {
  COMPILE("enum Weekdays { };")

  AssertExpectError(ERR_EMPTY_BODY);
}

static void Test_EnumDeclaration_NoIdentifierAssignments_OK() {
  COMPILE("enum Weekdays {"
          "  Sunday,      "
          "  Monday,      "
          "  Tuesday,     "
          "  Wednesday,   "
          "  Thursday,    "
          "  Friday,      "
          "  Saturday,    "
          "};             ")

  AssertNoError();
}

static void Test_EnumDeclaration_IdentifierAssignments_OK() {
  COMPILE("enum Weekdays {"
          "  Sunday = 0,  "
          "  Monday,      "
          "  Tuesday,     "
          "  Wednesday,   "
          "  Thursday,    "
          "  Friday,      "
          "  Saturday,    "
          "};             ")

  AssertNoError();
}

static void Test_EnumDeclaration_NegativeIntAssignment_OK() {
  COMPILE("enum Weekdays {"
          "  Sunday = 0,  "
          "  Monday = -1, "
          "  Tuesday,     "
          "  Wednesday,   "
          "  Thursday,    "
          "  Friday = -3, "
          "  Saturday,    "
          "};             ")

  AssertNoError();
}

static void Test_EnumDeclaration_FloatAssignment_NotAllowed() {
  COMPILE("enum Weekdays { "
          "  Sunday = 3.0, "
          "  Monday,       "
          "  Tuesday,      "
          "  Wednesday,    "
          "  Thursday,     "
          "  Friday = 1.2, "
          "  Saturday,     "
          "};              ")

  AssertExpectError(ERR_IMPROPER_ASSIGNMENT);
}

static void Test_EnumDeclaration_BoolAssignment_NotAllowed() {
  COMPILE("enum Weekdays {   "
          "  Sunday,         "
          "  Monday,         "
          "  Tuesday = false,"
          "  Wednesday,      "
          "  Thursday,       "
          "  Friday,         "
          "  Saturday,       "
          "};                ")

  AssertExpectError(ERR_IMPROPER_ASSIGNMENT);
}

static void Test_EnumDeclaration_CharAssignment_NotAllowed() {
  COMPILE("enum Weekdays {   "
          "  Sunday,         "
          "  Monday = 'c',   "
          "  Tuesday,        "
          "  Wednesday,      "
          "  Thursday,       "
          "  Friday,         "
          "  Saturday,       "
          "};                ")

  AssertExpectError(ERR_IMPROPER_ASSIGNMENT);
}

static void Test_EnumDeclaration_StringAssignment_NotAllowed() {
  COMPILE("enum Weekdays {       "
          "  Sunday,             "
          "  Monday = \"Monday\","
          "  Tuesday,            "
          "  Wednesday,          "
          "  Thursday,           "
          "  Friday,             "
          "  Saturday,           "
          "};                    ")

  AssertExpectError(ERR_IMPROPER_ASSIGNMENT);
}

static void Test_EnumDeclaration_IdentifierAssignment_NotAllowed() {
  COMPILE("i64 x = 5;     "
          "enum Weekdays {"
          "  Sunday,      "
          "  Monday = x,  "
          "  Tuesday,     "
          "  Wednesday,   "
          "  Thursday,    "
          "  Friday,      "
          "  Saturday,    "
          "};             ")

  AssertExpectError(ERR_IMPROPER_ASSIGNMENT);
}

static void Test_EnumDeclaration_IdentifiersWithSameValue_OK() {
  COMPILE("enum Weekdays { "
          "  Sunday = 1,   "
          "  Monday = 1,   "
          "  Tuesday = 1,  "
          "  Wednesday = 1,"
          "  Thursday = 1, "
          "  Friday = 1,   "
          "  Saturday = 1, "
          "};              ")

  AssertNoError();
}

static void Test_EnumDeclaration_SameEnumNames_NotAllowed() {
  COMPILE("enum Check {"
          "  Foo,      "
          "};          "
          "            "
          "enum Check {"
          "  Bar,      "
          "};          ")

  AssertExpectError(ERR_REDECLARED);
}

static void Test_EnumDeclaration_IdenticalEnumIdentifiers_InSameEnum_NotAllowed() {
  COMPILE("enum Foo {"
          "  Check,  "
          "  Check,  "
          "};        ")

  AssertExpectError(ERR_REDECLARED);
}

static void Test_EnumDeclaration_IdenticalEnumIdentifiers_InSeparateEnum_NotAllowed() {
  COMPILE("enum Foo {"
          "  Check,  "
          "};        "
          "          "
          "enum Bar {"
          "  Check,  "
          "};        ")

  AssertExpectError(ERR_REDECLARED);
}

static void Test_EnumDeclaration_AssignmentToOtherEnum_NotAllowed() {
  COMPILE("enum Check { "
          "  Foo,       "
          "};           "
          "             "
          "enum Check2 {"
          "  Bar = Foo  "
          "};           ")

  AssertExpectError(ERR_IMPROPER_ASSIGNMENT);
}

static void Test_Enum_IncrementalImplicitValues() {
  COMPILE("enum Numbers {"
          "  Zero,       "
          "  One,        "
          "  Two,        "
          "};            "
          "              "
          "i8 i = Two;   "
  )

  AssertNoError();
  AssertEqual(NewIntValue(2));
}

// Test that Enum name cannot be used in assignment
//
// Test implicit incremental values when there is a random assignment in
// the middle

void RunAllEnumTests() {
  Test_Enum_EmptyDeclaration_NotAllowed();
  Test_EnumDeclaration_NoIdentifierAssignments_OK();
  Test_EnumDeclaration_IdentifierAssignments_OK();
  Test_EnumDeclaration_FloatAssignment_NotAllowed();
  Test_EnumDeclaration_BoolAssignment_NotAllowed();
  Test_EnumDeclaration_CharAssignment_NotAllowed();
  Test_EnumDeclaration_StringAssignment_NotAllowed();
  Test_EnumDeclaration_IdentifierAssignment_NotAllowed();
  Test_EnumDeclaration_NegativeIntAssignment_OK();
  Test_EnumDeclaration_IdentifiersWithSameValue_OK();
  Test_EnumDeclaration_SameEnumNames_NotAllowed();
  Test_EnumDeclaration_IdenticalEnumIdentifiers_InSameEnum_NotAllowed();
  Test_EnumDeclaration_IdenticalEnumIdentifiers_InSeparateEnum_NotAllowed();
  Test_EnumDeclaration_AssignmentToOtherEnum_NotAllowed();
  Test_Enum_IncrementalImplicitValues();

  PrintAssertionResults("Enum");
}
