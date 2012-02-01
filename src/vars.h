#ifndef _H_VARS
#define _H_VARS
#include <coyaml_src.h>

typedef enum coyaml_vartype_enum {
    COYAML_VAR_ANCHOR,
    COYAML_VAR_STRING,
    COYAML_VAR_INTEGER,
} coyaml_vartype_t;

typedef struct coyaml_variable_s {
    struct coyaml_variable_s *left;
    struct coyaml_variable_s *right;
    char *name;
    int name_len;
    coyaml_vartype_t type;
    union coyaml_variable_data {
        struct {
            yaml_event_t *events;
        } anchor;
        struct {
            char *value;
            int value_len;
        } string;
        struct {
            long value;
        } integer;
    } data;
} coyaml_variable_t;

int coyaml_get_string(coyaml_context_t *ctx, char*name, char **data, int *dlen);
int coyaml_print_variables(coyaml_context_t *ctx);
#endif //_H_VARS
