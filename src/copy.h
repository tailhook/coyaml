#ifndef _H_COPY
#define _H_COPY

#include <coyaml_src.h>

int coyaml_group_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_usertype_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_custom_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_array_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_mapping_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_int_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_uint_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_bool_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_float_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_dir_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_file_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);
int coyaml_string_copy(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *prop, void *source, void *target);

#endif //_H_COPY
