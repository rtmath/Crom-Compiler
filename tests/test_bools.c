#include "assert.h"
#include "test_bools.h"

/* ======= Literals  ======= */
static void Test_Bool_TrueLiteral_OK() {
  COMPILE("bool check = true;")

  AssertNoError();
}

static void Test_Bool_FalseLiteral_OK() {
  COMPILE("bool check = false;")

  AssertNoError();
}

static void Test_Bool_NumberAssignment_NotAllowed() {
  COMPILE("bool check = 2;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

/* === Logical Operators === */
static void Test_Bool_NOTFalse_True() {
  COMPILE("bool check = !false;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_NOTTrue_False() {
  COMPILE("bool check = !true;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

static void Test_Bool_AND_FalseAndFalse_False() {
  COMPILE("bool check = false && false;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

static void Test_Bool_AND_TrueAndFalse_False() {
  COMPILE("bool check = true && false;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

static void Test_Bool_AND_FalseAndTrue_False() {
  COMPILE("bool check = false && true;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

static void Test_Bool_AND_TrueAndTrue_True() {
  COMPILE("bool check = true && true;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_OR_FalseAndFalse_False() {
  COMPILE("bool check = false || false;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

static void Test_Bool_OR_TrueAndFalse_True() {
  COMPILE("bool check = true || false;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_OR_FalseAndTrue_True() {
  COMPILE("bool check = false || true;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_OR_TrueAndTrue_True() {
  COMPILE("bool check = true || true;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_ComplexExpression_OK() {
  COMPILE("bool check = (true &&"
          "              (false || true) &&"
          "              !false);"
  )

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_LessThan_PositiveInts_OK() {
  COMPILE("bool check = 1 < 2;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_LessThan_NegativeInts_OK() {
  COMPILE("bool check = -2 < -1;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_LessThan_MixedSigns_OK() {
  COMPILE("bool check = -1 < 1;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_LessThan_False_OK() {
  COMPILE("bool check = 5 < 3;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

static void Test_Bool_GreaterThan_PositiveInts_OK() {
  COMPILE("bool check = 2 > 1;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_GreaterThan_NegativeInts_OK() {
  COMPILE("bool check = -1 > -2;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_GreaterThan_MixedSigns_OK() {
  COMPILE("bool check = 1 > -1;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_Bool_GreaterThan_False_OK() {
  COMPILE("bool check = 3 > 5;")

  AssertNoError();
  AssertEqual(NewBoolValue(false));
}

// TODO: Test unconvertible type comparisons

void RunAllBoolTests() {
  /* ------- Literals  ------- */
  Test_Bool_TrueLiteral_OK();
  Test_Bool_FalseLiteral_OK();
  Test_Bool_NumberAssignment_NotAllowed();

  /* --- Logical Operators --- */
  Test_Bool_NOTFalse_True();
  Test_Bool_NOTTrue_False();

  Test_Bool_AND_FalseAndFalse_False();
  Test_Bool_AND_TrueAndFalse_False();
  Test_Bool_AND_FalseAndTrue_False();
  Test_Bool_AND_TrueAndTrue_True();

  Test_Bool_OR_FalseAndFalse_False();
  Test_Bool_OR_TrueAndFalse_True();
  Test_Bool_OR_FalseAndTrue_True();
  Test_Bool_OR_TrueAndTrue_True();

  Test_Bool_ComplexExpression_OK();

  Test_Bool_LessThan_PositiveInts_OK();
  Test_Bool_LessThan_NegativeInts_OK();
  Test_Bool_LessThan_MixedSigns_OK();
  Test_Bool_LessThan_False_OK();

  Test_Bool_GreaterThan_PositiveInts_OK();
  Test_Bool_GreaterThan_NegativeInts_OK();
  Test_Bool_GreaterThan_MixedSigns_OK();
  Test_Bool_GreaterThan_False_OK();

  PrintAssertionResults("Bools");
}
