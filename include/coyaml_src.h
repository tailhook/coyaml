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
#define COYAML_CLI_SHOW_VARS (COYAML_CLI_RESERVED+2)

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
    // Inheritance marks
    struct coyaml_marks_s *last_mark;
    struct coyaml_marks_s *top_mark;
    // End marks
    struct coyaml_stack_s *root_file;
    struct coyaml_stack_s *current_file;
} coyaml_parseinfo_t;

struct coyaml_usertype_s;
struct coyaml_placeholder_s;
struct coyaml_printctx_s;

typedef enum {
    COYAML_PRINT_SHORT = 0x00,
    COYAML_PRINT_FULL = 0x01,
    COYAML_PRINT_VARS = 0x02,
    COYAML_PRINT_COMMENTS = 0x10 // bitmask
} coyaml_print_enum;

typedef int (*coyaml_convert_fun)(coyaml_parseinfo_t *info, char *value,
    struct coyaml_usertype_s *prop, void *target);
typedef int (*coyaml_state_fun)(coyaml_parseinfo_t *info,
    struct coyaml_placeholder_s *prop, void *target);
typedef int (*coyaml_option_fun)(char *value,
    struct coyaml_placeholder_s *prop, void *target);
typedef int (*coyaml_emit_fun)(struct coyaml_printctx_s *ctx,
    struct coyaml_placeholder_s *prop, void *target);
typedef int (*coyaml_copy_fun)(coyaml_context_t *ctx,
    struct coyaml_placeholder_s *sprop, void *source,
    struct coyaml_placeholder_s *tprop, void *target);
typedef void (*coyaml_defaults_fun)(void *target);

typedef enum {
    COYAML_UNKNOWN,
    COYAML_USER,
    COYAML_GROUP,
    COYAML_CUSTOM,
    COYAML_INT,
    COYAML_UINT,
    COYAML_BOOL,
    COYAML_FLOAT,
    COYAML_ARRAY,
    COYAML_MAPPING,
    COYAML_FILE,
    COYAML_DIR,
    COYAML_STRING,
    COYAML_TYPE_SENTINEL
} coyaml_type_enum;

typedef enum {
    COYAML_INH_NO,
    COYAML_INH_APPEND_DEFAULT,
    COYAML_INH_REPLACE_DEFAULT
} coyaml_inherit_enum;

typedef struct coyaml_valuetype_s {
    coyaml_type_enum ident;
    char *name;
    coyaml_state_fun yaml_parse;
    coyaml_option_fun cli_parse;
    coyaml_emit_fun emit;
    coyaml_copy_fun copy;
} coyaml_valuetype_t;

#define COYAML_PLACEHOLDER \
    coyaml_valuetype_t *type; \
    char *description; \
    int baseoffset; \
    int flagoffset;

typedef struct coyaml_placeholder_s {
    COYAML_PLACEHOLDER
} coyaml_placeholder_t;

typedef struct coyaml_transition_s {
    char *symbol;
    coyaml_placeholder_t *prop;
} coyaml_transition_t;

typedef struct coyaml_tag_s {
    char *tagname;
    int tagvalue;
} coyaml_tag_t;

// `baseoffset` must be first everywhere
typedef struct coyaml_group_s {
    COYAML_PLACEHOLDER
    coyaml_transition_t *transitions;
} coyaml_group_t;
extern coyaml_valuetype_t coyaml_group_type;

typedef struct coyaml_usertype_s {
    COYAML_PLACEHOLDER
    int ident;
    int flagcount;
    int size;
    int default_tag;
    coyaml_tag_t *tags;
    struct coyaml_group_s *group;
    coyaml_convert_fun scalar_fun;
} coyaml_usertype_t;
extern coyaml_valuetype_t coyaml_usertype_type;

typedef struct coyaml_custom_s {
    COYAML_PLACEHOLDER
    struct coyaml_usertype_s *usertype;
} coyaml_custom_t;
extern coyaml_valuetype_t coyaml_custom_type;

typedef struct coyaml_int_s {
    COYAML_PLACEHOLDER
    int bitmask;
    int min;
    int max;
} coyaml_int_t;
extern coyaml_valuetype_t coyaml_int_type;

typedef struct coyaml_uint_s {
    COYAML_PLACEHOLDER
    int bitmask;
    unsigned int min;
    unsigned int max;
} coyaml_uint_t;
extern coyaml_valuetype_t coyaml_uint_type;

typedef struct coyaml_bool_s {
    COYAML_PLACEHOLDER
} coyaml_bool_t;
extern coyaml_valuetype_t coyaml_bool_type;

typedef struct coyaml_float_s {
    COYAML_PLACEHOLDER
    int bitmask;
    double min;
    double max;
} coyaml_float_t;
extern coyaml_valuetype_t coyaml_float_type;

typedef struct coyaml_array_s {
    COYAML_PLACEHOLDER
    int inheritance;
    size_t element_size;
    coyaml_placeholder_t *element_prop;
    coyaml_defaults_fun element_defaults;
} coyaml_array_t;
extern coyaml_valuetype_t coyaml_array_type;

typedef struct coyaml_mapping_s {
    COYAML_PLACEHOLDER
    int inheritance;
    size_t element_size;
    coyaml_placeholder_t *key_prop;
    coyaml_placeholder_t *value_prop;
    coyaml_defaults_fun key_defaults;
    coyaml_defaults_fun value_defaults;
} coyaml_mapping_t;
extern coyaml_valuetype_t coyaml_mapping_type;

typedef struct coyaml_file_s {
    COYAML_PLACEHOLDER
    int bitmask;
    bool check_existence;
    bool check_dir;
    bool check_writable;
    char *warn_outside;
} coyaml_file_t;
extern coyaml_valuetype_t coyaml_file_type;

typedef struct coyaml_dir_s {
    COYAML_PLACEHOLDER
    bool check_existence;
    bool check_dir;
} coyaml_dir_t;
extern coyaml_valuetype_t coyaml_dir_type;

typedef struct coyaml_string_s {
    COYAML_PLACEHOLDER
} coyaml_string_t;
extern coyaml_valuetype_t coyaml_string_type;

typedef struct coyaml_option_s {
    coyaml_option_fun callback;
    void *prop;
} coyaml_option_t;

typedef struct coyaml_env_var_s {
    coyaml_option_fun callback;
    char *name;
    void *prop;
} coyaml_env_var_t;

int coyaml_readfile(coyaml_context_t *);
int coyaml_print(FILE *output, coyaml_group_t *root,
    void *cfg, coyaml_print_enum mode);
coyaml_context_t *coyaml_context_init(coyaml_context_t *ctx);

void coyaml_config_free(void *ptr);

int coyaml_tagged_scalar(coyaml_parseinfo_t *info, char *value,
    struct coyaml_usertype_s *prop, void *target);
int coyaml_parse_tag(coyaml_parseinfo_t *info,
    struct coyaml_usertype_s *prop, int *target);

int coyaml_int_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_int_incr_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_int_decr_o(char *value, coyaml_int_t *prop, void *target);
int coyaml_uint_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_uint_incr_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_uint_decr_o(char *value, coyaml_uint_t *prop, void *target);
int coyaml_bool_o(char *value, coyaml_bool_t *prop, void *target);
int coyaml_bool_enable_o(char *value, coyaml_bool_t *prop, void *target);
int coyaml_bool_disable_o(char *value, coyaml_bool_t *prop, void *target);
int coyaml_float_o(char *value, coyaml_float_t *prop, void *target);
int coyaml_file_o(char *value, coyaml_file_t *prop, void *target);
int coyaml_dir_o(char *value, coyaml_dir_t *prop, void *target);
int coyaml_string_o(char *value, coyaml_string_t *prop, void *target);
int coyaml_custom_o(char *value, coyaml_custom_t *prop, void *target);

#endif //COYAML_SRC_HEADER
