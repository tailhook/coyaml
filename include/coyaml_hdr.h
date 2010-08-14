#ifndef COYAML_HDR_HEADER
#define COYAML_HDR_HEADER

#include <stddef.h>

typedef int bool;
#define FALSE 0
#define TRUE 1
#define ECOYAML_MIN 67575558 // Some random value
#define ECOYAML_MAX (ECOYAML_MIN+2)
#define ECOYAML_SYNTAX_ERROR ECOYAML_MIN
#define ECOYAML_VALUE_ERROR (ECOYAML_MIN+1)
#define ECOYAML_ASSERTION_ERROR (ECOYAML_MIN+2)

typedef struct coyaml_arrayel_head_s {
    void *next;
} coyaml_arrayel_head_t;

typedef struct coyaml_mappingel_head_s {
    void *next;
} coyaml_mappingel_head_t;


#endif // COYAML_HDR_HEADER
