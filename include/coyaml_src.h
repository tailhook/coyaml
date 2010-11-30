#ifndef COYAML_SRC_HEADER
#define COYAML_SRC_HEADER

#include <stddef.h>
#include <yaml.h>
#include <coyaml_hdr.h>

#define COYAML_CLI_USER 1000
#define COYAML_CLI_FIRST 500
#define COYAML_CLI_HELP (COYAML_CLI_FIRST)
#define COYAML_CLI_FILENAME (COYAML_CLI_FIRST+1)
#define COYAML_CLI_DEBUG (COYAML_CLI_FIRST+2)
#define COYAML_CLI_VARS (COYAML_CLI_FIRST+3)
#define COYAML_CLI_NOVARS (COYAML_CLI_FIRST+4)
#define COYAML_CLI_VAR (COYAML_CLI_FIRST+5)
#define COYAML_CLI_RESERVED 600
#define COYAML_CLI_PRINT (COYAML_CLI_RESERVED)
#define COYAML_CLI_CHECK (COYAML_CLI_RESERVED+1)

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

typedef struct coyaml_anchor_s {
    struct coyaml_anchor_s *next;
    char *name; // It's allocated in obstack first, we don't need to free it
    yaml_event_t events[];
} coyaml_anchor_t;

typedef struct coyaml_parseinfo_s {
    struct coyaml_context_s *context;
    bool debug;
    bool parse_vars;
    void *target;
    yaml_event_t event;
    // Memory allocation structures
    struct coyaml_head_s *head;
    // End of memory allocation
    // Anchors structures
    struct obstack anchors;
    int anchor_level;
    struct coyaml_anchor_s *anchor_first;
    struct coyaml_anchor_s *anchor_last;
    // unpacking
    coyaml_anchor_t *anchor_unpacking;
    int anchor_pos;
    // End anchors
    // Merge structures
    struct obstack mappieces;
    struct coyaml_mapmerge_s *top_map;
    // End merge
    struct coyaml_stack_s *root_file;
    struct coyaml_stack_s *current_file;
} coyaml_parseinfo_t;

struct coyaml_usertype_s;

typedef int (*coyaml_convert_fun)(coyaml_parseinfo_t *info, char *value,
    struct coyaml_usertype_s *prop, void *target);
typedef int (*coyaml_state_fun)(coyaml_parseinfo_t *info,
    void *prop, void *target);
typedef int (*coyaml_option_fun)(char *value, void *prop, void *target);
typedef void (*coyaml_defaults_fun)(void *target);

typedef struct coyaml_transition_s {
    char *symbol;
    coyaml_state_fun callback;
    void *prop;
} coyaml_transition_t;

typedef struct coyaml_tag_s {
    char *tagname;
    int tagvalue;
} coyaml_tag_t;

// `baseoffset` must be first everywhere
typedef struct coyaml_group_s {
    int baseoffset;
    coyaml_transition_t *transitions;
} coyaml_group_t;

typedef struct coyaml_usertype_s {
    int baseoffset;
    int default_tag;
    coyaml_tag_t *tags;
    struct coyaml_group_s *group;
    coyaml_convert_fun scalar_fun;
} coyaml_usertype_t;

typedef struct coyaml_custom_s {
    int baseoffset;
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
    size_t element_size;
    void *element_prop;
    coyaml_defaults_fun element_defaults;
    coyaml_state_fun element_callback;
} coyaml_array_t;

typedef struct coyaml_mapping_s {
    int baseoffset;
    size_t element_size;
    void *key_prop;
    void *value_prop;
    coyaml_state_fun key_callback;
    coyaml_state_fun value_callback;
    coyaml_defaults_fun key_defaults;
    coyaml_defaults_fun value_defaults;
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

int coyaml_int_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_int_incr_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_int_decr_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_uint_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_uint_incr_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_uint_incr_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_bool_o(char *value, coyaml_bool_t *prop, void *target);
int coyaml_float_o(char *value, coyaml_float_t *prop, void *target);
int coyaml_file_o(char *value, coyaml_file_t *prop, void *target);
int coyaml_dir_o(char *value, coyaml_dir_t *prop, void *target);
int coyaml_string_o(char *value, coyaml_string_t *prop, void *target);
int coyaml_custom_o(char *value, coyaml_custom_t *prop, void *target);

int coyaml_readfile(coyaml_context_t *);
coyaml_context_t *coyaml_context_init(coyaml_context_t *ctx);
void coyaml_context_free(coyaml_context_t *ctx);

void coyaml_config_free(void *ptr);

int coyaml_tagged_scalar(coyaml_parseinfo_t *info, char *value,
    struct coyaml_usertype_s *prop, void *target);
int coyaml_parse_tag(coyaml_parseinfo_t *info,
    struct coyaml_usertype_s *prop, int *target);

#endif //COYAML_SRC_HEADER
