#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>

#define MAX_LINE 4096
#define MAX_MACROS 100
#define MAX_PARAMS 5

void remove_comments(const char *src, char *dest, int *in_block);

int include_files(const char *filename, FILE *output, int depth);

void replace_macros(FILE *input, FILE *output);

#endif
