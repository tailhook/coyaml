#include <eval.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "parser.h"
#include "vars.h"

#define SYNTAX_ERROR(cond) if(!(cond)) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %d column %d\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, info->event.type); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR2_NULL(message, ...) if(TRUE) { \
    fprintf(stderr, "COAYML: Syntax error in config file ``%s'' " \
        "at line %d column %d: " message "\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return NULL; }
#define COYAML_DEBUG(message, ...) if(info->debug) { \
    fprintf(stderr, "COYAML: " message "\n", ##__VA_ARGS__); }

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
            return a->events[0].data.scalar.value;
        }
    }
    return NULL;
}

int coyaml_eval_int(coyaml_parseinfo_t *info,
    char *value, size_t vlen, long *result) {
    char *end;
    int val = strtol(value, (char **)&end, 0);
    if(*end) {
        for(struct unit_s *unit = units; unit->unit; ++unit) {
            if(!strcmp(end, unit->unit)) {
                val *= unit->value;
                end += strlen(unit->unit);
                break;
            }
        }
    }
    if(end != value + vlen) {
        errno = EINVAL;
        return -1;
    }
    *result = val;
    return 0;
}

int coyaml_eval_float(coyaml_parseinfo_t *info,
    char *value, size_t vlen, double *result) {
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
    if(end != value + vlen) {
        errno = EINVAL;
        return -1;
    }
    *result = val;
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
                obstack_1grow(&info->head->pieces, *c);
                SYNTAX_ERROR(*++c);
                obstack_1grow(&info->head->pieces, *c);
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
            } else {
                while(*++c && (isalnum(*c) || *c == '_'));
                nlen = c - name;
            }
            char *value = find_var(info, name, nlen);
            if(value) {
                obstack_grow(&info->head->pieces, value, strlen(value));
            } else {
                COYAML_DEBUG("Not found variable ``%.*s''", nlen, name);
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
