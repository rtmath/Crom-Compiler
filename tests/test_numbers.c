#include <float.h> // for FLT_MAX, DBL_MAX

#include "assert.h"
#include "test_numbers.h"

/* ==== Signed ==== */
static void Test_I8Max_Limit_OK() {
  COMPILE("i8 check = 127;")

  AssertNoError();
}

static void Test_I8Max_Overflow() {
  COMPILE("i8 check = 128;") // INT8_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I16Max_Limit_OK() {
  COMPILE("i16 check = 32767;")

  AssertNoError();
}

static void Test_I16Max_Overflow() {
  COMPILE("i16 check = 32768;") // INT16_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I32Max_Limit_OK() {
  COMPILE("i32 check = 2147483647;")

  AssertNoError();
}

static void Test_I32Max_Overflow() {
  COMPILE("i32 check = 2147483648;") // INT32_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I64Max_Limit_OK() {
  COMPILE("i64 check = 9223372036854775807;")

  AssertNoError();
}

static void Test_I64Max_Overflow() {
  COMPILE("i64 check = 9223372036854775808;") // INT64_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I8Min_Limit_OK() {
  COMPILE("i8 check = -128;")

  AssertNoError();
}

static void Test_I8Min_Overflow() {
  COMPILE("i8 check = -129;") // INT8_MIN - 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I16Min_Limit_OK() {
  COMPILE("i16 check = -32768;")

  AssertNoError();
}

static void Test_I16Min_Overflow() {
  COMPILE("i16 check = -32769;") // INT16_MIN - 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I32Min_Limit_OK() {
  COMPILE("i32 check = -2147483648;")

  AssertNoError();
}

static void Test_I32Min_Overflow() {
  COMPILE("i32 check = -2147483649;") // INT16_MIN - 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_I64Min_Limit_OK() {
  COMPILE("i64 check = -9223372036854775808;")

  AssertNoError();
}

static void Test_I64Min_Overflow() {
  COMPILE("i64 check = -9223372036854775809;") // INT16_MIN - 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_DoubleUnaryMinus_IsPositive() {
  COMPILE("i64 check = -(-1);");

  AssertNoError();
  AssertEqual(NewIntValue(1));
}

/* === Unsigned === */
static void Test_U8_Limit_OK() {
  COMPILE("u8 check = 255;")

  AssertNoError();
}

static void Test_U8_Overflow() {
  COMPILE("u8 check = 256;") // UINT8_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_U16_Limit_OK() {
  COMPILE("u16 check = 65535;")

  AssertNoError();
}

static void Test_U16_Overflow() {
  COMPILE("u16 check = 65536;") // UINT16_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_U32_Limit_OK() {
  COMPILE("u32 check = 4294967295;")

  AssertNoError();
}

static void Test_U32_Overflow() {
  COMPILE("u32 check = 4294967296;") // UINT32_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_U64_Limit_OK() {
  COMPILE("u64 check = 18446744073709551615;")
  AssertNoError();
}

static void Test_U64_Overflow() {
  COMPILE("u64 check = 18446744073709551616;") // UINT64_MAX + 1

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Uint_NegativeLiteral_Overflows() {
  COMPILE("u8 check = -1;")

  AssertExpectError(ERR_OVERFLOW);
}

/* ====  Hex   ==== */
static void Test_Hex_U8_Overflow() {
  COMPILE("u8 check = 0x1FF;")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Hex_U16_Overflow() {
  COMPILE("u16 check = 0x1FFFF;")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Hex_U32_Overflow() {
  COMPILE("u32 check = 0x1FFFFFFFF;")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Hex_U64_Overflow() {
  COMPILE("u64 check = 0x1FFFFFFFFFFFFFFFF;")

  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_Hex_NegativeLiteral_NotAllowed() {
  COMPILE("u64 check = -0xBEEFCAFE;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_Hex_IntAssignment_NotAllowed() {
  COMPILE("i32 check = 0xFEFE;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_Hex_FloatAssignment_NotAllowed() {
  COMPILE("i32 check = 0xFEFE;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

/* ==== Binary ==== */
static void Test_Binary_U8_Limit_OK() {
  COMPILE("u8 check = b'11111111';")

  AssertNoError();
}

static void Test_Binary_U8_Overflow() {
  COMPILE("u8 check = b'111111110';")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Binary_U16_Limit_OK() {
  COMPILE("u16 check = b'1111111111111111';")

  AssertNoError();
}

static void Test_Binary_U16_Overflow() {
  COMPILE("u16 check = b'11111111000000001';")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Binary_U32_Limit_OK() {
  COMPILE("u32 check = b'11111111111111111111111111111111';")

  AssertNoError();
}

static void Test_Binary_U32_Overflow() {
  COMPILE("u32 check = b'111111110000000011111111000000001';")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_Binary_U64_Limit_OK() {
  COMPILE("u64 check = b'1111111111111111111111111111111111111111111111111111111111111111';")

  AssertNoError();
}

static void Test_Binary_U64_Overflow() {
  COMPILE("u64 check = b'11111111000000001111111100000000111111110000000011111111000000001';")

  // The Lexer creates an Error token when the binary literal exceeds a
  // length of 64
  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_Binary_NegativeLiteral_NotAllowed() {
  COMPILE("u64 check = -b'1000100100010110';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_Binary_LargeZeroLiteralInSmallType_OK() {
  COMPILE("u8 check = b'0000000000000000';")

  AssertNoError();
}

static void Test_Binary_IntAssignment_NotAllowed() {
  COMPILE("i32 check = b'10001001';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_Binary_FloatAssignment_NotAllowed() {
  COMPILE("f32 check = b'10001001';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

/* ==== Floats ==== */
static void Test_Float_LeadingDecimalLiteral_NotAllowed() {
  COMPILE(".12345;")

  AssertExpectError(ERR_UNEXPECTED);
}

static void Test_Float_TrailingDecimalLiteral_NotAllowed() {
  COMPILE("456.;")

  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_F32MAX_Limit_OK() {
  COMPILE("f32 check = 340282346638528859811704183484516925440.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(FLT_MAX));
}

static void Test_F32MIN_Limit_OK() {
  COMPILE("f32 check = -340282346638528859811704183484516925440.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(-FLT_MAX));
}

static void Test_F64MAX_Limit_OK() {
  COMPILE("f64 check = 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(DBL_MAX));
}

static void Test_F64MIN_Limit_OK() {
  COMPILE("f64 check = -179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000;")

  AssertNoError();
  AssertEqual(NewFloatValue(-DBL_MAX));
}

static void Test_F32_Overflow() {
  COMPILE("f32 check = 440282346638528859811704183484516925440.000000;")

  AssertExpectError(ERR_OVERFLOW);
}

static void Test_F64_Overflow() {
  COMPILE("f64 check = 279769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.000000;")

  AssertExpectError(ERR_OVERFLOW);
}

void RunAllNumberTests() {
  /* ---- Signed ---- */
  Test_I8Min_Limit_OK();
  Test_I16Min_Limit_OK();
  Test_I32Min_Limit_OK();
  Test_I64Min_Limit_OK();

  Test_I8Max_Limit_OK();
  Test_I16Max_Limit_OK();
  Test_I32Max_Limit_OK();
  Test_I64Max_Limit_OK();

  Test_I8Max_Overflow();
  Test_I16Max_Overflow();
  Test_I32Max_Overflow();
  Test_I64Max_Overflow();

  Test_I8Min_Overflow();
  Test_I16Min_Overflow();
  Test_I32Min_Overflow();
  Test_I64Min_Overflow();

  Test_DoubleUnaryMinus_IsPositive();

  /* --- Unsigned --- */
  Test_U8_Limit_OK();
  Test_U16_Limit_OK();
  Test_U32_Limit_OK();
  Test_U64_Limit_OK();

  Test_U8_Overflow();
  Test_U16_Overflow();
  Test_U32_Overflow();
  Test_U64_Overflow();

  Test_Uint_NegativeLiteral_Overflows();

  /* ----  Hex   ---- */
  Test_Hex_U8_Overflow();
  Test_Hex_U16_Overflow();
  Test_Hex_U32_Overflow();
  Test_Hex_U64_Overflow();

  Test_Hex_NegativeLiteral_NotAllowed();
  Test_Hex_IntAssignment_NotAllowed();
  Test_Hex_FloatAssignment_NotAllowed();

  /* ---- Binary ---- */
  Test_Binary_U8_Limit_OK();
  Test_Binary_U16_Limit_OK();
  Test_Binary_U32_Limit_OK();
  Test_Binary_U64_Limit_OK();

  Test_Binary_U8_Overflow();
  Test_Binary_U16_Overflow();
  Test_Binary_U32_Overflow();
  Test_Binary_U64_Overflow();

  Test_Binary_NegativeLiteral_NotAllowed();
  Test_Binary_LargeZeroLiteralInSmallType_OK();
  Test_Binary_IntAssignment_NotAllowed();
  Test_Binary_FloatAssignment_NotAllowed();


  /* ---- Floats ---- */
  Test_Float_LeadingDecimalLiteral_NotAllowed();
  Test_Float_TrailingDecimalLiteral_NotAllowed();

  Test_F32MAX_Limit_OK();
  Test_F32MIN_Limit_OK();
  Test_F64MAX_Limit_OK();
  Test_F64MIN_Limit_OK();

  Test_F32_Overflow();
  Test_F64_Overflow();

  PrintAssertionResults("Numbers");
}
