#include <yaml.h>

#include "emitter.h"
#include "util.h"

#define EMIT_STRING(value) CHECK(yaml_scalar_event_initialize(&event, \
            NULL, (unsigned char *)"tag:yaml.org,2002:str", \
            (unsigned char *)(value), -1, \
            1, 1, YAML_PLAIN_SCALAR_STYLE)); \
            CHECK(yaml_emitter_emit(&ctx->emitter, &event));
#define EMIT_STRING_LEN(value, len) CHECK(yaml_scalar_event_initialize(&event,\
            NULL, (unsigned char *)"tag:yaml.org,2002:str", \
            (unsigned char *)(value), (len), \
            1, 1, YAML_PLAIN_SCALAR_STYLE)); \
            CHECK(yaml_emitter_emit(&ctx->emitter, &event));
#define VISIT(child, target) CHECK((child)->type->emit(ctx, \
    (child), target));

int coyaml_print_root(coyaml_printctx_t *ctx) {
    yaml_event_t event;

    CHECK(yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));
    CHECK(yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));

    CHECK(coyaml_group_emit(ctx, (coyaml_group_t *)ctx->root, ctx->config));

    CHECK(yaml_document_end_event_initialize(&event, 1));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));
    CHECK(yaml_stream_end_event_initialize(&event));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));
    return 0;
}

int coyaml_print(FILE *file, coyaml_group_t *root,
    void *cfg, coyaml_print_enum mode)
{
    coyaml_printctx_t ctx;
    CHECK(yaml_emitter_initialize(&ctx.emitter));
    yaml_emitter_set_output_file(&ctx.emitter, file);
    ctx.file = file;
    ctx.comments = mode & COYAML_PRINT_COMMENTS;
    ctx.defaults = (mode & 0xf) == COYAML_PRINT_SHORT;
    ctx.config = cfg;
    ctx.root = root;
    int res = coyaml_print_root(&ctx);
    if(res < 0) {
        switch (ctx.emitter.error)
        {
            case YAML_MEMORY_ERROR:
                fprintf(stderr, "Memory error: Not enough memory for emitting\n");
                break;

            case YAML_WRITER_ERROR:
                fprintf(stderr, "Writer error: %s\n", ctx.emitter.problem);
                break;

            case YAML_EMITTER_ERROR:
                fprintf(stderr, "Emitter error: %s\n", ctx.emitter.problem);
                break;

            default:
                /* Couldn't happen. */
                fprintf(stderr, "Internal error: %m\n");
                break;
        }
    }

    yaml_emitter_delete(&ctx.emitter);
    return res;
}

int group_emit_impl(coyaml_printctx_t *ctx,
    coyaml_group_t *prop, void *target, char *tag, bool tag_implicit)
{
    yaml_event_t event;
    CHECK(yaml_mapping_start_event_initialize(&event,
        NULL, (unsigned char *)tag, tag_implicit,
        YAML_BLOCK_MAPPING_STYLE));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));

    for(coyaml_transition_t *tr = prop->transitions; tr && tr->symbol; ++tr) {
        if(tr->prop->description && ctx->comments) {
            char buf[strlen("_help_") + strlen(tr->symbol)];
            strcpy(buf, "_help_");
            strcpy(buf + strlen("_help_"), tr->symbol);
            EMIT_STRING(buf);
            EMIT_STRING(tr->prop->description);
        }
        EMIT_STRING(tr->symbol);
        VISIT(tr->prop, target);
    }

    CHECK(yaml_mapping_end_event_initialize(&event));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));
    return 0;
}

int coyaml_group_emit(coyaml_printctx_t *ctx,
    coyaml_group_t *prop, void *target)
{
    return group_emit_impl(ctx, prop, target,
        "tag:yaml.org,2002:map", TRUE);
}

int coyaml_usertype_emit(coyaml_printctx_t *ctx,
    coyaml_usertype_t *prop, void *target)
{
    char *tag = "tag:yaml.org,2002:map";
    if(prop->tags) {
        int tnum = *(int *)target;
        for(coyaml_tag_t *ctag = prop->tags; ctag->tagname; ++ctag) {
            if(tnum == ctag->tagvalue) {
                tag = ctag->tagname;
                break;
            }
        }
        return group_emit_impl(ctx, prop->group, target,
            tag, tnum == prop->default_tag);
    }
    return group_emit_impl(ctx, prop->group, target, tag, TRUE);
}

int coyaml_custom_emit(coyaml_printctx_t *ctx,
    coyaml_custom_t *prop, void *target)
{
    VISIT((coyaml_placeholder_t *)prop->usertype,
        ((char *)target)+prop->baseoffset);
    return 0;
}

int coyaml_array_emit(coyaml_printctx_t *ctx,
    coyaml_array_t *prop, void *target)
{
    yaml_event_t event;
    CHECK(yaml_sequence_start_event_initialize(&event,
        NULL, (unsigned char *)"tag:yaml.org,2002:seq", 1,
        YAML_BLOCK_SEQUENCE_STYLE));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));

    for(coyaml_arrayel_head_t *el
        = *(coyaml_arrayel_head_t **)((char *)target+prop->baseoffset);
        el; el = el->next) {
        VISIT(prop->element_prop, el);
    }

    CHECK(yaml_sequence_end_event_initialize(&event));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));
    return 0;
}

int coyaml_mapping_emit(coyaml_printctx_t *ctx,
    coyaml_mapping_t *prop, void *target)
{
    yaml_event_t event;
    CHECK(yaml_mapping_start_event_initialize(&event,
        NULL, (unsigned char *)"tag:yaml.org,2002:map", 1,
        YAML_BLOCK_MAPPING_STYLE));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));

    for(coyaml_mappingel_head_t *el
        = *(coyaml_mappingel_head_t **)((char *)target+prop->baseoffset);
        el; el = el->next) {
        VISIT(prop->key_prop, el);
        VISIT(prop->value_prop, el);
    }

    CHECK(yaml_mapping_end_event_initialize(&event));
    CHECK(yaml_emitter_emit(&ctx->emitter, &event));
    return 0;
}

int coyaml_int_emit(coyaml_printctx_t *ctx,
    coyaml_placeholder_t *prop, void *target)
{
    char buf[24];
    int len = snprintf(buf, 24, "%ld", *(long *)((char *)target
            + prop->baseoffset));
    yaml_event_t event;
    EMIT_STRING_LEN(buf, len);
    return 0;
}

int coyaml_uint_emit(coyaml_printctx_t *ctx,
    coyaml_placeholder_t *prop, void *target)
{
    char buf[24];
    int len = snprintf(buf, 24, "%lu", *(long *)((char *)target
            + prop->baseoffset));
    yaml_event_t event;
    EMIT_STRING_LEN(buf, len);
    return 0;
}

int coyaml_bool_emit(coyaml_printctx_t *ctx,
    coyaml_placeholder_t *prop, void *target)
{
    bool value = *(bool *)((char *)target + prop->baseoffset);
    yaml_event_t event;
    if(value) {
        EMIT_STRING("yes");
    } else {
        EMIT_STRING("no");
    }
    return 0;
}

int coyaml_float_emit(coyaml_printctx_t *ctx,
    coyaml_placeholder_t *prop, void *target)
{
    char buf[24];
    int len = snprintf(buf, 24, "%f", *(double *)((char *)target
            + prop->baseoffset));
    yaml_event_t event;
    EMIT_STRING_LEN(buf, len);
    return 0;
}

int coyaml_dir_emit(coyaml_printctx_t *ctx,
    coyaml_dir_t *prop, void *target)
{
    yaml_event_t event;
    char *str = *(char **)((char *)target + prop->baseoffset);
    if(str) {
        EMIT_STRING_LEN(str, *(int *)((char *)target
            + prop->baseoffset + sizeof(char *)));
    } else {
        EMIT_STRING("");
    }
    return 0;
}

int coyaml_file_emit(coyaml_printctx_t *ctx,
    coyaml_file_t *prop, void *target)
{
    yaml_event_t event;
    char *str = *(char **)((char *)target + prop->baseoffset);
    if(str) {
        EMIT_STRING_LEN(str, *(int *)((char *)target
            + prop->baseoffset + sizeof(char *)));
    } else {
        EMIT_STRING("");
    }
    return 0;
}

int coyaml_string_emit(coyaml_printctx_t *ctx,
    coyaml_string_t *prop, void *target)
{
    yaml_event_t event;
    char *str = *(char **)((char *)target + prop->baseoffset);
    if(str) {
        EMIT_STRING_LEN(str, *(int *)((char *)target
            + prop->baseoffset + sizeof(char *)));
    } else {
        EMIT_STRING("");
    }
    return 0;
}

