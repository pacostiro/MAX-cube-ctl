%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "max_parser.h"

struct keywords {
        const char      *name;
        int              val;
};

static FILE  *fin = NULL;
static int   lineno = 1;
static int   col = 1;
static int   errors = 0;

static struct ruleset **max_ruleset = NULL;

int yyerror(char *message);
int yyparse(void);
int yylex(void);

int day_index(char *day);

typedef struct {
        union {
                u_int32_t             number;
                int                   i;
                char                  *string;
                struct program        *program;
                struct auto_schedule  *auto_schedule;
                struct devparam       *devparam;
                struct device_config  *device_config;
                struct ruleset        *ruleset;
        } v;
        int lineno;
} YYSTYPE;
 

%}

%start ruleset

%token DEVICE AUTO ECO COMFORT STRING CONFIG ROOM
%token ERROR

%type  <v.string> STRING
%type  <v.program> program
%type  <v.auto_schedule> schedule
%type  <v.devparam> param
%type  <v.device_config> config
%type  <v.device_config> device
%type  <v.ruleset> ruleset

%%

ruleset     : /* empty */ { $$ = NULL; }
            | ruleset '\n'
            | ruleset device '\n' {
                struct ruleset *rs;

                rs = (struct ruleset*)
                    malloc(sizeof(struct ruleset));
                rs->device_config = $2;
#ifdef MAX_PARSER_DEBUG
                printf("ruleset adding device %x\n",
                       rs->device_config->rf_address);
#endif
                rs->next = NULL;
                if ($1 != NULL)
                {
                    struct ruleset *iter = $1;
                    while (iter->next != NULL)
                    {
                        iter = iter->next;
                    }
                    iter->next = rs;
                    $$ = $1;
                }
                else
                {
                    /* This is the set of the rule set */
                    *max_ruleset = rs;
                    $$ = rs;
                }
            }
            ;

device      : DEVICE STRING '{' config '}' ';' {
                char *endptr;
                struct device_config *dc;

                dc = $4;
                if (dc == NULL)
                {
                    dc = (struct device_config*)
                        malloc(sizeof(struct device_config));
                    dc->config.room_id = NOT_CONFIGURED_UL;
                    dc->config.auto_schedule = NULL;
                }
                dc->rf_address = strtol($2, &endptr, 16);
                if (*endptr != '\0')
                {
                    free($2);
                    yyerror("invalid value for temperature");
                    YYERROR;
                }
                free($2);
#ifdef MAX_PARSER_DEBUG
                printf("device %x config OK!\n", dc->rf_address);
#endif
                $$ = dc;
            }
            ;

config      : /* empty */ { $$ = NULL; }
            | config '\n'
            | config param '\n' {
                struct device_config *dc;

                if ($1 == NULL)
                {
                    dc = (struct device_config*)
                        malloc(sizeof(struct device_config));
                    dc->config.room_id = NOT_CONFIGURED_UL;
                    dc->config.eco_temp = NOT_CONFIGURED_F;
                    dc->config.comfort_temp = NOT_CONFIGURED_F;
                    dc->config.skip = 0;
                    dc->config.auto_schedule = NULL;
                }
                else
                {
                    dc = $1;
                }
                switch ($2->ptype)
                {
                    case RoomId:
                        dc->config.room_id = $2->param.room_id;
                        break;
                    case Eco:
                        dc->config.eco_temp = $2->param.eco_temp;
                        break;
                    case Comfort:
                        dc->config.comfort_temp = $2->param.comfort_temp;
                        break;
                    case Auto:
                        dc->config.auto_schedule = $2->param.auto_schedule;
                        break;
                }
                free($2);
#ifdef MAX_PARSER_DEBUG
                printf("config room id: %d eco: %f comfort %f\n",
                       dc->config.room_id, dc->config.eco_temp,
                       dc->config.comfort_temp);
#endif

                $$ = dc;
            }
                
param       : ROOM STRING ';' {
                struct devparam *dp;
                char *endptr;

                dp = (struct devparam*) malloc(sizeof(struct devparam));
                dp->ptype = RoomId;
                dp->param.room_id = (uint32_t) strtol($2, &endptr, 10);
                if (*endptr != '\0')
                {
                    free($2);
                    yyerror("invalid value for room");
                    YYERROR;
                }
                free($2);
#ifdef MAX_PARSER_DEBUG
                printf("room id: %d\n", dp->param.room_id);
#endif
                $$ = dp;
            }
            | COMFORT STRING ';' {
                struct devparam *dp;
                char *endptr;

                dp = (struct devparam*) malloc(sizeof(struct devparam));
                dp->ptype = Comfort;
                dp->param.comfort_temp = strtof($2, &endptr);
                if (*endptr != '\0')
                {
                    free($2);
                    yyerror("invalid value for temperature");
                    YYERROR;
                }
                free($2);
#ifdef MAX_PARSER_DEBUG
                printf("comfort temp: %.1f\n", dp->param.comfort_temp);
#endif
                $$ = dp;
            }
            | ECO STRING ';' {
                struct devparam *dp;
                char *endptr;

                dp = (struct devparam*) malloc(sizeof(struct devparam));
                dp->ptype = Eco;
                dp->param.eco_temp = strtof($2, &endptr);
                if (*endptr != '\0')
                {
                    free($2);
                    yyerror("invalid value for temperature");
                    YYERROR;
                }
                free($2);
#ifdef MAX_PARSER_DEBUG
                printf("eco temp: %.1f\n", dp->param.eco_temp);
#endif
                $$ = dp;
            }
            | AUTO '{' schedule '}' ';' {
                struct devparam *dp;

                dp = (struct devparam*) malloc(sizeof(struct devparam));
                dp->ptype = Auto;
                dp->param.auto_schedule = $3;
#ifdef MAX_PARSER_DEBUG
                printf("weekly schedule\n");
#endif
                $$ = dp;
            }
            ;
                
schedule    : /* empty */ { $$ = NULL; }
            | schedule '\n'
            | schedule STRING '{' program '}' ';' {
                struct auto_schedule *as;
                int index;

                as = (struct auto_schedule*)
                     malloc(sizeof(struct auto_schedule));
                index = day_index($2);
                if (index < 0)
                {
                    yyerror("invalid day of the week");
                    YYERROR;
                }
                as->day = (uint16_t) index;
#ifdef MAX_PARSER_DEBUG
                printf("schedule day: %s %d\n", $2, as->day);
#endif
                as->skip = 0;
                as->schedule = $4;
                free($2);
                as->next = NULL;
                if ($1 != NULL)
                {
                    struct auto_schedule *iter = $1;
                    while (iter->next != NULL)
                    {
                        iter = iter->next;
                    }
                    iter->next = as;
                    $$ = $1;
                }
                else
                {
                    $$ = as;
                }
            }
            ;
                
program     : /* empty */ { $$ = NULL; }
            | program '\n'
            | program STRING STRING ';' {
                struct program *pm;
                char *s;

                pm = (struct program*) malloc(sizeof(struct program));
                pm->temperature = strtof($2, &s);
                if (*s != '\0')
                {
                    free($2);
                    yyerror("invalid value for temperature");
                    YYERROR;
                }
                free($2);
                s = strchr($3, ':');
                if (s != NULL)
                {
                    char *endptr1, *endptr2;
                    *s = '\0';
                    s++;
                    pm->hour = (uint32_t) strtol($3, &endptr1, 10);
                    pm->minutes = (uint32_t) strtol(s, &endptr2, 10);
                    if (*endptr1 != '\0' || *endptr2 != '\0')
                    {
                        yyerror("invalid time");
                        YYERROR;
                    }
                }
                else
                {
                    yyerror("invalid time");
                    YYERROR;
                }
                free($3);
#ifdef MAX_PARSER_DEBUG
                printf("program temp setting %.1f %02d:%02d\n", pm->temperature,
                       pm->hour, pm->minutes);
#endif
                pm->next = NULL;
                if ($1 != NULL)
                {
                    struct program *iter = $1;
                    while (iter->next != NULL)
                    {
                        iter = iter->next;
                    }
                    iter->next = pm;
                    $$ = $1;
                }
                else
                {
                    $$ = pm;
                }
            }
                
%%

int day_index(char *day)
{
    static const char *days[] = {
            "saturday",
            "sunday",
            "monday",
            "tuesday",
            "wednesday",
            "thursday",
            "friday",
        };
    int i;

    for (i = 0; i < sizeof(days) / sizeof(days[0]); i++)
    {
        if (strcmp(days[i], day) == 0)
            return i;
    }

    return -1;
}

#define STACK_SZ 32
int stack[STACK_SZ];
int	sp = 0;

/* Reversible getc */
int
rev_getc(FILE* fp)
{
    col++;
    if (sp > 0)
    {
        return tolower(stack[--sp]);
    }
    return tolower(getc(fp));
}

int
rev_putc(int c)
{
    if (c == EOF)
        return (EOF);
    if (sp < STACK_SZ)
    {
        col--;
        stack[sp++] = c;
        return c;
    }
    return (EOF);
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

    yylval.lineno = lineno;

    /* Ignore comments */
    if (c == '#')
    {
        while ((c = rev_getc(fin)) != '\n' && c != EOF)
            ; /* nothing */
    }

#define allowed_in_string(x) (isalnum(x) || x == ':' || x == '.')
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
        yylval.v.string = strdup(buf);
        if (yylval.v.string == NULL)
            yyerror("strdup");
        return token;
    }

    /* Ignore '\r' */
    if (c == '\r')
    {
        while ((c = rev_getc(fin)) == '\r')
            ; /* nothing */
    }

    if (c == '\n')
    {
        lineno++;
        col = 1;
        yylval.lineno = lineno;
    }

    if (c == EOF)
    {
            return (0);
    }

    return c;
}

int yyerror(char *message)
{
    errors = 1;
    fprintf(stderr, "parsing error: %s line %d:%d\n", message, yylval.lineno, col);
    return 0;
}

int
parse_file(FILE *input, struct ruleset **ruleset)
{
    fin = input;
    lineno = 1;
    col = 1;
    errors = 0;
    if (ruleset == NULL)
    {
        fprintf(stderr, "parse_file ruleset argument cannot be NULL\n");
        return -1;
    }
    max_ruleset = ruleset;

    yyparse();

    return (errors ? -1 : 0);
}

