
#include "coyaml_src.h"
#include "parser.h"
#include "emitter.h"
#include "copy.h"

coyaml_valuetype_t coyaml_group_type = {
    ident: COYAML_GROUP,
    name: "group",
    yaml_parse: (coyaml_state_fun)coyaml_group,
    cli_parse: (coyaml_option_fun)NULL,
    emit: (coyaml_emit_fun)coyaml_group_emit,
    copy: NULL
    }; 
    
coyaml_valuetype_t coyaml_usertype_type = {
    ident: COYAML_USER,
    name: "usertype",
    yaml_parse: (coyaml_state_fun)coyaml_usertype,
    cli_parse: (coyaml_option_fun)NULL,
    emit: (coyaml_emit_fun)coyaml_usertype_emit,
    copy: NULL
    };
    
coyaml_valuetype_t coyaml_custom_type = {
    ident: COYAML_CUSTOM,
    name: "custom",
    yaml_parse: (coyaml_state_fun)coyaml_custom,
    cli_parse: (coyaml_option_fun)NULL,
    emit: (coyaml_emit_fun)coyaml_custom_emit,
    copy: (coyaml_copy_fun)coyaml_custom_copy
    };
    
coyaml_valuetype_t coyaml_int_type = {
    ident: COYAML_INT,
    name: "int",
    yaml_parse: (coyaml_state_fun)coyaml_int,
    cli_parse: (coyaml_option_fun)coyaml_int_o,
    emit: (coyaml_emit_fun)coyaml_int_emit,
    copy: (coyaml_copy_fun)coyaml_int_copy
};

coyaml_valuetype_t coyaml_uint_type = {
    ident: COYAML_UINT,
    name: "uint",
    yaml_parse: (coyaml_state_fun)coyaml_uint,
    cli_parse: (coyaml_option_fun)coyaml_uint_o,
    emit: (coyaml_emit_fun)coyaml_uint_emit,
    copy: (coyaml_copy_fun)coyaml_uint_copy
};

coyaml_valuetype_t coyaml_bool_type = {
    ident: COYAML_BOOL,
    name: "bool",
    yaml_parse: (coyaml_state_fun)coyaml_bool,
    cli_parse: (coyaml_option_fun)coyaml_bool_o,
    emit: (coyaml_emit_fun)coyaml_bool_emit,
    copy: (coyaml_copy_fun)coyaml_bool_copy
};

coyaml_valuetype_t coyaml_float_type = {
    ident: COYAML_FLOAT,
    name: "float",
    yaml_parse: (coyaml_state_fun)coyaml_float,
    cli_parse: (coyaml_option_fun)coyaml_float_o,
    emit: (coyaml_emit_fun)coyaml_float_emit,
    copy: (coyaml_copy_fun)coyaml_float_copy
};

coyaml_valuetype_t coyaml_array_type = {
    ident: COYAML_ARRAY,
    name: "array",
    yaml_parse: (coyaml_state_fun)coyaml_array,
    cli_parse: (coyaml_option_fun)NULL,
    emit: (coyaml_emit_fun)coyaml_array_emit,
    copy: (coyaml_copy_fun)coyaml_array_copy
};

coyaml_valuetype_t coyaml_mapping_type = {
    ident: COYAML_MAPPING,
    name: "mapping",
    yaml_parse: (coyaml_state_fun)coyaml_mapping,
    cli_parse: (coyaml_option_fun)NULL,
    emit: (coyaml_emit_fun)coyaml_mapping_emit,
    copy: (coyaml_copy_fun)coyaml_mapping_copy
};

coyaml_valuetype_t coyaml_file_type = {
    ident: COYAML_FILE,
    name: "file",
    yaml_parse: (coyaml_state_fun)coyaml_file,
    cli_parse: (coyaml_option_fun)coyaml_file_o,
    emit: (coyaml_emit_fun)coyaml_file_emit,
    copy: (coyaml_copy_fun)coyaml_file_copy
};

coyaml_valuetype_t coyaml_dir_type = {
    ident: COYAML_DIR,
    name: "dir",
    yaml_parse: (coyaml_state_fun)coyaml_dir,
    cli_parse: (coyaml_option_fun)coyaml_dir_o,
    emit: (coyaml_emit_fun)coyaml_dir_emit,
    copy: (coyaml_copy_fun)coyaml_dir_copy
};

coyaml_valuetype_t coyaml_string_type = {
    ident: COYAML_STRING,
    name: "string",
    yaml_parse: (coyaml_state_fun)coyaml_string,
    cli_parse: (coyaml_option_fun)coyaml_string_o,
    emit: (coyaml_emit_fun)coyaml_string_emit,
    copy: (coyaml_copy_fun)coyaml_string_copy
};

