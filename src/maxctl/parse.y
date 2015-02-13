%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct keywords {
        const char      *name;
        int              val;
};

static FILE             *fin = NULL;
static int               lineno = 1;
static int               errors = 0;

int yyerror(char *message);
int yyparse(void);
int yylex(void);

%}

%token DEVICE AUTO ECO COMFORT STRING CONFIG ROOM
%token ERROR
%%

ruleset         : /* empty */
                | ruleset '\n'
                | ruleset device '\n' {printf("device\n");}
                ;

device          : DEVICE STRING '{' config '}' ';' {printf("config\n");}
                ;

config          : /* empty */
                | config '\n'
                | config param '\n' {printf("param\n");}
                
param           : ROOM STRING ';' {printf("room\n");}
                | COMFORT STRING ';' {printf("comfort\n");}
                | ECO STRING ';' {printf("eco\n");}
                | AUTO '{' schedule '}' ';' {printf("schedule\n");}
                ;
                
schedule        : /* empty */
                | schedule '\n'
                | schedule STRING '{' program '}' ';' {printf("program\n");}
                ;
                
program         : /* empty */
                | program '\n'
                | program STRING STRING ';' {printf("temp setting\n");}
                
%%

#define STACK_SZ 32

int stack[STACK_SZ];
int	sp = 0;

/* Reversible getc */
int
rev_getc(FILE* fp)
{
    if (sp > 0)
    {
        return stack[--sp];
    }
    return getc(fp);
}

int
rev_putc(int c)
{
    if (c == EOF)
        return (EOF);
    if (sp < STACK_SZ)
    {
        stack[sp++] = c;
        return c;
    }
    return 0;
}
int
kcmp(const void *k, const void *e)
{
    return (strcmp(k, ((const struct keywords *)e)->name));
}

int
get_keyword(char *s)
{
    /* Keep this list sorted */
    static const struct keywords keywords[] = {
            { "auto", AUTO},
            { "comfort", COMFORT},
            { "device", DEVICE},
            { "eco", ECO},
            { "room", ROOM},
        };

    const struct keywords *kw;

    kw = bsearch(s, keywords, sizeof(keywords)/sizeof(keywords[0]),
        sizeof(keywords[0]), kcmp);

    if (kw != NULL) {
        return (kw->val);
    } else {
        return (STRING);
    }
}

int yylex(void)
{
    int      c, token;
    char     buf[8096], *p;

    p = buf;
    /* Ignore whitespaces before anything */
    while ((c = rev_getc(fin)) == ' ')
                ; /* nothing */

    /* Ignore comments */
    if (c == '#')
        while ((c = rev_getc(fin)) != '\n' && c != EOF)
            ; /* nothing */

#define allowed_in_string(x) \
	(isalnum(x) || x == ':')
    if (isalnum(c))
    {
        do {
            *p++ = c;
            if ((unsigned)(p-buf) >= sizeof(buf))
            {
                yyerror("string too long");
                return ERROR;
            }
        } while ((c = rev_getc(fin)) != EOF && allowed_in_string(c));
        rev_putc(c);
        *p++ = '\0';
        token = get_keyword(buf);
        return token;
    }

    /* Ignore '\r' */
    if (c == '\r')
    {
        while ((c = rev_getc(fin)) == '\r')
            ; /* nothing */
    }

    if (c == EOF)
            return (0);

    return c;
}

int yyerror(char *message)
{
    return 0;
}

int
parse_file(FILE *input)
{
    fin = input;
    lineno = 1;
    errors = 0;

    yyparse();

    return (errors ? -1 : 0);
}

