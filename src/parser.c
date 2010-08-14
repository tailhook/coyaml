
#include <yaml.h>
#include <errno.h>

#define COYAML_PARSEINFO
#define ANCHOR_ITEMS_MAX 64
#define ANCHOR_EVENTS_MAX 1024
typedef struct coyaml_parseinfo_s {
    int debug;
    char *filename;
    struct config_head_s *head;
    void *target;
    yaml_parser_t parser;
    yaml_event_t event;
    int anchor_level;
    int anchor_pos;
    int anchor_count;
    int anchor_event_count;
    yaml_event_t anchor_events[ANCHOR_EVENTS_MAX];
    struct {
        char name[64];
        int begin;
    } anchors[ANCHOR_ITEMS_MAX];
} coyaml_parseinfo_t;

#include <coyaml_src.h>

#define CHECK(cond) if((cond) < 0) { return -1; }
#define SYNTAX_ERROR(cond) if(!(cond)) { \
    if(info->debug) { fprintf(stderr, "%s:%s: ", __FILE__, __LINE__); } \
    fprintf(stderr, "Syntax error in config file ``%s'' " \
        "at line %d column %d\n", \
    __FILE__, __LINE__, \
    info->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, info->event.type); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR2(message, ...) if(TRUE) { \
    if(info->debug) { fprintf(stderr, "%s:%s: ", __FILE__, __LINE__); } \
    fprintf(stderr, "Syntax error in config file ``%s'' " \
        "at line %d column %d: " message "\n", \
    __FILE__, __LINE__, \
    info->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, info->event.type, ##__VA_ARGS__); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define VALUE_ERROR(cond, message, ...) if(!(cond)) { \
    if(info->debug) { fprintf(stderr, "%s:%s: ", __FILE__, __LINE__); } \
    fprintf(stderr, "Error at %s:%d[%d]: " message "\n", info->config_filename,\
    info->event.start_mark.line+1,info->event.start_mark.column,##__VA_ARGS__);\
    errno = ECOYAML_VALUE_ERROR; \
    return -1; }
#define COYAML_ASSERT(value) if(!(value)) { \
    fprintf(stderr, "Assertion " #value \
        " at " __FILE__ ":%d failed\n", __LINE__); \
    errno = ECOYAML_ASSERTION_ERROR; \
    return -1; }
#define COYAML_DEBUG(message, ...) if(info->debug) { \
    fprintf(stderr, "COYAML: " message "\n", ##__VA_ARGS__); }

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

static int coyaml_next(coyaml_parseinfo_t *info) {
    if(info->anchor_pos >= 0) {
        memcpy(&info->event, &info->anchor_events[info->anchor_pos++],
            sizeof(info->event));
        switch(info->event.type) {
            case YAML_ALIAS_EVENT:
                SYNTAX_ERROR("Nested aliases not supported");
                break;
            case YAML_MAPPING_START_EVENT:
            case YAML_SEQUENCE_START_EVENT:
                if(info->anchor_level >= 0) {
                    ++ info->anchor_level;
                }
            case YAML_SCALAR_EVENT:
                break;
            case YAML_MAPPING_END_EVENT:
            case YAML_SEQUENCE_END_EVENT:
                if(info->anchor_level >= 0) {
                    -- info->anchor_level;
                }
                break;
            default:
                COYAML_ASSERT(info->event.type);
                break;
        }
        if(info->anchor_level == 0) {
            info->anchor_pos = -1;
        }
        return 0;
    }
    if(info->event.type && info->anchor_level < 0) {
        yaml_event_delete(&info->event);
    }
    if(info->anchor_level == 0) {
        info->anchor_level = -1;
    }
    COYAML_ASSERT(yaml_parser_parse(&info->parser, &info->event));
    switch(info->event.type) {
        case YAML_ALIAS_EVENT:
            for(int i = 0; i < info->anchor_count; ++i) {
                if(!strcmp(info->event.data.alias.anchor,
                    info->anchors[i].name)) {
                    info->anchor_pos = info->anchors[i].begin;
                    info->anchor_level = 0;
                    return coyaml_next(info);
                }
            }
            SYNTAX_ERROR2("Anchor %s not found", info->event.data.alias.anchor);
            break;
        case YAML_MAPPING_START_EVENT:
        case YAML_SEQUENCE_START_EVENT:
            if(info->anchor_level >= 0) {
                ++ info->anchor_level;
            }
        case YAML_SCALAR_EVENT:
            if(info->event.data.scalar.anchor) {
                if(info->anchor_count >= ANCHOR_ITEMS_MAX) {
                    SYNTAX_ERROR2("Too many anchors (max %d)", ANCHOR_ITEMS_MAX);
                }
                strncpy(info->anchors[info->anchor_count].name, info->event.data.scalar.anchor, 64);
                info->anchors[info->anchor_count].name[63] = 0;
                info->anchors[info->anchor_count].begin = info->anchor_event_count;
                ++ info->anchor_count;
                if(info->anchor_level < 0) { // Supporting nested aliases
                    info->anchor_level = 0;
                }
            }
            break;
        case YAML_MAPPING_END_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            if(info->anchor_level >= 0) {
                -- info->anchor_level;
            }
            break;
        case YAML_STREAM_START_EVENT:
        case YAML_STREAM_END_EVENT:
        case YAML_DOCUMENT_START_EVENT:
        case YAML_DOCUMENT_END_EVENT:
            COYAML_ASSERT(info->anchor_level < 0);
            break;
        default:
            SYNTAX_ERROR(0);
            break;
    }
    if(info->anchor_level >= 0) {
        memcpy(&info->anchor_events[info->anchor_event_count++],
            &info->event, sizeof(info->event));
        if(!info->anchor_level) {
            info->anchor_level = -1;
        }
    }
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

    CHECK(coyaml_CGroup(info, root, config));

    CHECK(coyaml_next(info));
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
    sinfo.target = target;
    sinfo.anchor_level = -1;
    sinfo.anchor_pos = -1;
    sinfo.anchor_count = 0;
    sinfo.anchor_event_count = 0;
    sinfo.event.type = YAML_NO_EVENT;

    coyaml_parseinfo_t *info = &sinfo;
    COYAML_DEBUG("Opening filename");
    FILE *file = fopen(filename, "r");
    if(!file) return -1;
    yaml_parser_initialize(&info->parser);
    yaml_parser_set_input_file(&info->parser, file);

    int result = coyaml_root(info);

    yaml_parser_delete(&info->parser);
    fclose(file);
    COYAML_DEBUG("Done");
    return result;

}
