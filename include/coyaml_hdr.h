#ifndef COYAML_HDR_HEADER
#define COYAML_HDR_HEADER

#include <stddef.h>
#include <obstack.h>
#include <getopt.h>
#include <stdio.h>

typedef int bool;
#define FALSE 0
#define TRUE 1
#define ECOYAML_MIN 67575558 // Some random value
#define ECOYAML_SYNTAX_ERROR ECOYAML_MIN
#define ECOYAML_VALUE_ERROR (ECOYAML_MIN+1)
#define ECOYAML_ASSERTION_ERROR (ECOYAML_MIN+2)
#define ECOYAML_CLI_WRONG_OPTION (ECOYAML_MIN+3)
#define ECOYAML_CLI_EXIT (ECOYAML_MIN+4)
#define ECOYAML_CLI_HELP (ECOYAML_MIN+5)
#define ECOYAML_MAX (ECOYAML_MIN+5)

struct coyaml_group_s;

typedef int (*coyaml_print_fun)(FILE *out, void *cfg, int mode);

typedef struct coyaml_head_s {
    struct obstack pieces;
    bool free_object;
} coyaml_head_t;

typedef struct coyaml_arrayel_head_s {
    void *next;
} coyaml_arrayel_head_t;

typedef struct coyaml_mappingel_head_s {
    void *next;
} coyaml_mappingel_head_t;

typedef struct coyaml_cmdline_s {
    char *usage;
    char *full_description;
    char *optstr;
    int *optidx;
    bool has_arguments;
    struct option *options;
    struct coyaml_option_s *coyaml_options;
    coyaml_print_fun print_callback;
} coyaml_cmdline_t;

typedef struct coyaml_context_s {
    bool debug;
    bool parse_vars;
    bool print_vars;
    struct coyaml_head_s *target;
    char *program_name;
    coyaml_cmdline_t *cmdline;
    struct coyaml_group_s *root_group;
    struct coyaml_env_var_s *env_vars;
    char *root_filename;
    bool free_object;

    struct obstack pieces;
    struct coyaml_variable_s *variables;
    struct coyaml_parseinfo_s *parseinfo;
} coyaml_context_t;

int coyaml_readfile(coyaml_context_t *ctx);
int coyaml_cli_prepare(coyaml_context_t *, int argc, char **argv);
int coyaml_cli_parse(coyaml_context_t *, int argc, char **argv);
int coyaml_env_parse(coyaml_context_t *ctx);
void coyaml_context_free(coyaml_context_t *ctx);

int coyaml_set_string(coyaml_context_t *, char *name, char *data, int dlen);
int coyaml_set_integer(coyaml_context_t *ctx, char *name, long value);

void coyaml_cli_prepare_or_exit(coyaml_context_t *ctx, int argc, char **argv);
void coyaml_readfile_or_exit(coyaml_context_t *ctx);
void coyaml_env_parse_or_exit(coyaml_context_t *ctx);
void coyaml_cli_parse_or_exit(coyaml_context_t *ctx, int argc, char **argv);

#endif // COYAML_HDR_HEADER
