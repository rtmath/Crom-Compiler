#include <errno.h>
#include <stdio.h>
#include <stdlib.h>   // for system
#include <string.h>   // for strerror
#include <sys/wait.h> // for WEXITSTATUS

#include "../src/common.h"
#include "assert.h"
#include "test_io.h"

void RunTest(char *compiler_path, char *test_path, char *file_name, char *group_name) {
  char *partial_command = Concat(compiler_path, " ");
  char *full_command = Concat(partial_command, test_path);

  errno = 0;
  int result = system(full_command);
  if (errno != 0) {
    printf("RunTest(): Non-zero ERRNO running test '%s': %s\n", test_path, strerror(errno));
    exit(256);
  }

  if (result == -1) {
    printf("RunTest(): Child process could not be created\n");
    exit(256);
  }

  int status = WEXITSTATUS(result);
  if (status == 127) {
    printf("RunTest(): Shell could not be executed in child process\n");
    exit(256);
  }

  int expected = ExtractExpectedResult(test_path);

  Assert(expected, status, file_name, group_name);
}

int main() {
  char *ProgramPath = CompilerProgramPath();
  struct Filepaths Subfolders = FolderPaths();

  for (int i = 0; i < Subfolders.count; i++) {
    char *group_name = ExtractEndOfPath(Subfolders.names[i]);
    struct Filepaths TestFiles = TestPaths(Subfolders.names[i]);

    for (int j = 0; j < TestFiles.count; j++) {
      char *file_name = ExtractEndOfPath(TestFiles.names[j]);
      RunTest(ProgramPath, TestFiles.names[j], file_name, group_name);
    }

    PrintAssertionResults(group_name);
  }
}
