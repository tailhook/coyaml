#include <eval.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "parser.h"
#include "vars.h"

#define SYNTAX_ERROR(cond) if(!(cond)) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %ld column %ld\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR2_NULL(message, ...) if(TRUE) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %ld column %ld: " message "\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return NULL; }
#define COYAML_DEBUG(message, ...) if(info->debug) { \
    fprintf(stderr, "COYAML: " message "\n", ##__VA_ARGS__); }

typedef enum {
    VAR_INT,
    VAR_STRING,
    MAX_VARTYPE
} vartype_t;

typedef struct variable_s {
    vartype_t type;
    union {
        long intvalue;
        struct {
            char *value;
            int length;
        } str;
    } data;
} variable_t;

typedef enum {
    TOK_NONE,
    TOK_END,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_IDENT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_PRODUCT,
    TOK_DIVISION,
    TOK_MODULO,
    TOK_LPAREN,
    TOK_RPAREN,
    MAX_TOK
} token_t;

typedef struct eval_context_s {
    coyaml_parseinfo_t *info;
    char *data;
    char *token;
    char *next;
    char *end;
    token_t curtok;
} eval_context_t;

static struct char_token_s {
    char chvalue;
    token_t token;
} char_token[] = {
    {'+', TOK_PLUS},
    {'-', TOK_MINUS},
    {'*', TOK_PRODUCT},
    {'/', TOK_DIVISION},
    {'%', TOK_MODULO},
    {'(', TOK_LPAREN},
    {')', TOK_RPAREN},
    {'}', TOK_END},
    {'\0', TOK_END}  // to feel safer
    };

static struct unit_s {
    char *unit;
    size_t value;
} units[] = {
    {"k", 1000L},
    {"ki", 1L << 10},
    {"M", 1000000L},
    {"Mi", 1L << 20},
    {"G", 1000000000L},
    {"Gi", 1L << 30},
    {"T", 1000000000000L},
    {"Ti", 1L << 40},
    {"P", 1000000000000000L},
    {"Pi", 1L << 50},
    {"E", 1000000000000000000L},
    {"Ei", 1L << 60},
    {NULL, 0},
    };

static char *find_var(coyaml_parseinfo_t *info, char *name, int nlen) {
    char *data;
    int dlen;
    char cname[nlen+1];
    memcpy(cname, name, nlen);
    cname[nlen] = 0;
    COYAML_DEBUG("Searching for ``$%s''", cname);
    if(!coyaml_get_string(info->context, cname, &data, &dlen)) {
        return data;
    }
    for(coyaml_anchor_t *a = info->anchor_first; a; a = a->next) {
        if(!strncmp(name, a->name, nlen) && strlen(a->name) == nlen) {
            if(a->events[0].type != YAML_SCALAR_EVENT) {
                SYNTAX_ERROR2_NULL("You can only substitute a scalar variable,"
                    " use ``*'' to dereference complex anchors");
            }
            return (char *)a->events[0].data.scalar.value;
        }
    }
    return NULL;
}

static token_t _next_tok(char **data, char *end) {
    char *cur = *data;
    while(isspace(*cur)) ++cur;
    for(int i = 0; i < sizeof(char_token)/sizeof(char_token[0]); ++i) {
        if(char_token[i].chvalue == *cur) {
            ++cur;
            *data = cur;
            return char_token[i].token;
        }
    }
    if(isalpha(*cur)) {
        while(cur < end && isalnum(*cur)) ++cur;
        *data = cur;
        return TOK_IDENT;
    }
    if(isdigit(*cur)) {
        while(cur < end && isalnum(*cur)) ++cur;
        *data = cur;
        return TOK_INT;
    }
    if(*cur == '"') {
        while(cur < end && (*cur != '"' || cur[-1] == '\\')) ++cur;
        ++cur;  // set position after the quote
        *data = cur;
        return TOK_STRING;
    }
    return TOK_NONE;
}

static void next_tok(eval_context_t *ctx) {
    ctx->token = ctx->next;
    if(ctx->token == ctx->end) {
        ctx->curtok = TOK_END;
    } else {
        ctx->curtok = _next_tok(&ctx->next, ctx->end);
    }
}

static char *parse_long(char *value, long *result) {
    char *end;
    long val = strtol(value, (char **)&end, 0);
    if(*end) {
        for(struct unit_s *unit = units; unit->unit; ++unit) {
            if(!strcmp(end, unit->unit)) {
                val *= unit->value;
                end += strlen(unit->unit);
                break;
            }
        }
    }
    *result = val;
    return end;
}

static char *parse_double(char *value, double *result) {
    char *end;
    double val = strtod(value, (char **)&end);
    if(*end) {
        for(struct unit_s *unit = units; unit->unit; ++unit) {
            if(!strcmp(end, unit->unit)) {
                val *= unit->value;
                end += strlen(unit->unit);
                break;
            }
        }
    }
    *result = val;
    return end;
}

static int var_to_integer(variable_t **var) {
    variable_t *v = *var;
    if(v->type == VAR_INT) return 0;
    if(v->type != VAR_STRING) {
        errno = ENOTSUP;
        return -1;
    }
    char *end;
    long val = strtol(v->data.str.value, &end, 0);
    if(end != v->data.str.value + v->data.str.length) {
        errno = EINVAL;
        return -1;
    }
    v->type = VAR_INT;
    v->data.intvalue = val;
    return 0;
}

static int var_to_string(variable_t **var) {
    if(!*var) return -1;
    if((*var)->type == VAR_STRING) return 0;
    if((*var)->type != VAR_INT) {
        errno = ENOTSUP;
        return -1;
    }
    char buf[128];  // should be enought for long
    int len = sprintf(buf, "%ld", (*var)->data.intvalue);
    if(len <= 0) return -1;
    variable_t *nvar = realloc(*var, sizeof(variable_t)+len+1);
    if(!nvar) return -1;
    nvar->type = VAR_STRING;
    nvar->data.str.value = (char*)nvar + sizeof(variable_t);
    memcpy(nvar->data.str.value, buf, len);
    nvar->data.str.value[len] = 0;
    nvar->data.str.length = len;
    *var = nvar;
    return 0;
}

static variable_t *eval_sum(eval_context_t *ctx);

static variable_t *eval_atom(eval_context_t *ctx) {
    next_tok(ctx);
    if(ctx->curtok == TOK_LPAREN) {
        variable_t *res = eval_sum(ctx);
        if(!res) return NULL;
        if(ctx->curtok != TOK_RPAREN) {
            free(res);
            errno = EINVAL;
            return NULL;
        }
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_INT) {
        variable_t *res = (variable_t *)malloc(sizeof(variable_t));
        if(!res) return NULL;
        res->type = VAR_INT;
        res->data.intvalue = atoi(ctx->token);
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_STRING) {
        variable_t *res = (variable_t *)malloc(sizeof(variable_t)
            + ctx->next - ctx->token - 1);
        if(!res) return NULL;
        res->type = VAR_STRING;
        res->data.str.value = (char *)res + sizeof(variable_t);
        res->data.str.length = ctx->next - ctx->token;
        memcpy(res->data.str.value, ctx->token+1, ctx->next - ctx->token - 1);
        res->data.str.value[ctx->next - ctx->token - 2] = 0;
        next_tok(ctx);
        return res;
    } else if(ctx->curtok == TOK_IDENT) {
        char *value = find_var(ctx->info, ctx->token, ctx->next - ctx->token);
        if(!value) {
            return NULL;
        }
        variable_t *res = (variable_t *)malloc(sizeof(variable_t));
        if(!res) return NULL;
        res->type = VAR_STRING;
        res->data.str.value = value;
        res->data.str.length = strlen(value);
        next_tok(ctx);
        return res;
    } else {
        errno = EINVAL;
        return NULL;
    }
}

static variable_t *eval_product(eval_context_t *ctx) {
    variable_t *left = eval_atom(ctx);
    if(!left) return NULL;
    while(ctx->curtok == TOK_PRODUCT
        || ctx->curtok == TOK_DIVISION
        || ctx->curtok == TOK_MODULO) {
        token_t op = ctx->curtok;
        variable_t *right = eval_product(ctx);
        if(!right) {
            free(left);
            return NULL;
        }
        if(var_to_integer(&left) || var_to_integer(&right)) {
            free(left);
            free(right);
            return NULL;
        }
        if(op == TOK_PRODUCT) {
            left->data.intvalue *= right->data.intvalue;
        }
        if(op == TOK_DIVISION) {
            left->data.intvalue /= right->data.intvalue;
        }
        if(op == TOK_MODULO) {
            left->data.intvalue %= right->data.intvalue;
        }
        free(right);
    }
    return left;
}

static variable_t *eval_sum(eval_context_t *ctx) {
    variable_t *left = eval_product(ctx);
    if(!left) return NULL;
    while(ctx->curtok == TOK_PLUS || ctx->curtok == TOK_MINUS) {
        token_t op = ctx->curtok;
        variable_t *right = eval_product(ctx);
        if(!right) {
            free(left);
            return NULL;
        }
        if(var_to_integer(&left) || var_to_integer(&right)) {
            free(left);
            free(right);
            return NULL;
        }
        if(op == TOK_PLUS) {
            left->data.intvalue += right->data.intvalue;
        }
        if(op == TOK_MINUS) {
            left->data.intvalue -= right->data.intvalue;
        }
        free(right);
    }
    return left;
}

static variable_t *evaluate(coyaml_parseinfo_t *info, char *data, int dlen) {
    eval_context_t eval = {
        info: info,
        data: data,
        end: data+dlen,
        token: data,
        next:data,
        curtok: TOK_NONE
        };
    variable_t *res = eval_sum(&eval);
    if(eval.token != eval.end || eval.curtok != TOK_END) {
        if(res) {
            free(res);
        }
        return NULL;
    }
    return res;
}

int coyaml_eval_int(coyaml_parseinfo_t *info,
    char *value, size_t vlen, long *result) {
    if(info->parse_vars && strchr(value, '$')) {
        char *data;
        int dlen;
        if(coyaml_eval_str(info, value, vlen, &data, &dlen)) {
            return -1;
        }
        char *end = parse_long(data, result);
        obstack_free(&info->head->pieces, data);
        SYNTAX_ERROR(end == data + dlen);
        return 0;
    }
    char *end = parse_long(value, result);
    SYNTAX_ERROR(end == value + vlen);
    return 0;
}

int coyaml_eval_float(coyaml_parseinfo_t *info,
    char *value, size_t vlen, double *result) {
    if(info->parse_vars && strchr(value, '$')) {
        char *data;
        int dlen;
        if(coyaml_eval_str(info, value, vlen, &data, &dlen)) {
            return -1;
        }
        char *end = parse_double(data, result);
        obstack_free(&info->head->pieces, data);
        SYNTAX_ERROR(end == data + dlen);
        return 0;
    }
    char *end = parse_double(value, result);
    SYNTAX_ERROR(end == value + vlen);
    return 0;
}

int coyaml_eval_str(coyaml_parseinfo_t *info,
    char *data, size_t dlen, char **result, int *rlen) {
    if(info->parse_vars && strchr(data, '$')) {
        obstack_blank(&info->head->pieces, 0);
        for(char *c = data; *c;) {
            if(*c != '$' && *c != '\\') {
                obstack_1grow(&info->head->pieces, *c);
                ++c;
                continue;
            }
            if(*c == '\\') {
                SYNTAX_ERROR(*++c);
                obstack_1grow(&info->head->pieces, *c);
                ++c;
                continue;
            }
            ++c;
            char *name = c;
            int nlen;
            if(*c == '{') {
                ++c;
                ++name;
                while(*++c && *c != '}');
                nlen = c - name;
                SYNTAX_ERROR(*c++ == '}');
                variable_t *var = evaluate(info, name, nlen);
                SYNTAX_ERROR(var);
                if(var_to_string(&var)) {
                    free(var);
                    SYNTAX_ERROR(0);
                }
                obstack_grow(&info->head->pieces,
                    var->data.str.value, var->data.str.length);
                free(var);
            } else {
                while(*c && (isalnum(*c) || *c == '_')) ++c;
                nlen = c - name;
                if(nlen == 0) {
                    obstack_1grow(&info->head->pieces, '$');
                    continue;
                }
                char *value = find_var(info, name, nlen);
                if(value) {
                    obstack_grow(&info->head->pieces, value, strlen(value));
                } else {
                    COYAML_DEBUG("Not found variable ``%.*s''", nlen, name);
                }
            }
        }
        obstack_1grow(&info->head->pieces, 0);
        *rlen = obstack_object_size(&info->head->pieces)-1;
        *result = obstack_finish(&info->head->pieces);
    } else {
        *result = obstack_copy0(&info->head->pieces, data, dlen);
        *rlen = dlen;
    }
    return 0;
}
