
#include <yaml.h>
#include <errno.h>
#include <setjmp.h>
#include <obstack.h>
#include <strings.h>

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

static struct unit_s {
    char *unit;
    size_t value;
} units[] = {
    {"k", 1000L},
    {"ki", 1L << 10},
    {"M", 1000000L},
    {"Mi", 1L << 20},
    {"G", 1000000000L},
    {"Gi", 1L << 30},
    {"T", 1000000000000L},
    {"Ti", 1L << 40},
    {"P", 1000000000000000L},
    {"Pi", 1L << 50},
    {"E", 1000000000000000000L},
    {"Ei", 1L << 60},
    {NULL, 0},
    };

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

    CHECK(coyaml_group(info, root, config));

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

int coyaml_group(coyaml_parseinfo_t *info, coyaml_group_t *def, void *target) {
    COYAML_DEBUG("Entering Group");
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
    COYAML_DEBUG("Leaving Group");
    return 0;
}

int coyaml_int(coyaml_parseinfo_t *info, coyaml_int_t *def, void *target) {
    COYAML_DEBUG("Entering Int");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    unsigned char *end;
    int val = strtol(info->event.data.scalar.value, (char **)&end, 0);
    if(*end) {
        for(struct unit_s *unit = units; unit->unit; ++unit) {
            if(!strcmp(end, unit->unit)) {
                val *= unit->value;
                end += strlen(unit->unit);
                break;
            }
        }
    }
    SYNTAX_ERROR(end == info->event.data.scalar.value
        + info->event.data.scalar.length);
    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %d", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %d", def->min);
    *(long *)(((char *)target)+def->baseoffset) = val;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Int");
    return 0;
}

int coyaml_file(coyaml_parseinfo_t *info, coyaml_file_t *def, void *target) {
    COYAML_DEBUG("Entering File");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    // TODO: Implement more checks
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &info->head->pieces,
        info->event.data.scalar.value, info->event.data.scalar.length);
    *(int *)(((char *)target)+def->baseoffset+sizeof(char*)) =
        info->event.data.scalar.length;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving File");
    return 0;
}

int coyaml_dir(coyaml_parseinfo_t *info, coyaml_dir_t *def, void *target) {
    COYAML_DEBUG("Entering Dir");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    // TODO: Implement more checks
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &info->head->pieces,
        info->event.data.scalar.value, info->event.data.scalar.length);
    *(int *)(((char *)target)+def->baseoffset+sizeof(char*)) =
        info->event.data.scalar.length;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Dir");
    return 0;
}

int coyaml_string(coyaml_parseinfo_t *info, coyaml_string_t *def, void *target) {
    COYAML_DEBUG("Entering String");
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
        &info->head->pieces,
        info->event.data.scalar.value, info->event.data.scalar.length);
    *(int *)(((char *)target)+def->baseoffset+sizeof(char*)) =
        info->event.data.scalar.length;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving String");
    return 0;
}

int coyaml_usertype(coyaml_parseinfo_t *info, coyaml_usertype_t *def, void *target) {
    COYAML_DEBUG("Entering Usertype");
    if(info->event.type == YAML_SCALAR_EVENT) {
        SYNTAX_ERROR(def->scalar_fun);
        CHECK(def->scalar_fun(info,
            info->event.data.scalar.value, def, target));
        CHECK(coyaml_next(info));
    } else {
        SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT);
        CHECK(coyaml_group(info, def->group, target));
    }
    COYAML_DEBUG("Leaving Usertype");
    return 0;
}

int coyaml_custom(coyaml_parseinfo_t *info, coyaml_custom_t *def, void *target) {
    COYAML_DEBUG("Entering Custom");
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT
        || info->event.type == YAML_SCALAR_EVENT);
    CHECK(coyaml_usertype(info, def->usertype,
        ((char *)target)+def->baseoffset));
    COYAML_DEBUG("Leaving Custom");
    return 0;
}

int coyaml_mapping(coyaml_parseinfo_t *info, coyaml_mapping_t *def, void *target) {
    COYAML_DEBUG("Entering Mapping");
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT);
    CHECK(coyaml_next(info));
    coyaml_mappingel_head_t *lastel = NULL;
    size_t nelements = 0;
    while(info->event.type != YAML_MAPPING_END_EVENT) {
        coyaml_mappingel_head_t *newel = obstack_alloc(&info->head->pieces,
            def->element_size);
        bzero(newel, def->element_size);
        if(def->key_defaults) {
            def->key_defaults((char *)newel+*(int *)def->key_prop);
        }
        if(def->value_defaults) {
            def->value_defaults((char *)newel+*(int *)def->value_prop);
        }
        CHECK(def->key_callback(info, def->key_prop, newel));
        CHECK(def->value_callback(info, def->value_prop, newel));
        nelements += 1;
        if(!lastel) {
            *(void **)((char *)target+def->baseoffset) = newel;
        } else {
            lastel->next = newel;
        }
        lastel = newel;
    }
    *(size_t*)((char *)target+def->baseoffset+sizeof(void *)) = nelements;
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_END_EVENT);
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Mapping");
    return 0;
}

int coyaml_array(coyaml_parseinfo_t *info, coyaml_array_t *def, void *target) {
    COYAML_DEBUG("Entering Array");
    SYNTAX_ERROR(info->event.type == YAML_SEQUENCE_START_EVENT);
    CHECK(coyaml_next(info));
    coyaml_arrayel_head_t *lastel = NULL;
    size_t nelements = 0;
    while(info->event.type != YAML_SEQUENCE_END_EVENT) {
        coyaml_arrayel_head_t *newel = obstack_alloc(&info->head->pieces,
            def->element_size);
        bzero(newel, def->element_size);
        if(def->element_defaults) {
            def->element_defaults((char *)newel+*(int *)def->element_prop);
        }
        CHECK(def->element_callback(info, def->element_prop, newel));
        nelements += 1;
        if(!lastel) {
            *(void **)((char *)target+def->baseoffset) = newel;
        } else {
            lastel->next = newel;
        }
        lastel = newel;
    }
    *(size_t*)((char *)target+def->baseoffset+sizeof(void *)) = nelements;
    SYNTAX_ERROR(info->event.type == YAML_SEQUENCE_END_EVENT);
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Array");
    return 0;
}

int coyaml_parse_tag(coyaml_parseinfo_t *info,
    struct coyaml_usertype_s *prop, int *target) {
    COYAML_DEBUG("Entering Parse Tag");
    char *tag = info ? info->event.data.scalar.tag : NULL;
    if(!tag || !*tag) {
        if(prop->tags) {
            SYNTAX_ERROR(prop->default_tag == -1);
            *target = prop->default_tag;
        }
    } else {
        bool tagset = FALSE;
        for(coyaml_tag_t *t = prop->tags;t && t->tagname; ++t) {
            if(!strcmp(t->tagname, tag)) {
                *target = t->tagvalue;
                COYAML_DEBUG("Matched tag ``%s'', value %d",
                    t->tagname, t->tagvalue);
                tagset = TRUE;
            }
        }
        SYNTAX_ERROR(tagset);
    }
    COYAML_DEBUG("Leaving Parse Tag");
    return 0;
}

int coyaml_tagged_scalar(coyaml_parseinfo_t *info, char *value,
    struct coyaml_usertype_s *prop, void *target) {
    COYAML_DEBUG("Entering Tagged Scalar");
    CHECK(coyaml_parse_tag(info, prop, (int *)target));
    coyaml_group_t *gr = prop->group;
    COYAML_ASSERT(gr);
    coyaml_transition_t *tr = gr->transitions;
    COYAML_ASSERT(tr);
    coyaml_string_t *str = NULL;
    for(;tr->symbol; ++tr) {
        if(!strcmp(tr->symbol, "value")) {
            COYAML_ASSERT(tr->callback == (coyaml_state_fun)coyaml_string);
            str = tr->prop;
            break;
        }
    }
    COYAML_ASSERT(str);
    if(info) {
        *(char **)(((char *)target)+str->baseoffset) = obstack_copy0(
            &info->head->pieces,
            info->event.data.scalar.value, info->event.data.scalar.length);
        *(int *)(((char *)target)+str->baseoffset+sizeof(char*)) =
            info->event.data.scalar.length;
    } else {
        *(char **)(((char *)target)+str->baseoffset) = value; //Dirty hack
        *(int *)(((char *)target)+str->baseoffset+sizeof(char*)) =
            strlen(value);
    }
    COYAML_DEBUG("Leaving Tagged Scalar");
    return 0;
}
