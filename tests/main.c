#include <stdio.h>

#include "common.h"
#include "test_literals.h"
#include "test_integers.h"

void RunTests() {
  RunAllLiteralTests();
  RunAllIntegerTests();
}

int main() {
  RunTests();
  return 0;
}
