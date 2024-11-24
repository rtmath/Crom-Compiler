#include "assert.h"
#include "test_bitwise.h"

static void Test_NOT_OK() {
  COMPILE("u8 a = b'10001000';       "
          "u8 b = b'01110111';       "
          "                          "
          "bool check = (~a == b);")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_OR_OK() {
  COMPILE("u8 a = b'10001000';       "
          "u8 b = b'00100010';       "
          "u8 c = b'10101010';       "
          "                          "
          "bool check = (a | b == c);")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_AND_OK() {
  COMPILE("u8 a = b'10001000';       "
          "u8 b = b'00100010';       "
          "u8 c = b'00000000';       "
          "                          "
          "bool check = (a & b == c);")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_XOR_OK() {
  COMPILE("u8 a = b'11101000';       "
          "u8 b = b'00101111';       "
          "u8 c = b'11000111';       "
          "                          "
          "bool check = (a ^ b == c);")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_LSHIFT_OK() {
  COMPILE("u8 a = b'00111000';        "
          "u8 b = b'01110000';        "
          "                           "
          "bool check = (a << 1) == b;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_RSHIFT_OK() {
  COMPILE("u8 a = b'10101010';        "
          "u8 b = b'01010101';        "
          "                           "
          "bool check = (a >> 1) == b;")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_NOT_NonUint_NotAllowed() {
  COMPILE("i8 a = 6; "
          "          "
          "u8 b = ~a;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_OR_NonUint_NotAllowed() {
  COMPILE("i8 a = 6;    "
          "i8 b = 10;   "
          "             "
          "u8 c = a | b;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_AND_NonUint_NotAllowed() {
  COMPILE("i8 a = 6;    "
          "i8 b = 10;   "
          "             "
          "u8 c = a & b;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_XOR_NonUint_NotAllowed() {
  COMPILE("i8 a = 6;    "
          "i8 b = 10;   "
          "             "
          "u8 c = a ^ b;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_LSHIFT_NonUint_NotAllowed() {
  COMPILE("f32 a = 6.0;  "
          "f32 b = 10.3; "
          "              "
          "u8 c = a << b;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_RSHIFT_NonUint_NotAllowed() {
  COMPILE("i8 a = 6;     "
          "i8 b = 10;    "
          "              "
          "u8 c = a >> b;")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_OR_DifferentBitWidths_OK() {
  COMPILE("u8  a =         b'11110000';"
          "u16 b = b'1010101000001111';"
          "u16 c = b'1010101011111111';"
          "bool check = (a | b == c);  ")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_AND_DifferentBitWidths_OK() {
  COMPILE("u8  a =         b'11110101';"
          "u16 b = b'1010101010101111';"
          "u16 c = b'0000000010100101';"
          "bool check = (a & b == c);  ")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

static void Test_XOR_DifferentBitWidths_OK() {
  COMPILE("u8  a =         b'11110101';"
          "u16 b = b'1010101010101111';"
          "u16 c = b'1010101001011010';"
          "bool check = (a ^ b == c);  ")

  AssertNoError();
  AssertEqual(NewBoolValue(true));
}

void RunAllBitwiseTests() {
  Test_NOT_OK();
  Test_OR_OK();
  Test_AND_OK();
  Test_XOR_OK();
  Test_LSHIFT_OK();
  Test_RSHIFT_OK();

  Test_NOT_NonUint_NotAllowed();
  Test_OR_NonUint_NotAllowed();
  Test_AND_NonUint_NotAllowed();
  Test_XOR_NonUint_NotAllowed();
  Test_LSHIFT_NonUint_NotAllowed();
  Test_RSHIFT_NonUint_NotAllowed();

  Test_OR_DifferentBitWidths_OK();
  Test_AND_DifferentBitWidths_OK();
  Test_XOR_DifferentBitWidths_OK();

  PrintAssertionResults("Bitwise");
}
