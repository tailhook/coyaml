#ifndef _H_EMITTER
#define _H_EMITTER

#include <coyaml_src.h>

typedef struct coyaml_printctx_s {
    FILE *file;
    yaml_emitter_t emitter;
    void *config;
    coyaml_group_t *root;
    bool comments;
    bool defaults;
} coyaml_printctx_t;

int coyaml_group_emit(coyaml_printctx_t *emitter,
    coyaml_group_t *prop, void *target);
int coyaml_usertype_emit(coyaml_printctx_t *emitter,
    struct coyaml_usertype_s *prop, void *target);
int coyaml_custom_emit(coyaml_printctx_t *emitter,
    struct coyaml_custom_s *prop, void *target);
int coyaml_array_emit(coyaml_printctx_t *emitter,
    struct coyaml_array_s *prop, void *target);
int coyaml_mapping_emit(coyaml_printctx_t *emitter,
    struct coyaml_mapping_s *prop, void *target);
int coyaml_int_emit(coyaml_printctx_t *emitter,
    struct coyaml_placeholder_s *prop, void *target);
int coyaml_uint_emit(coyaml_printctx_t *emitter,
    struct coyaml_placeholder_s *prop, void *target);
int coyaml_bool_emit(coyaml_printctx_t *emitter,
    struct coyaml_placeholder_s *prop, void *target);
int coyaml_float_emit(coyaml_printctx_t *emitter,
    struct coyaml_placeholder_s *prop, void *target);
int coyaml_dir_emit(coyaml_printctx_t *emitter,
    struct coyaml_dir_s *prop, void *target);
int coyaml_file_emit(coyaml_printctx_t *emitter,
    struct coyaml_file_s *prop, void *target);
int coyaml_string_emit(coyaml_printctx_t *emitter,
    struct coyaml_string_s *prop, void *target);

#endif //_H_EMITTER
