#ifndef _H_EVAL
#define _H_EVAL

#include "coyaml_src.h"

int coyaml_eval_int(coyaml_parseinfo_t *info,
    char *value, size_t vlen, long *result);
int coyaml_eval_float(coyaml_parseinfo_t *info,
    char *value, size_t vlen, double *result);
int coyaml_eval_str(coyaml_parseinfo_t *info,
    char *value, size_t vlen, char **result, int *rlen);

#endif // _H_EVAL
