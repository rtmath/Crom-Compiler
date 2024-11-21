#include <stdio.h>

#include "common.h"
#include "test_numbers.h"
#include "test_strings.h"

void RunTests() {
  RunAllNumberTests();
  RunAllStringTests();
}

int main() {
  RunTests();
  return 0;
}
