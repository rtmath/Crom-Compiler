#include <errno.h>  // for errno
#include <stdio.h>  // for fopen et al.
#include <stdlib.h> // for malloc
#include <string.h> // for strerror

#include "common.h"
#include "error.h"
#include "io.h"

int ReadFile(const char *filename, char **dest) {
  FILE *fd = fopen(filename, "r");
  if (fd == NULL) ERROR_EXIT("Could not open file %s: %s", filename, strerror(errno));

  fseek(fd, 0L, SEEK_END);
  size_t filesize = ftell(fd);
  rewind(fd);

  char *contents = malloc(filesize + ROOM_FOR_NULL_BYTE);
  if (contents == NULL) ERROR_EXIT("Not enough memory to read file %s: ", filename, strerror(errno));

  size_t bytes_read = fread(contents, sizeof(char), filesize, fd);
  if (bytes_read < filesize) ERROR_EXIT("Could only read %d of %d bytes from file %s", bytes_read, filesize, filename);

  contents[bytes_read] = '\0';

  fclose(fd);

  *dest = contents;
  return bytes_read;
}
