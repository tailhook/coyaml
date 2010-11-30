#ifndef _H_PARSER
#define _H_PARSER

#include <stdio.h>

typedef struct coyaml_stack_s {
    struct coyaml_stack_s *prev;
    struct coyaml_stack_s *next;
    char *filename;
    char *basedir;
    int basedir_len;
    FILE *file;
    yaml_parser_t parser;
} coyaml_stack_t;

typedef struct coyaml_mapkey_s {
    struct coyaml_mapkey_s *left;
    struct coyaml_mapkey_s *right;
    char name[];
} coyaml_mapkey_t;

typedef struct coyaml_mapmerge_s {
    struct coyaml_mapmerge_s *prev;
    coyaml_mapkey_t *keys;
    int state;
    int level;
    int mergelevel;
} coyaml_mapmerge_t;

#endif //_H_PARSER
