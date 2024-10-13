#include <stddef.h> // for NULL

#include "compiler.h"
#include "io.h"

int main(void) {
  const char *filename = "test.txt";
  char *contents = NULL;
  ReadFile(filename, &contents);

  Compile(contents);

  return 0;
}
