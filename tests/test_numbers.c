#include <float.h> // for FLT_MAX, DBL_MAX

#include "assert.h"
#include "test_numbers.h"

/* ==== Signed ==== */

/* === Unsigned === */

/* ====  Hex   ==== */

/* ==== Binary ==== */
static void Test_U8BinaryLimits() {
  COMPILE("u8 check = b'111111110';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_U16BinaryLimits() {
  COMPILE("u16 check = b'11111111000000001';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_U32BinaryLimits() {
  COMPILE("u32 check = b'111111110000000011111111000000001';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_U64BinaryLimits() {
  COMPILE("u64 check = b'11111111000000001111111100000000111111110000000011111111000000001';")

  // The Lexer creates an Error token when the binary literal exceeds a
  // length of 64
  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_LargeZeroBinaryLiteralInSmallType() {
  COMPILE("u8 check = b'0000000000000000';")

  AssertNoError();
}

// TODO: Verify binary literals cannot be assigned to Int or Float

/* ==== Floats ==== */
static void Test_PositiveFloatLiteral() {
  COMPILE("f32 check = 3.14;")

  AssertNoError();
  AssertEqual(NewFloatValue(3.14));
}

static void Test_NegativeFloatLiteral() {
  COMPILE("f32 check = -75.00;")

  AssertNoError();
  AssertEqual(NewFloatValue(-75.00));
}

static void Test_UnexpectedLeadingDecimalFloatLiteral() {
  COMPILE(".12345;")

  AssertExpectError(ERR_UNEXPECTED);
}

static void Test_CanStorePositiveFltMax() {
  COMPILE("f32 check = 340282346638528859811704183484516925440.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(FLT_MAX));
}

static void Test_CanStoreNegativeFltMax() {
  COMPILE("f32 check = -340282346638528859811704183484516925440.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(-FLT_MAX));
}

static void Test_CanStorePositiveDblMax() {
  COMPILE("f64 check = 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(DBL_MAX));
}

static void Test_CanStoreNegativeDblMax() {
  COMPILE("f64 check = -179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(-DBL_MAX));
}

static void Test_CannotAssignBigLiteralToF32() {
  COMPILE("f32 check = 440282346638528859811704183484516925440.000000;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

void RunAllNumberTests() {
  /* ---- Signed ---- */
  /* --- Unsigned --- */
  /* ----  Hex   ---- */
  /* ---- Binary ---- */
  Test_U8BinaryLimits();
  Test_U16BinaryLimits();
  Test_U32BinaryLimits();
  Test_U64BinaryLimits();
  Test_LargeZeroBinaryLiteralInSmallType();

  /* ---- Floats ---- */
  Test_PositiveFloatLiteral();
  Test_NegativeFloatLiteral();
  Test_UnexpectedLeadingDecimalFloatLiteral();
  Test_CanStorePositiveFltMax();
  Test_CanStoreNegativeFltMax();
  Test_CanStorePositiveDblMax();
  Test_CanStoreNegativeDblMax();
  Test_CannotAssignBigLiteralToF32();

  PrintAssertionResults("Numbers");
}
