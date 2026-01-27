#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Failed to open file: %s\n", path);
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buffer = (char *)malloc(length + 1);
  if (!buffer) {
    fclose(f);
    return NULL;
  }
  fread(buffer, 1, length, f);
  buffer[length] = '\0';
  fclose(f);
  return buffer;
}
