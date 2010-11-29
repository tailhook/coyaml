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

#endif //_H_PARSER