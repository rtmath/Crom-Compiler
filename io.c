#include <errno.h>  // for errno
#include <stdio.h>  // for fopen et al.
#include <stdlib.h> // for malloc
#include <string.h> // for strerror

#include "common.h"
#include "error.h"
#include "io.h"

int ReadFile(const char *filename, char **dest) {
  FILE *fd = fopen(filename, "r");
  if (fd == NULL) ERROR_AND_EXIT_FMTMSG("ReadFile(): Could not open file %s: %s", filename, strerror(errno));

  fseek(fd, 0L, SEEK_END);
  size_t filesize = ftell(fd);
  rewind(fd);

  char *contents = malloc(filesize + ROOM_FOR_NULL_BYTE);
  if (contents == NULL) ERROR_AND_EXIT_FMTMSG("Not enough memory to read file %s: ", filename, strerror(errno));

  size_t bytes_read = fread(contents, sizeof(char), filesize, fd);
  if (bytes_read < filesize) ERROR_AND_EXIT_FMTMSG("Could only read %d of %d bytes from file %s", bytes_read, filesize, filename);

  contents[bytes_read] = '\0';

  fclose(fd);

  *dest = contents;
  return bytes_read;
}

void PrintSourceLine(const char *filename, int line_number) {
  char buf[200];

  FILE *fd = fopen(filename, "r");
  if (fd == NULL) ERROR_AND_EXIT_FMTMSG("PrintSourceLine(): Could not open file %s: %s", filename, strerror(errno));

  for (int i = 0; i < line_number; i++) {
    fgets(buf, 200, fd);
  }

  printf("%s:\n", filename);
  printf("%5d | %s\n", line_number, buf);

  fclose(fd);
}

void PrintSourceLineOfToken(Token t) {
  PrintSourceLine(t.from_filename, t.on_line);
}
