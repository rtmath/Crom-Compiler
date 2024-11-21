#include "assert.h"
#include "test_strings.h"

/* ==== CHAR ==== */
static void Test_Char_Uppercase_OK() {
  COMPILE("char c = 'O';")

  AssertNoError();
}

static void Test_Char_Lowercase_OK() {
  COMPILE("char c = 'k';")

  AssertNoError();
}

static void Test_Char_Escaped_OK() {
  // The backslash needs to be escaped here to simulate the behavior of
  // reading this source code from a file, where '\n' would be two characters
  COMPILE("char c = '\\n';")

  AssertNoError();
}

static void Test_Char_EmptyLiteral_NotAllowed() {
  COMPILE("char c = '';")

  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_Char_MulticharLiteral_NotAllowed() {
  COMPILE("char c = 'ab';")

  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_Char_DoubleQuotes_NotAllowed() {
  COMPILE("char c = \"a\";")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

/* === STRING === */
static void Test_String_CharAssignment_NotAllowed() {
  COMPILE("string s = 'x';")

  AssertExpectError(ERR_TYPE_DISAGREEMENT);
}

static void Test_String_EmptyLiteral_OK() {
  COMPILE("string s = \"\";")

  AssertNoError();
}

static void Test_String_UnterminatedLiteral_NotAllowed() {
  COMPILE("string s = \"wassup my dude;")

  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_String_Multiline_NotAllowed() {
  COMPILE("string s = \"multi\n line\n string\";")

  AssertExpectError(ERR_LEXER_ERROR);
}

static void Test_String_ArrayAccess_OK() {
  COMPILE("string s = \"gucci\";"
          "char c = s[4];")

  AssertNoError();
}

void RunAllStringTests() {
  /* ---- CHAR ---- */
  Test_Char_Uppercase_OK();
  Test_Char_Lowercase_OK();
  Test_Char_Escaped_OK();
  Test_Char_EmptyLiteral_NotAllowed();
  Test_Char_MulticharLiteral_NotAllowed();
  Test_Char_DoubleQuotes_NotAllowed();

  /* --- STRING --- */
  Test_String_CharAssignment_NotAllowed();
  Test_String_EmptyLiteral_OK();
  Test_String_UnterminatedLiteral_NotAllowed();
  Test_String_Multiline_NotAllowed();
  Test_String_ArrayAccess_OK();

  PrintAssertionResults("Strings");
}
