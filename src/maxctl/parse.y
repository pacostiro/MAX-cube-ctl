%{
#include <stdio.h>

struct keywords {
        const char      *name;
        int              val;
};

static FILE             *fin = NULL;
static int               lineno = 1;
static int               errors = 0;

int yyerror(char *message);
int yyparse(void);

%}

%token DEVICE AUTO ECO COMFORT STRING CONFIG

%%

ruleset         : /* empty */
                | ruleset device
                ;

device          :  CHAR 
                ;
%%

int
kcmp(const void *k, const void *e)
{
    return (strcmp(k, ((const struct keywords *)e)->k_name));
}

int
get_keyword(char *s)
{
    static const struct keywords keywords[] = {
            { "auto", AUTO},
            { "comfort", COMFORT},
            { "device", DEVICE},
            { "eco", ECO},
        };

    const struct keywords *kw;

    kw = bsearch(s, keywords, sizeof(keywords)/sizeof(keywords[0]),
        sizeof(keywords[0]), kcmp);

    if (kw != NULL) {
        return (p->k_val);
    } else {
        return (STRING);
    }
}

int yylex(void)
{
    int      c, token;
    char     buf[8096];

    p = buf;
    c = getc(fin);

    /* Ignore whitespaces before anything */
    while ((c = getc(fin)) == ' ')
                ; /* nothing */

    /* Ignore comments */
    if (c == '#')
        while ((c = getc(fin)) != '\n' && c != EOF)
            ; /* nothing */

    /* Ignore comments */
    if (c == '{')
    {
        int cnt = 1;
        while ((c = getc(fin)) != '\n' && c != EOF)
        {
            *p = c;
            if (c == '{')
                cnt++;
            else if ((c == '}'))
                cnt--;
            if (cnt == 0)
            {
                p--;
                break;
            }
        }
        return CONFIG;
    }

    if (isalnum(c))
    {
        *p++ = c;

        do {
            *p++ = c;
            if ((unsigned)(p-buf) >= sizeof(buf))
            {
                yyerror("string too long");
                return (findeol());
            }
        } while ((c = getc(fin)) != EOF && isalnum(c));
        *p++ = '\0';
        token = get_keyword(p);
        return token;
    }


    if (c == EOF)
            return (0);

    return CHAR;
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

    yydebug = 0;
    yynerrs = 0;
    yyerrflag = 0;
    yychar = 0;
    yyssp = NULL;
    yyvsp = NULL;
    memset(&yyval, 0, sizeof(YYSTYPE));
    memset(&yylval, 0, sizeof(YYSTYPE));
    yyss = NULL;
    yysslim = NULL;
    yyvs = NULL;
    yystacksize = 0;

    yyparse();

    return (errors ? -1 : 0);
}

