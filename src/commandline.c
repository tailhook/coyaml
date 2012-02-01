#include <coyaml_src.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define VALUE_ERROR(cond, message, ...) if(!(cond)) { \
    fprintf(stderr, "Error parsing option: " message "\n", ##__VA_ARGS__); \
    errno = ECOYAML_VALUE_ERROR; \
    return -1; }

int coyaml_cli_prepare(coyaml_context_t *ctx, int argc, char **argv) {
    int opt;
    while((opt = getopt_long(argc, argv,
        ctx->cmdline->optstr, ctx->cmdline->options, NULL)) != -1) {
        char *pos = strchr(ctx->cmdline->optstr, opt);
        if(pos) {
            opt = ctx->cmdline->optidx[pos - ctx->cmdline->optstr];
        }
        switch(opt) {
            case COYAML_CLI_FILENAME:
                ctx->root_filename = optarg;
                break;
            case COYAML_CLI_DEBUG:
                ctx->debug = TRUE;
                break;
            case COYAML_CLI_VARS:
                ctx->parse_vars = TRUE;
                break;
            case COYAML_CLI_NOVARS:
                ctx->parse_vars = FALSE;
                break;
            case COYAML_CLI_HELP:
                fprintf(stdout, "%s", ctx->cmdline->full_description);
                errno = ECOYAML_CLI_HELP;
                return -1;
            case COYAML_CLI_VAR: {
                char *nend = strchr(optarg, '=');
                if(!nend || nend == optarg) {
                    errno = ECOYAML_CLI_WRONG_OPTION;
                    return -1;
                }
                char name[nend - optarg + 1];
                memcpy(name, optarg, nend - optarg);
                name[nend - optarg] = 0;
                if(coyaml_set_string(ctx, name, nend + 1, strlen(nend+1)))
                    return -1;
                } break;
            case COYAML_CLI_SHOW_VARS:
                ctx->print_vars = TRUE;
                break;
            case '?':
                fprintf(stderr, "%s", ctx->cmdline->usage);
                errno = ECOYAML_CLI_WRONG_OPTION;
                return -1;
            default:
                break;
        }
    }
    optind = 0;
    return 0;
}

int coyaml_cli_parse(coyaml_context_t *ctx, int argc, char **argv) {
    int opt;
    bool do_print = 0;
    bool do_exit = 0;
    coyaml_print_enum print_mode = COYAML_PRINT_FULL;
    while((opt = getopt_long(argc, argv,
        ctx->cmdline->optstr, ctx->cmdline->options, NULL)) != -1) {
        char *pos = strchr(ctx->cmdline->optstr, opt);
        if(pos) {
            opt = ctx->cmdline->optidx[pos - ctx->cmdline->optstr];
        }
        if(opt >= COYAML_CLI_USER) {
            coyaml_option_t *o = \
                &ctx->cmdline->coyaml_options[opt-COYAML_CLI_USER];
            if(o->callback(optarg, o->prop, ctx->target) < 0) {
                fprintf(stderr, "%s", ctx->cmdline->usage);
                errno = ECOYAML_CLI_WRONG_OPTION;
                return -1;
            }
        } else if(opt >= COYAML_CLI_RESERVED) {
            switch(opt) {
            case COYAML_CLI_PRINT:
                if(do_print) {
                    print_mode |= COYAML_PRINT_COMMENTS;
                }
                do_print = TRUE;
                do_exit = TRUE;
                break;
            case COYAML_CLI_CHECK:
            case COYAML_CLI_SHOW_VARS:
                do_exit = TRUE;
                break;
            }
        } else if(opt >= COYAML_CLI_FIRST) {
            // nothing, was used in coyaml_cli_prepare
        } else {
            fprintf(stderr, "%s", ctx->cmdline->usage);
            errno = ECOYAML_CLI_WRONG_OPTION;
            return -1;
        }
    }
    if(optind != argc && !ctx->cmdline->has_arguments) {
        fprintf(stderr, "%s", ctx->cmdline->usage);
        errno = ECOYAML_CLI_WRONG_OPTION;
        return -1;
    }
    if(do_print) {
        if(ctx->cmdline->print_callback(stdout, ctx->target, print_mode) < 0) {
            return -1;
        }
    }
    if(do_exit) {
        errno = ECOYAML_CLI_EXIT;
        return -1;
    }
    return 0;
}

int coyaml_env_parse(coyaml_context_t *ctx) {
    for(coyaml_env_var_t *var = ctx->env_vars; var->name; ++var) {
        char *value = getenv(var->name);
        if(value) {
            if(var->callback(value, var->prop, ctx->target) < 0) {
                fprintf(stderr, "Wrong value for environment variable '%s'",
                    var->name);
                errno = EINVAL;
                return -1;
            }
        }
    }
    return 0;
}

int coyaml_int_o(char *value, coyaml_int_t *def, void *target) {
    char *end;
    int val = strtol(value, (char **)&end, 0);
    VALUE_ERROR(end == value + strlen(value),
        "Option value ``%s'' is not integer", value);
    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %d", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %d", def->min);
    *(int *)(((char *)target)+def->baseoffset) = val;
    return 0;
}

int coyaml_uint_o(char *value, coyaml_uint_t *def, void *target) {
    char *end;
    int val = strtol(value, (char **)&end, 0);
    VALUE_ERROR(end == value + strlen(value),
        "Option value ``%s'' is not integer", value);
    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %d", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %d", def->min);
    *(unsigned *)(((char *)target)+def->baseoffset) = val;
    return 0;
}

int coyaml_float_o(char *value, coyaml_float_t *def, void *target) {
    char *end;
    double val = strtod(value, (char **)&end);
    VALUE_ERROR(end == value + strlen(value),
        "Option value ``%s'' is not float", value);
    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %lf", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %lf", def->min);
    *(double *)(((char *)target)+def->baseoffset) = val;
    return 0;
}

int coyaml_int_incr_o(char *value, coyaml_int_t *def, void *target) {
    ++*(int *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_int_decr_o(char *value, coyaml_int_t *def, void *target) {
    --*(int *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_uint_incr_o(char *value, coyaml_uint_t *def, void *target) {
    ++*(unsigned *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_uint_decr_o(char *value, coyaml_uint_t *def, void *target) {
    --*(unsigned *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_bool_o(char *value, coyaml_bool_t *def, void *target) {
    if(
        !strcasecmp(value, "true")
        || !strcasecmp(value, "y")
        || !strcasecmp(value, "yes")
        || !strcasecmp(value, "on")
        ) {
        *(bool *)(((char *)target)+def->baseoffset) = TRUE;
        return 0;
    } else if(
        !strcasecmp(value, "false")
        || !strcasecmp(value, "n")
        || !strcasecmp(value, "no")
        || !strcasecmp(value, "off")
        ) {
        *(bool *)(((char *)target)+def->baseoffset) = FALSE;
        return 0;
    }
    VALUE_ERROR(FALSE, "Option value ``%s'' is not boolean", value);
}

int coyaml_bool_enable_o(char *value, coyaml_bool_t *def, void *target) {
    *(bool *)(((char *)target)+def->baseoffset) = TRUE;
    return 0;
}
int coyaml_bool_disable_o(char *value, coyaml_bool_t *def, void *target) {
    *(bool *)(((char *)target)+def->baseoffset) = FALSE;
    return 0;
}

int coyaml_file_o(char *value, coyaml_file_t *def, void *target) {
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &((coyaml_head_t *)target)->pieces, value, strlen(value));
    return 0;
}
int coyaml_dir_o(char *value, coyaml_dir_t *def, void *target) {
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &((coyaml_head_t *)target)->pieces, value, strlen(value));
    //TODO: more checks
    return 0;
}
int coyaml_string_o(char *value, coyaml_string_t *def, void *target) {
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &((coyaml_head_t *)target)->pieces, value, strlen(value));
    //TODO: more checks
    return 0;
}
int coyaml_custom_o(char *value, coyaml_custom_t *def, void *target) {
    def->usertype->scalar_fun(NULL, value, def->usertype,
        (void *)(((char *)target)+def->baseoffset));
    return 0;
}
