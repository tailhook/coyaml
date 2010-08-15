
#include <yaml.h>
#include <errno.h>
#include <setjmp.h>
#include <obstack.h>

#define COYAML_PARSEINFO

typedef struct coyaml_anchor_s {
    struct coyaml_anchor_s *next;
    char *name; // It's allocated in obstack first, we don't need to free it
    int nevents;
    yaml_event_t events;
} coyaml_anchor_t;

typedef struct coyaml_parseinfo_s {
    int debug;
    char *filename;
    void *target;
    yaml_parser_t parser;
    yaml_event_t event;
    // Memory allocation structures
    struct coyaml_head_s *head;
    jmp_buf memrecover;
    // End of memory allocation
    // Anchors structures
    int anchor_level;
    int anchor_pos;
    int anchor_count;
    int anchor_event_count;
    struct obstack anchors;
    struct coyaml_anchor_s *first;
    struct coyaml_anchor_s *last;
    // End anchors
} coyaml_parseinfo_t;

#include <coyaml_src.h>

#define CHECK(cond) if((cond) < 0) { return -1; }
#define SYNTAX_ERROR(cond) if(!(cond)) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %d column %d\n", \
    info->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, info->event.type); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR2(message, ...) if(TRUE) { \
    fprintf(stderr, "COAYML: Syntax error in config file ``%s'' " \
        "at line %d column %d: " message "\n", \
    info->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define VALUE_ERROR(cond, message, ...) if(!(cond)) { \
    fprintf(stderr, "COYAML: Error at %s:%d[%d]: " message "\n", \
    info->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_VALUE_ERROR; \
    return -1; }
#define COYAML_ASSERT(value) if(!(value)) { \
    fprintf(stderr, "COAYML: Assertion " #value \
        " at " __FILE__ ":%d failed\n", __LINE__); \
    errno = ECOYAML_ASSERTION_ERROR; \
    return -1; }
#define COYAML_DEBUG(message, ...) if(info->debug) { \
    fprintf(stderr, "COYAML: " message "\n", ##__VA_ARGS__); }

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#define obstack_alloc_failed_handler() longjmp(info->recover);

static char *yaml_event_names[] = {
    "YAML_NO_EVENT",
    "YAML_STREAM_START_EVENT",
    "YAML_STREAM_END_EVENT",
    "YAML_DOCUMENT_START_EVENT",
    "YAML_DOCUMENT_END_EVENT",
    "YAML_ALIAS_EVENT",
    "YAML_SCALAR_EVENT",
    "YAML_SEQUENCE_START_EVENT",
    "YAML_SEQUENCE_END_EVENT",
    "YAML_MAPPING_START_EVENT",
    "YAML_MAPPING_END_EVENT"
    };

static char *coyaml_scalar_dup(coyaml_parseinfo_t *info) {
    char *res = obstack_alloc(&info->head->pieces,
        info->event.data.scalar.length+1);
    memcpy(res, info->event.data.scalar.value,
        info->event.data.scalar.length+1);
    return res;
}

static int coyaml_next(coyaml_parseinfo_t *info) {
    // Temporarily without aliases
/*    if(info->anchor_pos >= 0) {*/
/*        memcpy(&info->event, &info->anchor_events[info->anchor_pos++],*/
/*            sizeof(info->event));*/
/*        switch(info->event.type) {*/
/*            case YAML_ALIAS_EVENT:*/
/*                SYNTAX_ERROR("Nested aliases not supported");*/
/*                break;*/
/*            case YAML_MAPPING_START_EVENT:*/
/*            case YAML_SEQUENCE_START_EVENT:*/
/*                if(info->anchor_level >= 0) {*/
/*                    ++ info->anchor_level;*/
/*                }*/
/*            case YAML_SCALAR_EVENT:*/
/*                break;*/
/*            case YAML_MAPPING_END_EVENT:*/
/*            case YAML_SEQUENCE_END_EVENT:*/
/*                if(info->anchor_level >= 0) {*/
/*                    -- info->anchor_level;*/
/*                }*/
/*                break;*/
/*            default:*/
/*                COYAML_ASSERT(info->event.type);*/
/*                break;*/
/*        }*/
/*        if(info->anchor_level == 0) {*/
/*            info->anchor_pos = -1;*/
/*        }*/
/*        return 0;*/
/*    }*/
/*    if(info->event.type && info->anchor_level < 0) {*/
        yaml_event_delete(&info->event);
/*    }*/
/*    if(info->anchor_level == 0) {*/
/*        info->anchor_level = -1;*/
/*    }*/
    COYAML_ASSERT(yaml_parser_parse(&info->parser, &info->event));
    switch(info->event.type) {
        case YAML_ALIAS_EVENT:
            SYNTAX_ERROR2("Aliases not supported yet");
/*            for(int i = 0; i < info->anchor_count; ++i) {*/
/*                if(!strcmp(info->event.data.alias.anchor,*/
/*                    info->anchors[i].name)) {*/
/*                    info->anchor_pos = info->anchors[i].begin;*/
/*                    info->anchor_level = 0;*/
/*                    return coyaml_next(info);*/
/*                }*/
/*            }*/
/*            SYNTAX_ERROR2("Anchor %s not found", info->event.data.alias.anchor);*/
            break;
        case YAML_MAPPING_START_EVENT:
        case YAML_SEQUENCE_START_EVENT:
/*            if(info->anchor_level >= 0) {*/
/*                ++ info->anchor_level;*/
/*            }*/
        case YAML_SCALAR_EVENT:
            if(info->event.data.scalar.anchor) {
                SYNTAX_ERROR2("Anchors not supported yet");
/*                if(info->anchor_count >= ANCHOR_ITEMS_MAX) {*/
/*                    SYNTAX_ERROR2("Too many anchors (max %d)", ANCHOR_ITEMS_MAX);*/
/*                }*/
/*                strncpy(info->anchors[info->anchor_count].name, info->event.data.scalar.anchor, 64);*/
/*                info->anchors[info->anchor_count].name[63] = 0;*/
/*                info->anchors[info->anchor_count].begin = info->anchor_event_count;*/
/*                ++ info->anchor_count;*/
/*                if(info->anchor_level < 0) { // Supporting nested aliases*/
/*                    info->anchor_level = 0;*/
/*                }*/
            }
            break;
        case YAML_MAPPING_END_EVENT:
        case YAML_SEQUENCE_END_EVENT:
/*            if(info->anchor_level >= 0) {*/
/*                -- info->anchor_level;*/
/*            }*/
            break;
        case YAML_STREAM_START_EVENT:
        case YAML_STREAM_END_EVENT:
        case YAML_DOCUMENT_START_EVENT:
        case YAML_DOCUMENT_END_EVENT:
/*            COYAML_ASSERT(info->anchor_level < 0);*/
            break;
        default:
            SYNTAX_ERROR(0);
            break;
    }
/*    if(info->anchor_level >= 0) {*/
/*        memcpy(&info->anchor_events[info->anchor_event_count++],*/
/*            &info->event, sizeof(info->event));*/
/*        if(!info->anchor_level) {*/
/*            info->anchor_level = -1;*/
/*        }*/
/*    }*/
    if(info->event.type == YAML_SCALAR_EVENT) {
        COYAML_DEBUG("Event %s[%d] (%s)",
            yaml_event_names[info->event.type], info->event.type,
            info->event.data.scalar.value);
    } else {
        COYAML_DEBUG("Event %s[%d]",
            yaml_event_names[info->event.type],
            info->event.type);
    }
    return 0;
}

static int coyaml_skip(coyaml_parseinfo_t *info) {
    int level = 0;
    do {
        CHECK(coyaml_next(info));
        switch(info->event.type) {
            case YAML_MAPPING_START_EVENT:
            case YAML_SEQUENCE_START_EVENT:
                ++ level;
                break;
            case YAML_MAPPING_END_EVENT:
            case YAML_SEQUENCE_END_EVENT:
                -- level;
                break;
        }
    } while(level);
    return 0;
}

static int coyaml_root(info, root, config)
coyaml_parseinfo_t *info;
coyaml_group_t *root;
void *config;
{
    CHECK(coyaml_next(info));
    SYNTAX_ERROR(info->event.type == YAML_STREAM_START_EVENT);
    CHECK(coyaml_next(info));
    SYNTAX_ERROR(info->event.type == YAML_DOCUMENT_START_EVENT);
    CHECK(coyaml_next(info));

    CHECK(coyaml_CGroup(info, root, config));

    SYNTAX_ERROR(info->event.type == YAML_DOCUMENT_END_EVENT);
    CHECK(coyaml_next(info));
    SYNTAX_ERROR(info->event.type == YAML_STREAM_END_EVENT);
    return 0;
}


int coyaml_readfile(char *filename, coyaml_group_t *root,
    void *target, bool debug) {
    coyaml_parseinfo_t sinfo;
    sinfo.filename = filename;
    sinfo.debug = debug;
    sinfo.head = target;
    sinfo.target = target;
    sinfo.anchor_level = -1;
    sinfo.anchor_pos = -1;
    sinfo.anchor_count = 0;
    sinfo.anchor_event_count = 0;
    sinfo.event.type = YAML_NO_EVENT;
    obstack_init(&sinfo.head->pieces);

    coyaml_parseinfo_t *info = &sinfo;
    COYAML_DEBUG("Opening filename");
    FILE *file = fopen(filename, "r");
    if(!file) return -1;
    yaml_parser_initialize(&info->parser);
    yaml_parser_set_input_file(&info->parser, file);

    int result = coyaml_root(info, root, target);

    yaml_parser_delete(&info->parser);
    fclose(file);
    COYAML_DEBUG("Done %s", result ? "ERROR" : "OK");
    return result;

}

int coyaml_CGroup(coyaml_parseinfo_t *info, coyaml_group_t *def, void *target) {
    COYAML_DEBUG("Entering CGroup");
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT);
    CHECK(coyaml_next(info));
    while(info->event.type == YAML_SCALAR_EVENT) {
        coyaml_transition_t *tran;
        for(tran = def->transitions;
            tran && tran->symbol; ++tran) {
            if(!strcmp(tran->symbol, info->event.data.scalar.value)) {
                break;
            }
        }
        if(tran && tran->symbol) {
            COYAML_DEBUG("Matched key ``%s''", tran->symbol);
            CHECK(coyaml_next(info));
            CHECK(tran->callback(info, tran->prop, target));
        } else {
            if(info->debug) {
                COYAML_DEBUG("Expected keys:");
                for(coyaml_transition_t *tran = def->transitions;
                    tran && tran->symbol; ++tran) {
                    COYAML_DEBUG("    %s", tran->symbol);
                }
            }
            SYNTAX_ERROR2("Unexpected key ``%s''",
                info->event.data.scalar.value);
        }
    }
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_END_EVENT);
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving CGroup");
    return 0;
}

int coyaml_CInt(coyaml_parseinfo_t *info, coyaml_int_t *def, void *target) {
    COYAML_DEBUG("Entering CInt");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    unsigned char *end;
    int val = strtol(info->event.data.scalar.value, (char **)&end, 0);
    SYNTAX_ERROR(end == info->event.data.scalar.value
        + info->event.data.scalar.length);
    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %d", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %d", def->min);
    *(int *)(((char *)target)+def->baseoffset) = val;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving CInt");
    return 0;
}
int coyaml_CFile(coyaml_parseinfo_t *info, coyaml_file_t *def, void *target) {
    COYAML_DEBUG("Entering CFile");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    // TODO: Implement more checks
    *(char **)(((char *)target)+def->baseoffset) = coyaml_scalar_dup(info);
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving CFile");
    return 0;
}
int coyaml_CDir(coyaml_parseinfo_t *info, coyaml_dir_t *def, void *target) {
    COYAML_DEBUG("Entering CDir");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    // TODO: Implement more checks
    *(char **)(((char *)target)+def->baseoffset) = coyaml_scalar_dup(info);
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving CDir");
    return 0;
}
int coyaml_CString(coyaml_parseinfo_t *info, coyaml_string_t *def, void *target) {
    COYAML_DEBUG("Entering CString");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    *(char **)(((char *)target)+def->baseoffset) = coyaml_scalar_dup(info);
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving CString");
    return 0;
}
int coyaml_CCustom(coyaml_parseinfo_t *info, coyaml_custom_t *def, void *target) {
    COYAML_ASSERT(!"Not implemented");
}
int coyaml_CMapping(coyaml_parseinfo_t *info, coyaml_mapping_t *def, void *target) {
    COYAML_ASSERT(!"Not implemented");
}
int coyaml_CArray(coyaml_parseinfo_t *info, coyaml_array_t *def, void *target) {
}
