#ifndef _H_PARSER
#define _H_PARSER

#include <stdio.h>

// Files' stack
typedef struct coyaml_stack_s {
    struct coyaml_stack_s *prev;
    struct coyaml_stack_s *next;
    char *filename;
    char *basedir;
    int basedir_len;
    FILE *file;
    yaml_parser_t parser;
} coyaml_stack_t;

// Tree of mapping keys, determining their uniqueness
typedef struct coyaml_mapkey_s {
    struct coyaml_mapkey_s *left;
    struct coyaml_mapkey_s *right;
    char name[];
} coyaml_mapkey_t;

// Stack of map merging, for `<<` operator
typedef struct coyaml_mapmerge_s {
    struct coyaml_mapmerge_s *prev;
    coyaml_mapkey_t *keys;
    int height;
    int state;
    int level;
    int mergelevel;
    int mergelists;
} coyaml_mapmerge_t;

// Marks of filled fields for each structure, used for inheritance
typedef struct coyaml_marks_s {
    struct coyaml_marks_s *parent;
    struct coyaml_marks_s *prev;
    coyaml_usertype_t *prop;
    void *object;
    int type;
    char filled[];
} coyaml_marks_t;

int coyaml_group(coyaml_parseinfo_t *info,
    coyaml_group_t *prop, void *target);
int coyaml_int(coyaml_parseinfo_t *info,
    coyaml_int_t *prop, void *target);
int coyaml_uint(coyaml_parseinfo_t *info,
    coyaml_uint_t *prop, void *target);
int coyaml_bool(coyaml_parseinfo_t *info,
    coyaml_bool_t *prop, void *target);
int coyaml_float(coyaml_parseinfo_t *info,
    coyaml_float_t *prop, void *target);
int coyaml_array(coyaml_parseinfo_t *info,
    coyaml_array_t *prop, void *target);
int coyaml_mapping(coyaml_parseinfo_t *info,
    coyaml_mapping_t *prop, void *target);
int coyaml_file(coyaml_parseinfo_t *info,
    coyaml_file_t *prop, void *target);
int coyaml_dir(coyaml_parseinfo_t *info,
    coyaml_dir_t *prop, void *target);
int coyaml_string(coyaml_parseinfo_t *info,
    coyaml_string_t *prop, void *target);
int coyaml_custom(coyaml_parseinfo_t *info,
    coyaml_custom_t *prop, void *target);
int coyaml_usertype(coyaml_parseinfo_t *info,
    coyaml_usertype_t *prop, void *target);

#endif //_H_PARSER
