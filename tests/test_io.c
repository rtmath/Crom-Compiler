#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>   // for qsort, free
#include <string.h>   // for strlen and friends
#include <sys/stat.h> // for stat
#include <unistd.h>   // for getcwd

#include "../src/common.h"
#include "../src/error.h"
#include "test_io.h"

#define STR_SIZE 256

static char *BuildSrcFullPath() {
  char *buf = NewString(STR_SIZE);
  char *s = getcwd(buf, STR_SIZE);
  int len = strlen(s);

  if (len < 5) return false;

  if (strcmp("tests", &buf[len-5]) == 0) {
    strncpy(&buf[len-5], "src", 4);
  }

  return buf;
}

static char *BuildTestsFullPath() {
  char *buf = NewString(STR_SIZE);
  char *s = getcwd(buf, STR_SIZE);
  int len = strlen(s);

  if (len < 5) return false;

  if (strcmp("src", &buf[len-3]) == 0) {
    strncpy(&buf[len-3], "tests", 6);
  }

  return buf;
}

static char *ConcatPath(char *a, char *b) {
  char *path = Concat(a, "/");
  char *full_path = Concat(path, b);

  free(path);
  return full_path;
}

static struct Filepaths Folders(char *dir_path) {
  struct Filepaths folders = {0};

  struct dirent *ep;
  DIR *dp = opendir(dir_path);
  if (dp == NULL) return folders;

  while ((ep = readdir(dp)) != NULL) {
    struct stat s = {0};
    char *path = ConcatPath(dir_path, ep->d_name);
    int result = stat(path, &s);
    if (result != 0) {
      printf("Failed to stat() %s\n", path);
      continue;
    }

    if (S_ISDIR(s.st_mode)) {
      if (ep->d_name[0] != '.') { // skip "." and ".."
        char *fname = CopyString(ep->d_name);
        folders.names[folders.count++] = path;
      }
    }
  }

  closedir(dp);

  return folders;
}

static struct Filepaths Files(char *dir_path) {
  struct Filepaths folders = {0};

  struct dirent *ep;
  DIR *dp = opendir(dir_path);
  if (dp == NULL) return folders;

  while ((ep = readdir(dp)) != NULL) {
    struct stat s = {0};
    char *path = ConcatPath(dir_path, ep->d_name);
    int result = stat(path, &s);
    if (result != 0) {
      printf("Failed to stat() %s\n", path);
      continue;
    }

    if (S_ISREG(s.st_mode)) {
      if (ep->d_name[0] != '.') { // skip "." and ".."
        char *fname = CopyString(ep->d_name);
        folders.names[folders.count++] = path;
      }
    }
  }

  closedir(dp);

  return folders;
}

static int SortAscending(const void *a, const void *b) {
  return strcmp(*(const char**)a, *(const char**)b);
}

struct Filepaths FolderPaths() {
  char *full_path = BuildTestsFullPath();
  struct Filepaths Subfolders = Folders(full_path);

  // Sort strings in ascending order
  qsort(Subfolders.names,
        Subfolders.count,
        sizeof(Subfolders.names[0]),
        SortAscending);

  return Subfolders;
}

struct Filepaths TestPaths(char *str) {
  struct Filepaths Tests = Files(str);

  return Tests;
}

char *CompilerProgramPath() {
  return ConcatPath(BuildSrcFullPath(), "t.out");
}

int ExtractExpectedErrorCode(char *filename) {
  char buf[200];

  FILE *fd = fopen(filename, "r");
  if (fd == NULL) {
    printf("ExtractExpectedResult(): Could not open file '%s'\n", filename);
    return 0;
  }

  fgets(buf, 200, fd);

  if (buf[0] != '/' || buf[1] != '/') {
    printf("ExtractExpectedResult(): Expected line 1 comment in file\n    '%s'\n", filename);
    return 0;
  }

  char str[200] = {0};
  int j = 0;
  for (int i = 2; i < 200; i++) {
    if (buf[i] != ' ' && buf[i] != '\n') {
      str[j++] = buf[i];
    }
  }

  fclose(fd);

  return ErrorCodeLookup(str);
}

char *ExtractEndOfPath(char *file_path) {
  int len = strlen(file_path);
  int chop_location = 0;
  for (int i = len - 1; i >= 0; i--) {
    if (file_path[i] == '/') {
      chop_location = i + 1;
      break;
    }
  }

  return CopyStringL(&file_path[chop_location], len - chop_location);
}
