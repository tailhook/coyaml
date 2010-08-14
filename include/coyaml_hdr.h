#ifndef COYAML_HDR_HEADER
#define COYAML_HDR_HEADER

#include <stddef.h>

typedef int bool;

typedef struct coyaml_arrayel_head_s {
    void *next;
} coyaml_arrayel_head_t;

typedef struct coyaml_mappingel_head_s {
    void *next;
} coyaml_mappingel_head_t;


#endif // COYAML_HDR_HEADER
