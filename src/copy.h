#ifndef _H_COPY
#define _H_COPY

#include <coyaml_src.h>
#include "parser.h"

int coyaml_copier(coyaml_context_t *ctx, coyaml_usertype_t *def,
    coyaml_marks_t *source, coyaml_marks_t *target);

int coyaml_custom_copy(coyaml_context_t *ctx,
    struct coyaml_custom_s *sprop, void *source,
    struct coyaml_custom_s *tprop, void *target);
int coyaml_array_copy(coyaml_context_t *ctx,
    struct coyaml_array_s *sprop, void *source,
    struct coyaml_array_s *tprop, void *target);
int coyaml_mapping_copy(coyaml_context_t *ctx,
    struct coyaml_mapping_s *sprop, void *source,
    struct coyaml_mapping_s *tprop, void *target);
int coyaml_int_copy(coyaml_context_t *ctx,
    struct coyaml_int_s *sprop, void *source,
    struct coyaml_int_s *tprop, void *target);
int coyaml_uint_copy(coyaml_context_t *ctx,
    struct coyaml_uint_s *sprop, void *source,
    struct coyaml_uint_s *tprop, void *target);
int coyaml_bool_copy(coyaml_context_t *ctx,
    struct coyaml_bool_s *sprop, void *source,
    struct coyaml_bool_s *tprop, void *target);
int coyaml_float_copy(coyaml_context_t *ctx,
    struct coyaml_float_s *sprop, void *source,
    struct coyaml_float_s *tprop, void *target);
int coyaml_dir_copy(coyaml_context_t *ctx,
    struct coyaml_dir_s *sprop, void *source,
    struct coyaml_dir_s *tprop, void *target);
int coyaml_file_copy(coyaml_context_t *ctx,
    struct coyaml_file_s *sprop, void *source,
    struct coyaml_file_s *tprop, void *target);
int coyaml_string_copy(coyaml_context_t *ctx,
    struct coyaml_string_s *sprop, void *source,
    struct coyaml_string_s *tprop, void *target);

#endif //_H_COPY
