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

typedef void (*coyaml_print_fun)(FILE *out, char *prefix, void *cfg);

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
    char *filename;
    bool debug;
    char *usage;
    char *full_description;
    char *optstr;
    int *optidx;
    struct option *options;
    struct coyaml_option_s *coyaml_options;
    coyaml_print_fun print_callback;
} coyaml_cmdline_t;

int coyaml_cli_prepare(int argc, char **argv, coyaml_cmdline_t *);
int coyaml_cli_parse(int argc, char **argv, coyaml_cmdline_t *, void *target);

#endif // COYAML_HDR_HEADER
