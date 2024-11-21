#include <stdio.h>

#include "common.h"
#include "test_bools.h"
#include "test_numbers.h"
#include "test_strings.h"

void RunTests() {
  RunAllBoolTests();
  RunAllNumberTests();
  RunAllStringTests();
}

int main() {
  RunTests();
  return 0;
}
