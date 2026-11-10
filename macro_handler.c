#include "preprocessor.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char name[64], body[MAX_LINE];
    char params[MAX_PARAMS][64];
    int is_function, count;
} Macro;

static Macro macros[MAX_MACROS];
static int macro_count = 0;

static int ident(int c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static void add(char *out, int *j, const char *text)
{
    while (*text && *j < MAX_LINE - 1)
        out[(*j)++] = *text++;

    out[*j] = '\0';
}

static void save_macro(char *line)
{
    char *p = line;
    int j = 0;
    Macro *m;

    if (macro_count == MAX_MACROS)
        return;

    m = &macros[macro_count];
    memset(m, 0, sizeof(*m));

    while (*p == ' ' || *p == '\t')
        p++;

    if (!isalpha((unsigned char)*p) && *p != '_')
        return;

    while (ident(*p) && j < 63)
        m->name[j++] = *p++;

    while (ident(*p))
        p++;

    m->name[j] = '\0';

    if (*p == '(')
    {
        m->is_function = 1;
        p++;

        while (*p && *p != ')')
        {
            while (*p == ' ' || *p == '\t')
                p++;

            if (m->count >= MAX_PARAMS)
                return;

            j = 0;

            while (ident(*p) && j < 63)
                m->params[m->count][j++] = *p++;

            while (ident(*p))
                p++;

            if (j == 0)
                return;

            m->params[m->count++][j] = '\0';

            while (*p == ' ' || *p == '\t')
                p++;

            if (*p == ',')
                p++;
            else if (*p != ')')
                return;
        }

        if (*p != ')')
            return;

        p++;
    }

    while (*p == ' ' || *p == '\t')
        p++;

    snprintf(m->body, sizeof(m->body), "%s", p);

    m->body[strcspn(m->body, "\r\n")] = '\0';

    macro_count++;
}

static int get_args(const char *p,
                    char args[MAX_PARAMS][MAX_LINE],
                    int *used)
{
    int n = 0, j = 0, level = 1, i = 1;
    char quote = 0;

    memset(args, 0, MAX_PARAMS * MAX_LINE);

    if (p[0] != '(')
        return -1;

    if (p[i] == ')')
    {
        *used = 2;
        return 0;
    }

    while (p[i])
    {
        char c = p[i];

        if (quote)
        {
            if (c == '\\' && p[i + 1])
            {
                if (j < MAX_LINE - 2)
                {
                    args[n][j++] = c;
                    args[n][j++] = p[++i];
                }
            }
            else
            {
                if (j < MAX_LINE - 1)
                    args[n][j++] = c;

                if (c == quote)
                    quote = 0;
            }
        }
        else if (c == '"' || c == '\'')
        {
            quote = c;

            if (j < MAX_LINE - 1)
                args[n][j++] = c;
        }
        else if (c == '(')
        {
            level++;

            if (j < MAX_LINE - 1)
                args[n][j++] = c;
        }
        else if (c == ')')
        {
            level--;

            if (level == 0)
            {
                args[n][j] = '\0';
                *used = i + 1;
                return n + 1;
            }

            if (j < MAX_LINE - 1)
                args[n][j++] = c;
        }
        else if (c == ',' && level == 1)
        {
            args[n++][j] = '\0';

            if (n >= MAX_PARAMS)
                return -1;

            j = 0;
        }
        else
        {
            if (j < MAX_LINE - 1)
                args[n][j++] = c;
        }

        i++;
    }

    return -1;
}

static void substitute(const Macro *m,
                       char args[MAX_PARAMS][MAX_LINE],
                       char *out)
{
    int i = 0, j = 0, k, start;
    char word[64];

    out[0] = '\0';

    while (m->body[i])
    {
        if (isalpha((unsigned char)m->body[i]) ||
            m->body[i] == '_')
        {
            start = i;

            while (ident(m->body[i]))
                i++;

            int len = i - start;

            if (len > 63)
                len = 63;

            memcpy(word, m->body + start, len);
            word[len] = '\0';

            for (k = 0; k < m->count; k++)
            {
                if (strcmp(word, m->params[k]) == 0)
                    break;
            }

            if (k < m->count)
            {
                add(out, &j, args[k]);
            }
            else
            {
                char token[64];

                memcpy(token, m->body + start, len);
                token[len] = '\0';

                add(out, &j, token);
            }
        }
        else
        {
            char c[2] = {m->body[i++], '\0'};
            add(out, &j, c);
        }
    }
}

static void expand(const char *src, char *out, int depth)
{
    int i = 0, j = 0, k;
    char name[64], tmp[MAX_LINE];
    char args[MAX_PARAMS][MAX_LINE];

    out[0] = '\0';

    if (depth > 10)
    {
        add(out, &j, src);
        return;
    }

    while (src[i])
    {
        if (src[i] == '"' || src[i] == '\'')
        {
            char quote = src[i];
            char c[2] = {src[i++], 0};

            add(out, &j, c);

            while (src[i])
            {
                c[0] = src[i++];
                add(out, &j, c);

                if (c[0] == '\\' && src[i])
                {
                    c[0] = src[i++];
                    add(out, &j, c);
                }
                else if (c[0] == quote)
                {
                    break;
                }
            }
        }
        else if (isalpha((unsigned char)src[i]) ||
                 src[i] == '_')
        {
            int start = i;
            int len, idx = -1, used = 0, count;

            while (ident(src[i]))
                i++;

            len = i - start;

            if (len < 64)
            {
                memcpy(name, src + start, len);
                name[len] = '\0';

                for (k = macro_count - 1; k >= 0; k--)
                {
                    if (strcmp(macros[k].name, name) == 0)
                    {
                        idx = k;
                        break;
                    }
                }
            }

            if (idx >= 0 && macros[idx].is_function)
            {
                int pos = i;

                while (src[pos] == ' ' || src[pos] == '\t')
                    pos++;

                count = get_args(src + pos, args, &used);

                if (src[pos] != '(' ||
                    count != macros[idx].count)
                {
                    idx = -1;
                }
                else
                {
                    i = pos + used;
                }
            }

            if (idx >= 0)
            {
                char result[MAX_LINE];

                if (macros[idx].is_function)
                {
                    substitute(&macros[idx], args, tmp);
                    expand(tmp, result, depth + 1);
                }
                else
                {
                    expand(macros[idx].body, result, depth + 1);
                }

                add(out, &j, result);
            }
            else
            {
                while (start < i && j < MAX_LINE - 1)
                    out[j++] = src[start++];

                out[j] = '\0';
            }
        }
        else
        {
            char c[2] = {src[i++], 0};
            add(out, &j, c);
        }
    }
}

void replace_macros(FILE *input, FILE *output)
{
    char line[MAX_LINE], result[MAX_LINE], *p;

    macro_count = 0;

    while (fgets(line, sizeof(line), input))
    {
        p = line;

        while (*p == ' ' || *p == '\t')
            p++;

        if (strncmp(p, "#define", 7) == 0 &&
            isspace((unsigned char)p[7]))
        {
            save_macro(p + 7);
            fputc('\n', output);
        }
        else
        {
            expand(line, result, 0);
            fputs(result, output);
        }
    }
}

