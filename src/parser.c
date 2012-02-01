
#include <yaml.h>
#include <errno.h>
#include <obstack.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <alloca.h>
#include <ctype.h>

#include <coyaml_src.h>
#include "vars.h"
#include "parser.h"
#include "util.h"
#include "copy.h"
#include "eval.h"

#define SYNTAX_ERROR(cond) if(!(cond)) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %ld column %ld\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR_AT(line, col) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %ld column %ld\n", \
    info->current_file->filename, line, col); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR2(message, ...) if(TRUE) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %ld column %ld: " message "\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return -1; }
#define SYNTAX_ERROR2_NULL(message, ...) if(TRUE) { \
    fprintf(stderr, "COYAML: Syntax error in config file ``%s'' " \
        "at line %d column %d: " message "\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_SYNTAX_ERROR; \
    return NULL; }
#define VALUE_ERROR(cond, message, ...) if(!(cond)) { \
    fprintf(stderr, "COYAML: Error at %s:%ld[%ld]: " message "\n", \
    info->current_file->filename, info->event.start_mark.line+1, \
    info->event.start_mark.column, ##__VA_ARGS__); \
    errno = ECOYAML_VALUE_ERROR; \
    return -1; }
#define COYAML_DEBUG(message, ...) if(info->debug) { \
    fprintf(stderr, "COYAML: " message "\n", ##__VA_ARGS__); }
#define SETFLAG(info, def) if((info)->top_mark && (def)->flagoffset) { \
    COYAML_ASSERT(!((info)->top_mark->filled[(def)->flagoffset])); \
    (info)->top_mark->filled[(def)->flagoffset] = 1; \
    }
#define SETFLAG_1(info, def) if((info)->top_mark && (def)->flagoffset) { \
    COYAML_ASSERT(!((info)->top_mark->filled[(def)->flagoffset])); \
    (info)->top_mark->filled[(def)->flagoffset] = -1; \
    }

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

static char zero[sizeof(yaml_event_t)] = {0}; // compiler should make this zero

static int coyaml_next(coyaml_parseinfo_t *info);
static int topmost_next(coyaml_parseinfo_t *info);

static void my_event_delete(yaml_event_t *event) {
    event->data.scalar.tag = NULL;
    yaml_event_delete(event);
}

static int coyaml_skip(coyaml_parseinfo_t *info) {
    COYAML_DEBUG("Skipping subtree");
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
            default:
                break;
        }
    } while(level);
    COYAML_DEBUG("End of skipping");
    return 0;
}


static coyaml_anchor_t *find_anchor(coyaml_parseinfo_t *info, char *name) {
    for(coyaml_anchor_t *a = info->anchor_first; a; a = a->next) {
        if(!strcmp(name, a->name)) {
            return a;
        }
    }
    return NULL;
}

static coyaml_stack_t *open_file(coyaml_parseinfo_t *info, char *filename) {
    coyaml_stack_t *res = malloc(sizeof(coyaml_stack_t)+strlen(filename)+1);
    if(!res) return NULL;
    COYAML_DEBUG("Opening file ``%s''", filename);
    res->file = fopen(filename, "r");
    if(!res->file) {
        free(res);
        return NULL;
    }
    yaml_parser_initialize(&res->parser);
    yaml_parser_set_input_file(&res->parser, res->file);
    res->filename = (char *)res + sizeof(coyaml_stack_t);
    strcpy(res->filename, filename);
    char *suffix = strrchr(filename, '/');
    if(suffix) {
        res->basedir_len = suffix - filename + 1;
        res->basedir = obstack_alloc(&info->context->pieces,res->basedir_len+1);
        strncpy(res->basedir, filename, res->basedir_len);
        res->basedir[res->basedir_len] = 0;
    } else {
        res->basedir = "";
        res->basedir_len = 0;
    }
    res->next = NULL;
    res->prev = NULL;
    return res;
}

static int mapping_next(coyaml_parseinfo_t *info);
static int anchor_next(coyaml_parseinfo_t *info);
static int alias_next(coyaml_parseinfo_t *info);

static int unpack_anchor(coyaml_parseinfo_t *info) {
    memcpy(&info->event, &info->anchor_unpacking->events[info->anchor_pos],
        sizeof(info->event));
    if(info->event.type == YAML_NO_EVENT) {
        info->anchor_pos = -1;
        info->anchor_unpacking = NULL;
        return mapping_next(info);
    } else {
        info->anchor_pos += 1;
        if(info->event.type == YAML_SCALAR_EVENT) {
            COYAML_DEBUG("Unpacked %s[%d] (%.*s)",
                yaml_event_names[info->event.type], info->event.type,
                (int)info->event.data.scalar.length,
                info->event.data.scalar.value);
        } else {
            COYAML_DEBUG("Unpacked %s[%d]",
                yaml_event_names[info->event.type],
                info->event.type);
        }
        return 0;
    }
}

static int plain_next(coyaml_parseinfo_t *info) {
    long oldline = info->event.end_mark.line+1;
    long oldcol = info->event.end_mark.column;
    if(info->event.type && !info->anchor_unpacking && info->anchor_level < 0) {
        my_event_delete(&info->event);
    }
    if(!yaml_parser_parse(&info->current_file->parser, &info->event)) {
        SYNTAX_ERROR_AT(oldline, oldcol);
        return -1;
    }
    if(info->event.data.scalar.tag) {
        char *oldtag = (char*)info->event.data.scalar.tag;
        info->event.data.scalar.tag = obstack_copy0(
            &info->context->pieces, oldtag, strlen(oldtag));
        free(oldtag);
    }
    if(info->event.type == YAML_SCALAR_EVENT) {
        COYAML_DEBUG("Low-level event %s[%u] (%.*s)",
            yaml_event_names[info->event.type], info->event.type,
            (int)info->event.data.scalar.length,
            info->event.data.scalar.value);
    } else {
        COYAML_DEBUG("Low-level event %s[%d]",
            yaml_event_names[info->event.type],
            info->event.type);
    }
    return 0;
}

static int include_next(coyaml_parseinfo_t *info) {
    CHECK(plain_next(info));
    switch(info->event.type) {
        case YAML_SCALAR_EVENT:
            if(info->event.data.scalar.tag
                && !strcmp((char *)info->event.data.scalar.tag, "!Include")) {
                char *fn = (char *)info->event.data.scalar.value;
                SYNTAX_ERROR(*fn);
                if(*fn != '/') {
                    fn = alloca(info->current_file->basedir_len +
                        info->event.data.scalar.length + 1);
                    strcpy(fn, info->current_file->basedir);
                    strcpy(fn + info->current_file->basedir_len,
                        (char *)info->event.data.scalar.value);
                }
                coyaml_stack_t *cur = open_file(info, fn);
                VALUE_ERROR(cur, "Can't open file ``%s''", fn);
                cur->prev = info->current_file;
                info->current_file->next = cur;
                info->current_file = cur;

                CHECK(plain_next(info));
                SYNTAX_ERROR(info->event.type == YAML_STREAM_START_EVENT);
                CHECK(plain_next(info));
                SYNTAX_ERROR(info->event.type == YAML_DOCUMENT_START_EVENT);
                return plain_next(info);
            }
            break;
        case YAML_DOCUMENT_END_EVENT:
            if(info->current_file != info->root_file) {
                coyaml_stack_t *cur = info->current_file;
                CHECK(plain_next(info));
                SYNTAX_ERROR(info->event.type == YAML_STREAM_END_EVENT);
                yaml_parser_delete(&cur->parser);
                fclose(cur->file);
                info->current_file = cur->prev;
                info->current_file->next = NULL;
                free(cur);
                return plain_next(info);
            }
            break;
        default:
            break;
    }
    return 0;
}

static int alias_next(coyaml_parseinfo_t *info) {
    if(info->anchor_unpacking) {
        return unpack_anchor(info);
    }
    CHECK(include_next(info));
    if(info->event.type == YAML_ALIAS_EVENT) {
        coyaml_anchor_t *anch = find_anchor(info,
            (char *)info->event.data.alias.anchor);
        if(anch) {
            info->anchor_pos = 0;
            info->anchor_unpacking = anch;
            // Sorry, we don't delete event while unpacking alias
            my_event_delete(&info->event);
            return alias_next(info);
        } else {
            SYNTAX_ERROR2("Anchor %s not found",
                info->event.data.alias.anchor);
        }
    }
    return 0;
}

static int anchor_next(coyaml_parseinfo_t *info) {
    CHECK(alias_next(info));
    if(info->anchor_level == 0) {
        info->anchor_level = -1;
    }
    if(info->event.data.scalar.anchor && info->anchor_level >= 0) {
        SYNTAX_ERROR2("Nested anchors are not supported");
    }
    if(info->anchor_unpacking) {
        return 0; // No anchors while unpacking alias
    }
    switch(info->event.type) {
        case YAML_MAPPING_START_EVENT:
        case YAML_SEQUENCE_START_EVENT:
            if(info->anchor_level >= 0 || info->event.data.scalar.anchor) {
                ++ info->anchor_level;
            }
        case YAML_SCALAR_EVENT:
            if(info->event.data.scalar.anchor) {
                info->anchor_level += 1;
                char *name = obstack_copy0(&info->anchors,
                    info->event.data.scalar.anchor,
                    strlen((char *)info->event.data.scalar.anchor));
                COYAML_DEBUG("Found anchor ``%s''", name);
                obstack_blank(&info->anchors, sizeof(coyaml_anchor_t));
                coyaml_anchor_t *cur = obstack_base(&info->anchors);
                cur->name = name;
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
        obstack_grow(&info->anchors, &info->event, sizeof(info->event));
        if(info->event.type == YAML_SCALAR_EVENT) {
            COYAML_DEBUG("Packed %s[%d] (%.*s)",
                yaml_event_names[info->event.type], info->event.type,
                (int)info->event.data.scalar.length,
                info->event.data.scalar.value);
        } else {
            COYAML_DEBUG("Packed %s[%d]",
                yaml_event_names[info->event.type],
                info->event.type);
        }
        if(!info->anchor_level) {
            obstack_grow(&info->anchors, zero, sizeof(zero));
            coyaml_anchor_t *cur = obstack_finish(&info->anchors);
            COYAML_DEBUG("Done anchor ``%s''", cur->name);
            if(info->anchor_last) {
                info->anchor_last->next = cur;
                info->anchor_last = cur;
            } else {
                info->anchor_first = info->anchor_last = cur;
            }
            cur->next = NULL;
        }
    }
    return 0;
}

static int find_mapping_key(coyaml_mapkey_t *root, char *name,
    coyaml_mapkey_t **node) {
    coyaml_mapkey_t *parent;
    coyaml_mapkey_t *cur = root;
    while(TRUE) {
        int res = strcmp(name, cur->name);
        if(!res) {
            *node = cur;
            return 0;
        }
        if(res < 0) {
            parent = cur;
            cur = cur->left;
            if(!cur) {
                *node = parent;
                return -1;
            }
        } else { // res > 0
            parent = cur;
            cur = cur->right;
            if(!cur) {
                *node = parent;
                return 1;
            }
        }
    }
    abort();
}

static coyaml_mapkey_t *make_mapping_key(coyaml_parseinfo_t *info) {
    coyaml_mapkey_t *res = obstack_alloc(&info->mappieces,
        sizeof(coyaml_mapkey_t)+info->event.data.scalar.length+1);
    res->left = NULL;
    res->right = NULL;
    memcpy((char *)res + sizeof(coyaml_mapkey_t),
        info->event.data.scalar.value, info->event.data.scalar.length + 1);
    return res;
}

static int mapping_next(coyaml_parseinfo_t *info) {
    CHECK(anchor_next(info));
    coyaml_mapmerge_t *mapping = info->top_map;
    if(!mapping) return 0;
    switch(info->event.type) {
        case YAML_SCALAR_EVENT:
            if(!mapping->state
                && !strcmp((char *)info->event.data.scalar.value,"<<")) {
                CHECK(anchor_next(info));
                switch(info->event.type) {
                    case YAML_MAPPING_START_EVENT:
                        mapping->mergelevel += 1;
                        return mapping_next(info);
                    case YAML_SEQUENCE_START_EVENT:
                        mapping->mergelists = TRUE;
                        CHECK(anchor_next(info));
                        VALUE_ERROR(info->event.type==YAML_MAPPING_START_EVENT,
                            "Can only merge mappings %d", info->event.type);
                        mapping->mergelevel += 1;
                        return mapping_next(info);
                    default:
                        SYNTAX_ERROR2("Can merge only mapping"
                            " or sequence of mappings");
                        break;
                }
            }
            break;
        case YAML_MAPPING_END_EVENT:
            if(mapping->level) break;
            if(mapping->mergelevel) {
                mapping->mergelevel -= 1;
                if(mapping->mergelists) {
                    CHECK(anchor_next(info));
                    switch(info->event.type) {
                        case YAML_MAPPING_START_EVENT:
                            mapping->mergelevel += 1;
                            return mapping_next(info);
                        case YAML_SEQUENCE_END_EVENT:
                            mapping->mergelists = FALSE;
                            return mapping_next(info);
                        default:
                            SYNTAX_ERROR2("Can only merge mapping");
                            break;
                    }
                }
                return mapping_next(info);
            }
            break;
        default:
            break;
    }
    return 0;
}

static int duplicate_next(coyaml_parseinfo_t *info) {
    CHECK(mapping_next(info));
    coyaml_mapmerge_t *mapping = info->top_map;
    if(!mapping && info->event.type != YAML_MAPPING_START_EVENT) return 0;

    switch(info->event.type) {
        case YAML_SCALAR_EVENT:
            COYAML_DEBUG("Scalar at [%d] state %d level %d",
                mapping->height, mapping->state, mapping->level);
            if(!mapping->state && !mapping->level) {
                coyaml_mapkey_t *key = mapping->keys;
                if(!key) {
                    info->top_map->keys = make_mapping_key(info);
                } else {
                    int rel = find_mapping_key(key,
                        (char *)info->event.data.scalar.value, &key);
                    if(rel == 0) {
                        COYAML_DEBUG("Skipping duplicate ``%.*s''",
                            (int)info->event.data.scalar.length,
                            info->event.data.scalar.value);
                        CHECK(coyaml_skip(info));
                        mapping->state = 0;
                        return duplicate_next(info);
                    } else {
                        if(rel == 1) {
                            key->right = make_mapping_key(info);
                        } else { // rel == -1
                            key->left = make_mapping_key(info);
                        }
                    }
                }
                mapping->state = 1;
            } else if(!mapping->level) {
                mapping->state = 0;
            }
            break;
        case YAML_SEQUENCE_START_EVENT:
            COYAML_DEBUG("Sequence start at [%d] state %d level %d",
                mapping->height, mapping->state, mapping->level);
            mapping->level += 1;
            break;
        case YAML_SEQUENCE_END_EVENT:
            COYAML_DEBUG("Sequence end at [%d] state %d level %d",
                mapping->height, mapping->state, mapping->level);
            mapping->level -= 1;
            if(!mapping->level) {
                mapping->state = !mapping->state;
            }
            break;
        case YAML_MAPPING_START_EVENT:
            if(mapping) {
                COYAML_DEBUG("Mapping[%d] start at state %d level %d",
                    mapping->height, mapping->state, mapping->level);
            }
            if(!mapping || mapping->state == 1) {
                mapping = obstack_alloc(&info->mappieces,
                    sizeof(coyaml_mapmerge_t));
                mapping->prev = info->top_map;
                if(!mapping->prev) {
                    mapping->height = 0;
                } else {
                    mapping->height = mapping->prev->height + 1;
                }
                mapping->keys = NULL;
                mapping->state = 0;
                mapping->level = 0;
                mapping->mergelevel = 0;
                mapping->mergelists = 0;
                info->top_map = mapping;
            } else {
                mapping->level += 1;
            }
            break;
        case YAML_MAPPING_END_EVENT:
            COYAML_DEBUG("Mapping[%d] end at state %d level %d",
                mapping->height, mapping->state, mapping->level);
            if(!mapping->state && !mapping->level) {
                info->top_map = mapping->prev;
                obstack_free(&info->mappieces, mapping);
                mapping = info->top_map;
                if(mapping && !mapping->level) {
                    mapping->state = !mapping->state;
                }
            } else {
                mapping->level -= 1;
                if(!mapping->level) {
                    mapping->state = !mapping->state;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}
static int topmost_next(coyaml_parseinfo_t *info) {
    return duplicate_next(info);
}

static int coyaml_next(coyaml_parseinfo_t *info) {
    CHECK(topmost_next(info));
    if(info->event.type == YAML_SCALAR_EVENT) {
        COYAML_DEBUG("Event %s[%d]%s (%.*s)",
            yaml_event_names[info->event.type], info->event.type,
            info->anchor_unpacking ? "u" : "",
            (int)info->event.data.scalar.length,
            info->event.data.scalar.value);
    } else {
        COYAML_DEBUG("Event %s[%d]%s",
            yaml_event_names[info->event.type],
            info->event.type,
            info->anchor_unpacking ? "u" : "");
    }
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

int coyaml_readfile(coyaml_context_t *ctx) {
    coyaml_parseinfo_t sinfo;
    sinfo.context = ctx;
    sinfo.debug = ctx->debug;
    sinfo.parse_vars = ctx->parse_vars;
    sinfo.head = ctx->target;
    sinfo.target = ctx->target;
    sinfo.anchor_level = -1;
    sinfo.anchor_pos = -1;
    sinfo.anchor_unpacking = NULL;
    sinfo.anchor_first = NULL;
    sinfo.anchor_last = NULL;
    sinfo.top_map = NULL;
    sinfo.last_mark = NULL;
    sinfo.top_mark = NULL;
    sinfo.event.type = YAML_NO_EVENT;
    obstack_init(&sinfo.anchors);
    obstack_init(&sinfo.mappieces);

    coyaml_parseinfo_t *info = &sinfo;

    sinfo.root_file = sinfo.current_file = open_file(info, ctx->root_filename);
    if(!sinfo.root_file) {
        obstack_free(&sinfo.anchors, NULL);
        obstack_free(&sinfo.mappieces, NULL);
        return -1;
    }

    ctx->parseinfo = &sinfo;
    int result = coyaml_root(info, ctx->root_group, ctx->target);
    if(ctx->print_vars) {
        coyaml_print_variables(ctx);
    }
    ctx->parseinfo = NULL;

    for(coyaml_marks_t *m = sinfo.last_mark; m; m = m->prev) {
        if(m->parent && m->parent->type == m->type) {
            COYAML_ASSERT(m->prop);
            CHECK(coyaml_copier(info->context, m->prop, m->parent, m));
        }
    }

    for(coyaml_anchor_t *a = sinfo.anchor_first; a; a = a->next) {
        for(yaml_event_t *ev = a->events; ev->type != YAML_NO_EVENT; ++ev) {
            my_event_delete(ev);
        }
    }
    obstack_free(&sinfo.anchors, NULL);
    obstack_free(&sinfo.mappieces, NULL);

    for(coyaml_stack_t *t = info->current_file, *n; t; t = n) {
        yaml_parser_delete(&t->parser);
        fclose(t->file);
        n = t->prev;
        free(t);
    }
    COYAML_DEBUG("Done %s", result ? "ERROR" : "OK");
    return result;
}

int coyaml_group(coyaml_parseinfo_t *info, coyaml_group_t *def, void *target) {
    COYAML_DEBUG("Entering Group");
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT);
    CHECK(coyaml_next(info));
    while(info->event.type == YAML_SCALAR_EVENT) {
        if(info->event.data.scalar.value[0] == '_') {
            // Hidden keys, user's can use that for their own reusable anchors
            CHECK(coyaml_skip(info));
            CHECK(coyaml_next(info));
            continue;
        }
        coyaml_transition_t *tran;
        char *key = (char *)info->event.data.scalar.value;
        if(!strcmp(key, "=")) {
            key = "value";
        }
        for(tran = def->transitions;
            tran && tran->symbol; ++tran) {
            if(!strcmp(tran->symbol, key)) {
                break;
            }
        }
        if(tran && tran->symbol) {
            COYAML_DEBUG("Matched key ``%s''", tran->symbol);
            CHECK(coyaml_next(info));
            CHECK(tran->prop->type->yaml_parse(info, tran->prop, target));
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
    SETFLAG(info, def);
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);

    long val = 0;
    if(coyaml_eval_int(info, (char *)info->event.data.scalar.value,
        info->event.data.scalar.length, &val)) {
        SYNTAX_ERROR(0);
    }

    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %d", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %d", def->min);
    *(long *)(((char *)target)+def->baseoffset) = val;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Int");
    return 0;
}

int coyaml_float(coyaml_parseinfo_t *info, coyaml_float_t *def, void *target) {
    COYAML_DEBUG("Entering Float");
    SETFLAG(info, def);
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);

    double val;
    if(coyaml_eval_float(info, (char *)info->event.data.scalar.value,
        info->event.data.scalar.length, &val)) {
        SYNTAX_ERROR(0);
    }

    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %lf", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %lf", def->min);
    *(double *)(((char *)target)+def->baseoffset) = val;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Float");
    return 0;
}

int coyaml_bool(coyaml_parseinfo_t *info, coyaml_bool_t *def, void *target) {
    COYAML_DEBUG("Entering Bool");
    SETFLAG(info, def);
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    char *value = (char *)info->event.data.scalar.value;
    if(
        !strcasecmp(value, "true")
        || !strcasecmp(value, "y")
        || !strcasecmp(value, "yes")
        || !strcasecmp(value, "on")
        ) {
        *(bool *)(((char *)target)+def->baseoffset) = TRUE;
    } else if(
        !strcasecmp(value, "false")
        || !strcasecmp(value, "n")
        || !strcasecmp(value, "no")
        || !strcasecmp(value, "off")
        ) {
        *(bool *)(((char *)target)+def->baseoffset) = FALSE;
    } else {
        VALUE_ERROR(FALSE, "Option value ``%s'' is not boolean", value);
    }
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving Bool");
    return 0;
}

int coyaml_uint(coyaml_parseinfo_t *info, coyaml_uint_t *def, void *target) {
    COYAML_DEBUG("Entering UInt");
    SETFLAG(info, def);
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    long tval = 0;
    if(coyaml_eval_int(info, (char *)info->event.data.scalar.value,
        info->event.data.scalar.length, &tval)) {
        SYNTAX_ERROR(0);
    }
    unsigned long val = (unsigned long) tval;
    VALUE_ERROR(!(def->bitmask&2) || val <= def->max,
        "Value must be less than or equal to %d", def->max);
    VALUE_ERROR(!(def->bitmask&1) || val >= def->min,
        "Value must be greater than or equal to %d", def->min);
    VALUE_ERROR(tval >= 0,
        "Value must be greater or equal to zero");
    *(unsigned long *)(((char *)target)+def->baseoffset) = val;
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving UInt");
    return 0;
}

int coyaml_file(coyaml_parseinfo_t *info, coyaml_file_t *def, void *target) {
    COYAML_DEBUG("Entering File");
    SETFLAG(info, def);
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
    SETFLAG(info, def);
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
    SETFLAG(info, def);
    SYNTAX_ERROR(info->event.type == YAML_SCALAR_EVENT);
    char *tag = info ? (char *)info->event.data.scalar.tag : NULL;
    if(tag) {
        if(!strcmp(tag, "!FromFile")) {
            char *fn = (char *)info->event.data.scalar.value;
            if(*fn != '/') {
                fn = alloca(info->current_file->basedir_len
                    + info->event.data.scalar.length + 1);
                strcpy(fn, info->current_file->basedir);
                strcpy(fn + info->current_file->basedir_len,
                    (char *)info->event.data.scalar.value);
            }
            COYAML_DEBUG("Opening ``%s'' at ``%s''",
                fn, info->current_file->basedir);
            int file = open(fn, O_RDONLY);
            VALUE_ERROR(file >= 0, "Can't open file ``%s''", fn);
            struct stat finfo;
            VALUE_ERROR(!fstat(file, &finfo), "Can't stat ``%s''", fn);
            void *body = *(char **)(((char *)target)+def->baseoffset) \
                = obstack_alloc(&info->head->pieces, finfo.st_size);
            *(int *)(((char *)target)+def->baseoffset+sizeof(char*)) \
                = finfo.st_size;
            VALUE_ERROR(read(file, body, finfo.st_size) == finfo.st_size,
                "Couldn't read file ``%s''", fn);
            close(file);
        } else if(!strcmp(tag, "!Raw")) {
            *(char **)(((char *)target)+def->baseoffset) = obstack_copy0(
                &info->head->pieces, info->event.data.scalar.value,
                info->event.data.scalar.length);
            *(int *)(((char *)target)+def->baseoffset+sizeof(char*)) \
                = info->event.data.scalar.length;
        } else {
            VALUE_ERROR(TRUE, "Unknown tag ``%s''", tag);
        }
    } else {
        char *data = (char *)info->event.data.scalar.value;
        int dlen = info->event.data.scalar.length;
        if(coyaml_eval_str(info, data, dlen, &data, &dlen)) {
            SYNTAX_ERROR(0);
        }
        *(char **)(((char *)target)+def->baseoffset) = data;
        *(int *)(((char *)target)+def->baseoffset+sizeof(char*)) = dlen;
    }
    CHECK(coyaml_next(info));
    COYAML_DEBUG("Leaving String");
    return 0;
}

int coyaml_usertype(coyaml_parseinfo_t *info, coyaml_usertype_t *def, void *target) {
    COYAML_DEBUG("Entering Usertype");
    if(info->event.type == YAML_SCALAR_EVENT) {
        SYNTAX_ERROR(def->scalar_fun);
        CHECK(def->scalar_fun(info,
            (char *)info->event.data.scalar.value, def, target));
        if(def->scalar_fun != coyaml_tagged_scalar) {
            CHECK(coyaml_next(info));
        }
    } else if(info->event.type == YAML_SEQUENCE_START_EVENT) {
        CHECK(coyaml_parse_tag(info, def, target));
        for(coyaml_transition_t *tr = def->group->transitions; tr->symbol; ++tr) {
            COYAML_DEBUG("Symbol ``%s''", tr->symbol);
            if(!strcmp(tr->symbol, "value")) {
                COYAML_ASSERT((void *)tr->prop->type == &coyaml_array_type);
                CHECK(coyaml_array(info, (coyaml_array_t *)tr->prop, target));
                break;
            }
        }
    } else {
        CHECK(coyaml_parse_tag(info, def, target));
        SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT);
        int fsize = sizeof(coyaml_marks_t) + sizeof(char)*def->flagcount;
        coyaml_marks_t *marks = obstack_alloc(&info->context->pieces, fsize);
        bzero(marks, fsize);
        marks->type = def->ident;
        marks->object = target;
        marks->prop = def;
        marks->parent = info->top_mark;
        info->top_mark = marks;

        CHECK(coyaml_group(info, def->group, target));

        info->top_mark = marks->parent;
        marks->prev = info->last_mark;
        info->last_mark = marks;
    }
    COYAML_DEBUG("Leaving Usertype");
    return 0;
}

int coyaml_custom(coyaml_parseinfo_t *info, coyaml_custom_t *def, void *target) {
    COYAML_DEBUG("Entering Custom");
    SETFLAG(info, def);
    SYNTAX_ERROR(info->event.type == YAML_MAPPING_START_EVENT
        || info->event.type == YAML_SEQUENCE_START_EVENT
        || info->event.type == YAML_SCALAR_EVENT);
    CHECK(coyaml_usertype(info, def->usertype,
        ((char *)target)+def->baseoffset));
    COYAML_DEBUG("Leaving Custom");
    return 0;
}

int coyaml_mapping(coyaml_parseinfo_t *info, coyaml_mapping_t *def, void *target) {
    COYAML_DEBUG("Entering Mapping");
    if(def->inheritance == COYAML_INH_REPLACE_DEFAULT) {
        if(!info->event.data.mapping_start.tag
            || strcmp((char *)info->event.data.mapping_start.tag, "!Append")) {
            SETFLAG(info, def);
        }
    } else if(def->inheritance == COYAML_INH_APPEND_DEFAULT) {
        if(info->event.data.mapping_start.tag && !strcmp(
            (char *)info->event.data.mapping_start.tag, "!Replace")) {
            SETFLAG(info, def);
        } else {
            SETFLAG_1(info, def);
        }
    }
    CHECK(coyaml_next(info));
    coyaml_mappingel_head_t *lastel = NULL;
    size_t nelements = 0;
    while(info->event.type != YAML_MAPPING_END_EVENT) {
        coyaml_mappingel_head_t *newel = obstack_alloc(&info->head->pieces,
            def->element_size);
        bzero(newel, def->element_size);
        if(def->key_defaults) {
            def->key_defaults((char *)newel + def->key_prop->baseoffset);
        }
        if(def->value_defaults) {
            def->value_defaults((char *)newel + def->value_prop->baseoffset);
        }
        CHECK(def->key_prop->type->yaml_parse(info, def->key_prop, newel));
        CHECK(def->value_prop->type->yaml_parse(info, def->value_prop, newel));
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
    if(def->inheritance == COYAML_INH_REPLACE_DEFAULT) {
        if(!info->event.data.sequence_start.tag
            || strcmp((char*)info->event.data.sequence_start.tag, "!Append")) {
            SETFLAG(info, def);
        }
    } else if(def->inheritance == COYAML_INH_APPEND_DEFAULT) {
        if(info->event.data.sequence_start.tag && !strcmp(
            (char *)info->event.data.sequence_start.tag, "!Replace")) {
            SETFLAG(info, def);
        } else {
            SETFLAG_1(info, def);
        }
    }
    SYNTAX_ERROR(info->event.type == YAML_SEQUENCE_START_EVENT);
    CHECK(coyaml_next(info));
    coyaml_arrayel_head_t *lastel = NULL;
    size_t nelements = 0;
    while(info->event.type != YAML_SEQUENCE_END_EVENT) {
        coyaml_arrayel_head_t *newel = obstack_alloc(&info->head->pieces,
            def->element_size);
        bzero(newel, def->element_size);
        if(def->element_defaults) {
            def->element_defaults((char *)newel + def->element_prop->baseoffset);
        }
        CHECK(def->element_prop->type->yaml_parse(info,
            def->element_prop, newel));
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
    char *tag = info ? (char *)info->event.data.scalar.tag : NULL;
    if(!tag || !*tag) {
        if(prop->tags) {
            SYNTAX_ERROR(prop->default_tag != -1);
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
    if(info) {
        CHECK(coyaml_parse_tag(info, prop, (int *)target));
        if(info->event.data.scalar.tag) {
            info->event.data.scalar.tag = NULL;
        }
    }
    coyaml_group_t *gr = prop->group;
    COYAML_ASSERT(gr);
    coyaml_transition_t *tr = gr->transitions;
    COYAML_ASSERT(tr);
    for(;tr->symbol; ++tr) {
        if(!strcmp(tr->symbol, "value")) {
            COYAML_ASSERT(tr->prop->type == &coyaml_string_type || info);
            break;
        }
    }
    COYAML_ASSERT(tr);
    if(info) {
        tr->prop->type->yaml_parse(info, tr->prop, target);
    } else {
        *(char **)(((char *)target)+tr->prop->baseoffset) = value; //Dirty hack
        *(int *)(((char *)target)+tr->prop->baseoffset+sizeof(char*)) =
            strlen(value);
    }
    COYAML_DEBUG("Leaving Tagged Scalar");
    return 0;
}

coyaml_context_t *coyaml_context_init(coyaml_context_t *inp) {
    coyaml_context_t *ctx;
    if(!inp) {
        ctx = malloc(sizeof(coyaml_context_t));
        if(!ctx) return NULL;
        memset(ctx, 0, sizeof(coyaml_context_t));
        ctx->free_object = TRUE;
    } else {
        ctx = inp;
        memset(ctx, 0, sizeof(coyaml_context_t));
        ctx->free_object = FALSE;
    }
    ctx->parse_vars = TRUE;
    ctx->print_vars = FALSE;
    ctx->parseinfo = NULL;
    obstack_init(&ctx->pieces);
    coyaml_set_string(ctx, "coyaml_version",
        COYAML_VERSION, strlen(COYAML_VERSION));
    return ctx;
}

void coyaml_context_free(coyaml_context_t *ctx) {
    obstack_free(&ctx->pieces, NULL);
    if(ctx->free_object) {
        free(ctx);
    }
}

void coyaml_config_free(void *ptr) {
    obstack_free(&((coyaml_head_t *)ptr)->pieces, NULL);
    if(((coyaml_head_t *)ptr)->free_object) {
        free(ptr);
    }
}

