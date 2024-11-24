#include "assert.h"
#include "test_bitwise.h"

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

//TODO: Test bitwise failing on non-UINT types

void RunAllBitwiseTests() {
  Test_OR_OK();
  Test_AND_OK();
  Test_XOR_OK();

  PrintAssertionResults("Bitwise");
}
