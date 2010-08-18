#include <coyaml_src.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define VALUE_ERROR(cond, message, ...) if(!(cond)) { \
    fprintf(stderr, "Error parsing option: " message "\n", ##__VA_ARGS__); \
    errno = ECOYAML_VALUE_ERROR; \
    return -1; }

int coyaml_cli_prepare(int argc, char **argv, coyaml_cmdline_t *cmdline) {
    int opt;
    int old_optind = optind;
    while((opt = getopt_long(argc, argv,
        cmdline->optstr, cmdline->options, NULL)) != -1) {
        char *pos = strchr(cmdline->optstr, opt);
        if(pos) {
            opt = cmdline->optidx[pos - cmdline->optstr];
        }
        switch(opt) {
            case COYAML_CLI_FILENAME:
                cmdline->filename = optarg;
                break;
            case COYAML_CLI_DEBUG:
                cmdline->debug = TRUE;
                break;
            case COYAML_CLI_HELP:
                fprintf(stdout, cmdline->full_description);
                errno = ECOYAML_CLI_HELP;
                return -1;
            case '?':
                fprintf(stderr, cmdline->usage);
                errno = ECOYAML_CLI_WRONG_OPTION;
                return -1;
            default:
                break;
        }
    }
    optind = old_optind;
    return 0;
}

int coyaml_cli_parse(int argc, char **argv, coyaml_cmdline_t *cmdline,
    void *target) {
    int opt;
    int old_optind = optind;
    bool do_print = 0;
    bool do_exit = 0;
    while((opt = getopt_long(argc, argv,
        cmdline->optstr, cmdline->options, NULL)) != -1) {
        char *pos = strchr(cmdline->optstr, opt);
        if(pos) {
            opt = cmdline->optidx[pos - cmdline->optstr];
        }
        if(opt >= COYAML_CLI_USER) {
            coyaml_option_t *o = &cmdline->coyaml_options[opt-COYAML_CLI_USER];
            if(o->callback(optarg, o->prop, target) < 0) {
                fprintf(stderr, cmdline->usage);
                errno = ECOYAML_CLI_WRONG_OPTION;
                return -1;
            }
        } else if(opt >= COYAML_CLI_RESERVED) {
            switch(opt) {
            case COYAML_CLI_PRINT:
                do_print = TRUE;
                do_exit = TRUE;
                break;
            case COYAML_CLI_CHECK:
                do_exit = TRUE;
                break;
            }
        } else if(opt >= COYAML_CLI_FIRST) {
            // nothing, was used in coyaml_cli_prepare
        } else {
            fprintf(stderr, cmdline->usage);
            errno = ECOYAML_CLI_WRONG_OPTION;
            return -1;
        }
    }
    optind = old_optind;
    if(do_print) {
        cmdline->print_callback(stdout, target);
    }
    if(do_exit) {
        errno = ECOYAML_CLI_EXIT;
        return -1;
    }
    return 0;
}

int coyaml_CInt_o(char *value, coyaml_int_t *def, void *target) {
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
int coyaml_CUInt_o(char *value, coyaml_uint_t *def, void *target) {
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

int coyaml_CInt_incr_o(char *value, coyaml_int_t *def, void *target) {
    ++*(int *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_CInt_decr_o(char *value, coyaml_int_t *def, void *target) {
    --*(int *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_CUInt_incr_o(char *value, coyaml_uint_t *def, void *target) {
    ++*(unsigned *)(((char *)target)+def->baseoffset);
    return 0;
}
int coyaml_CUInt_decr_o(char *value, coyaml_uint_t *def, void *target) {
    --*(unsigned *)(((char *)target)+def->baseoffset);
    return 0;
}

int coyaml_CFile_o(char *value, coyaml_file_t *def, void *target) {
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &((coyaml_head_t *)target)->pieces, value, strlen(value));
}
int coyaml_CDir_o(char *value, coyaml_dir_t *def, void *target) {
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &((coyaml_head_t *)target)->pieces, value, strlen(value));
    //TODO: more checks
}
int coyaml_CString_o(char *value, coyaml_string_t *def, void *target) {
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &((coyaml_head_t *)target)->pieces, value, strlen(value));
    //TODO: more checks
}
