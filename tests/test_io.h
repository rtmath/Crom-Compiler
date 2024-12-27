#ifndef TEST_IO_H
#define TEST_IO_H

struct Filepaths {
  int count;
  char *names[256];
};

struct Filepaths FolderPaths();
struct Filepaths TestPaths(char *str);
char *CompilerProgramPath();
char *TmpFilePath();

int ExtractExpectedErrorCode(char *filename);
char *ExtractExpectedPrintOutput(char *filename);
char *ExtractEndOfPath(char *file_path);

#endif
