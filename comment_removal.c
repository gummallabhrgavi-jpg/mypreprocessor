#include "preprocessor.h"

void remove_comments(const char *src, char *dest, int *in_block)
{
    int i = 0, j = 0;
    char quote = 0;

    while (src[i] != '\0')
    {
        if (*in_block)
        {
            if (src[i] == '*' && src[i + 1] == '/')
            {
                *in_block = 0;
                i += 2;
            }
            else
            {
                if (src[i] == '\n')
                    dest[j++] = '\n';

                i++;
            }
        }
        else if (quote)
        {
            dest[j++] = src[i];

            if (src[i] == '\\' && src[i + 1])
            {
                dest[j++] = src[++i];
            }
            else if (src[i] == quote)
            {
                quote = 0;
            }

            i++;
        }
        else if (src[i] == '"' || src[i] == '\'')
        {
            quote = src[i];
            dest[j++] = src[i++];
        }
        else if (src[i] == '/' && src[i + 1] == '/')
        {
            while (src[i] && src[i] != '\n')
                i++;
        }
        else if (src[i] == '/' && src[i + 1] == '*')
        {
            dest[j++] = ' ';
            *in_block = 1;
            i += 2;
        }
        else
        {
            dest[j++] = src[i++];
        }
    }

    dest[j] = '\0';
}
