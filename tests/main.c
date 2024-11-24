#include <stdio.h>

#include "common.h"
#include "test_bitwise.h"
#include "test_bools.h"
#include "test_enums.h"
#include "test_numbers.h"
#include "test_strings.h"

void RunTests() {
  RunAllBitwiseTests();
  RunAllBoolTests();
  RunAllEnumTests();
  RunAllNumberTests();
  RunAllStringTests();
}

int main() {
  RunTests();
  return 0;
}
