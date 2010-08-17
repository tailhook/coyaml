#ifndef COYAML_SRC_HEADER
#define COYAML_SRC_HEADER

#include <stddef.h>
#include <coyaml_hdr.h>

#define COYAML_CLI_USER 1000
#define COYAML_CLI_FIRST 500
#define COYAML_CLI_HELP (COYAML_CLI_FIRST)
#define COYAML_CLI_FILENAME (COYAML_CLI_FIRST+1)
#define COYAML_CLI_DEBUG (COYAML_CLI_FIRST+2)
#define COYAML_CLI_RESERVED 600
#define COYAML_CLI_PRINT (COYAML_CLI_RESERVED)
#define COYAML_CLI_CHECK (COYAML_CLI_RESERVED+1)

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#ifndef COYAML_PARSEINFO
typedef struct coyaml_parseinfo_s {
} coyaml_parseinfo_t;
#endif

typedef int (*coyaml_convert_fun)(coyaml_parseinfo_t *info,
    char *value, int value_len,
    void *prop, void *target);
typedef int (*coyaml_state_fun)(coyaml_parseinfo_t *info,
    void *prop, void *target);
typedef int (*coyaml_option_fun)(coyaml_parseinfo_t *info,
    void *prop, void *target);

typedef struct coyaml_transition_s {
    char *symbol;
    coyaml_state_fun callback;
    void *prop;
} coyaml_transition_t;

typedef struct coyaml_tag_s {
    char *tagname;
    int tagvalue;
} coyaml_tag_t;

typedef struct coyaml_group_s {
    int baseoffset;
    int bitmask;
    coyaml_transition_t *transitions;
} coyaml_group_t;

typedef struct coyaml_usertype_s {
    int baseoffset;
    int bitmask;
    struct coyaml_group_s *group;
    coyaml_tag_t *tags;
    coyaml_convert_fun *scalar_fun;
} coyaml_usertype_t;

typedef struct coyaml_custom_s {
    int baseoffset;
    int bitmask;
    struct coyaml_usertype_s *usertype;
} coyaml_custom_t;

typedef struct coyaml_int_s {
    int baseoffset;
    int bitmask;
    int min;
    int max;
} coyaml_int_t;

typedef struct coyaml_uint_s {
    int baseoffset;
    int bitmask;
    unsigned int min;
    unsigned int max;
} coyaml_uint_t;

typedef struct coyaml_bool_s {
    int baseoffset;
} coyaml_bool_t;

typedef struct coyaml_float_s {
    int baseoffset;
    int bitmask;
    double min;
    double max;
} coyaml_float_t;

typedef struct coyaml_array_s {
    int baseoffset;
    int element_size;
    void *element_prop;
    coyaml_state_fun element_callback;
} coyaml_array_t;

typedef struct coyaml_mapping_s {
    int baseoffset;
    int element_size;
    void *key_prop;
    void *value_prop;
    coyaml_state_fun key_callback;
    coyaml_state_fun value_callback;
} coyaml_mapping_t;

typedef struct coyaml_file_s {
    int baseoffset;
    int bitmask;
    bool check_existence;
    bool check_dir;
    bool check_writable;
    char *warn_outside;
} coyaml_file_t;

typedef struct coyaml_dir_s {
    int baseoffset;
    int bitmask;
    bool check_existence;
    bool check_dir;
} coyaml_dir_t;

typedef struct coyaml_string_s {
    int baseoffset;
} coyaml_string_t;

typedef struct coyaml_option_s {
    coyaml_option_fun callback;
    void *prop;
} coyaml_option_t;

int coyaml_CGroup(coyaml_parseinfo_t *info,
    coyaml_group_t *prop, void *target);
int coyaml_CInt(coyaml_parseinfo_t *info,
    coyaml_int_t *prop, void *target);
int coyaml_CUInt(coyaml_parseinfo_t *info,
    coyaml_uint_t *prop, void *target);
int coyaml_Bool(coyaml_parseinfo_t *info,
    coyaml_bool_t *prop, void *target);
int coyaml_CFloat(coyaml_parseinfo_t *info,
    coyaml_float_t *prop, void *target);
int coyaml_CArray(coyaml_parseinfo_t *info,
    coyaml_array_t *prop, void *target);
int coyaml_CMapping(coyaml_parseinfo_t *info,
    coyaml_mapping_t *prop, void *target);
int coyaml_CFile(coyaml_parseinfo_t *info,
    coyaml_file_t *prop, void *target);
int coyaml_CDir(coyaml_parseinfo_t *info,
    coyaml_dir_t *prop, void *target);
int coyaml_CString(coyaml_parseinfo_t *info,
    coyaml_string_t *prop, void *target);
int coyaml_CCustom(coyaml_parseinfo_t *info,
    coyaml_custom_t *prop, void *target);

int coyaml_CInt_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_CUInt_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_Bool_o(char *value, coyaml_bool_t *prop, void *target);
int coyaml_CFloat_o(char *value, coyaml_float_t *prop, void *target);
int coyaml_CFile_o(char *value, coyaml_file_t *prop, void *target);
int coyaml_CDir_o(char *value, coyaml_dir_t *prop, void *target);
int coyaml_CString_o(char *value, coyaml_string_t *prop, void *target);

int coyaml_readfile(char *filename, coyaml_group_t *root,
    void *target, bool debug);

#endif //COYAML_SRC_HEADER
