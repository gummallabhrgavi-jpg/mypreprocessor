#include "preprocessor.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int include_files(const char *filename, FILE *output, int depth)
{
    FILE *fp;
    char line[MAX_LINE];
    char clean[MAX_LINE];
    char name[256];
    char path[1024];
    char *p, *slash;
    int block = 0, n;

    if (depth > 10)
    {
        printf("Too many includes\n");
        return 0;
    }

    fp = fopen(filename, "r");

    if (fp == NULL)
    {
        printf("Cannot open %s\n", filename);
        return 0;
    }

    while (fgets(line, sizeof(line), fp))
    {
        remove_comments(line, clean, &block);

        p = clean;

        while (isspace((unsigned char)*p))
            p++;

        n = 0;

        if (strncmp(p, "#include", 8) == 0 &&
            (isspace((unsigned char)p[8]) ||
             p[8] == '"' || p[8] == '<'))
        {
            p += 8;

            while (*p == ' ' || *p == '\t')
                p++;

            if (*p == '"')
                n = sscanf(p, "\"%255[^\"]\"", name);
            else if (*p == '<')
                n = sscanf(p, "<%255[^>]>", name);
        }

        if (n == 1)
        {
            slash = strrchr(filename, '/');

            if (slash != NULL)
                snprintf(path, sizeof(path),
                         "%.*s/%s",
                         (int)(slash - filename),
                         filename, name);
            else
                snprintf(path, sizeof(path), "%s", name);

            if (!include_files(path, output, depth + 1))
            {
                fclose(fp);
                return 0;
            }
        }
        else
        {
            fputs(clean, output);
        }
    }

    fclose(fp);
    return 1;
}

