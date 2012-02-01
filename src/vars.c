#include <coyaml_src.h>
#include "vars.h"
#include <assert.h>

static int find_value(coyaml_variable_t *var, char *name,
    coyaml_variable_t **node) {
    coyaml_variable_t *parent;
    coyaml_variable_t *cur = var;
    while(TRUE) {
        int res = strcmp(name, cur->name);
        if(!res) {
            *node = cur;
            return 0;
        }
        if(res < 0) {
            parent = cur;
            cur = cur->left;
            if(!cur) {
                *node = parent;
                return -1;
            }
        } else { // res > 0
            parent = cur;
            cur = cur->right;
            if(!cur) {
                *node = parent;
                return 1;
            }
        }
    }
    abort();
}

static coyaml_variable_t *new_variable(coyaml_context_t *ctx, char *name) {
    coyaml_variable_t *var = obstack_alloc(&ctx->pieces,
        sizeof(coyaml_variable_t) + strlen(name) + 1);
    var->left = NULL;
    var->right = NULL;
    var->name = ((char *)var) + sizeof(coyaml_variable_t);
    var->name_len = strlen(name);
    memcpy(var->name, name, var->name_len);
    var->name[var->name_len] = 0;
    return var;
}

static coyaml_variable_t *find_and_set(coyaml_context_t *ctx, char *name) {
    coyaml_variable_t *var;
    if(!ctx->variables) {
        ctx->variables = var = new_variable(ctx, name);
    } else {
        coyaml_variable_t *node;
        int rel = find_value(ctx->variables, name, &node);
        if(rel == 1) {
            var = node->right = new_variable(ctx, name);
        } else if(rel == -1) {
            var = node->left = new_variable(ctx, name);
        } else {
            assert(0);
        }
    }
    return var;
}

int coyaml_set_string(coyaml_context_t *ctx,
    char *name, char *data, int dlen){
    coyaml_variable_t *var = find_and_set(ctx, name);
    if(!var) return -1;
    var->type = COYAML_VAR_STRING;
    var->data.string.value = obstack_copy0(&ctx->pieces, data, dlen);
    var->data.string.value_len = dlen;
    return 0;
}

int coyaml_set_integer(coyaml_context_t *ctx, char *name, long value) {
    coyaml_variable_t *var = find_and_set(ctx, name);
    if(!var) return -1;
    var->type = COYAML_VAR_INTEGER;
    var->data.integer.value = value;
    return 0;
}

int coyaml_get_string(coyaml_context_t *ctx, char *name,
    char **data, int *dlen) {
    coyaml_variable_t *var;
    if(!ctx->variables) return -1;
    int val = find_value(ctx->variables, name, &var);
    if(!val) {
        switch(var->type) {
            case COYAML_VAR_STRING:
                *data = var->data.string.value;
                *dlen = var->data.string.value_len;
                return 0;
            case COYAML_VAR_INTEGER:
                *data = obstack_alloc(&ctx->pieces, 24);
                *dlen = sprintf(*data, "%ld", var->data.integer.value);
                return 0;
            default:
                return -1; // maybe will fix this
        };
    }
    return -1;
}

static void print_var(coyaml_variable_t *var) {
    if(var->left) {
        print_var(var->left);
    }
    switch(var->type) {
        case COYAML_VAR_STRING:
            printf("%s=%.*s\n", var->name,
                var->data.string.value_len,
                var->data.string.value);
            break;
        case COYAML_VAR_INTEGER:
            printf("%s=%ld\n", var->name, var->data.integer.value);
            break;
        default: break;
    };
    if(var->right) {
        print_var(var->right);
    }
}

int coyaml_print_variables(coyaml_context_t *ctx) {
    print_var(ctx->variables);
    for(coyaml_anchor_t *a = ctx->parseinfo->anchor_first; a; a = a->next) {
        if(a->events[0].type == YAML_SCALAR_EVENT) {
            printf("%s=%.*s\n", a->name,
                (int)a->events[0].data.scalar.length,
                (char *)a->events[0].data.scalar.value);
        }
    }
    return 0;
}
