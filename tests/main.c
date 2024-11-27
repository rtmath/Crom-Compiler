#include <stdio.h>

#include "common.h"
#include "test_bitwise.h"
#include "test_bools.h"
#include "test_enums.h"
#include "test_numbers.h"
#include "test_strings.h"
#include "test_structs.h"
#include "test_terse_assignments.h"

void RunTests() {
  RunAllBitwiseTests();
  RunAllBoolTests();
  RunAllEnumTests();
  RunAllNumberTests();
  RunAllStringTests();
  RunAllStructTests();
  RunAllTerseAssignmentTests();
}

int main() {
  RunTests();
  return 0;
}
