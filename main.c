#include "preprocessor.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *temp, *output;
    char filename[1024], *dot;

    if (argc != 2)
    {
        printf("Usage: ./mypreprocessor inputfile.c\n");
        return 1;
    }

    temp = tmpfile();

    if (temp == NULL)
    {
        perror("tmpfile");
        return 1;
    }

    if (!include_files(argv[1], temp, 0))
    {
        fclose(temp);
        return 1;
    }

    rewind(temp);

    snprintf(filename, sizeof(filename), "%s", argv[1]);

    dot = strrchr(filename, '.');

    if (dot && strcmp(dot, ".c") == 0)
        strcpy(dot, ".i");
    else if (strlen(filename) + 2 < sizeof(filename))
        strcat(filename, ".i");

    output = fopen(filename, "w");

    if (output == NULL)
    {
        perror(filename);
        fclose(temp);
        return 1;
    }

    replace_macros(temp, output);

    fclose(temp);
    fclose(output);

    printf("Created %s\n", filename);

    return 0;
}

