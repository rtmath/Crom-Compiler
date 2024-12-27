#include <errno.h>
#include <stdio.h>
#include <stdlib.h>   // for system
#include <string.h>   // for strerror
#include <sys/wait.h> // for WEXITSTATUS

#include "../src/common.h"
#include "assert.h"
#include "test_io.h"

void RunTest(char *compiler_path, char *test_path, char *file_name, char *group_name) {
  // Here I'm piping each test's stdout to a tmp file so that the interpreter's
  // print() output can be verified without polluting the normal stdout
  char *partial_path = Concat(compiler_path, " ");
  char *full_path = Concat(partial_path, test_path);
  char *add_pipe = Concat(full_path, " > ");
  char *full_command = Concat(add_pipe, TmpFilePath());

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

  // Assert error code matches
  int expected_code = ExtractExpectedErrorCode(test_path);
  Assert(expected_code, status, file_name, group_name);

  // Assert print output matches, if applicable
  // TODO: Cleanup
  FILE *tmp_fd = fopen(TmpFilePath(), "r");
  if (tmp_fd == NULL) {
    printf("Could not open %s\n", TmpFilePath());
    exit(300);
  }

  char test_stdout[200] = {0};
  bool stdout_exists = (fgets(&test_stdout[0], 200, tmp_fd) != NULL);

  char *expected_stdout = ExtractExpectedPrintOutput(test_path);
  if (!stdout_exists && expected_stdout == NULL) goto cleanup;

  if (stdout_exists && expected_stdout == NULL) {
    printf("Runtest(): File '%s' printed to stdout when nothing was expected\n", file_name);
    goto cleanup;
  }

  if (!stdout_exists && expected_stdout != NULL) {
    printf("Runtest(): File '%s' did not print to stdout as expected\n", file_name);
    goto cleanup;
  }

  for (int i = 0; test_stdout[i] != '\0'; i++) {
    if (test_stdout[i] == '\n') test_stdout[i] = '\0';
  }

  bool strings_match = (expected_stdout != NULL) && (strncmp(&test_stdout[0], expected_stdout, strlen(expected_stdout)) == 0);

  AssertPrintResult(strings_match, test_stdout, expected_stdout, file_name, group_name);

cleanup:
  fclose(tmp_fd);
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

  PrintResultTotals();
}
