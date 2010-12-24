
#include <errno.h>

#include "copy.h"
#include "util.h"

#define REF(obj, prop, typ) *(typ*)((char *)(obj) + (prop)->baseoffset)
#define LEN(obj, prop, typ) *(int*)((char *)(obj) \
    + (prop)->baseoffset + sizeof(typ))

static int copy_group(coyaml_context_t *ctx, coyaml_group_t *group,
    coyaml_marks_t *source, coyaml_marks_t *target)
{
    void *src = source->object;
    void *trg = target->object;
    for(coyaml_transition_t *tr = group->transitions;
        tr && tr->symbol; ++tr) {
        if(tr->prop->type->ident == COYAML_GROUP) {
            copy_group(ctx, (coyaml_group_t *)tr->prop, source, target);
        } else if(tr->prop->flagoffset
            &&  target->filled[tr->prop->flagoffset] <= 0
            &&  source->filled[tr->prop->flagoffset]) {
            CHECK(tr->prop->type->copy(ctx,
                tr->prop, src,
                tr->prop, trg));
            target->filled[tr->prop->flagoffset] = 1;
        }
    }
    return 0;
}


int coyaml_copier(coyaml_context_t *ctx, coyaml_usertype_t *def,
    coyaml_marks_t *source, coyaml_marks_t *target)
{
    CHECK(copy_group(ctx, def->group, source, target));
    return 0;
}

int coyaml_custom_copy(coyaml_context_t *ctx,
    struct coyaml_custom_s *sprop, void *source,
    struct coyaml_custom_s *tprop, void *target)
{
    COYAML_ASSERT(tprop->usertype->size == sprop->usertype->size);
    memcpy((char *)target + tprop->baseoffset,
        (char *)source + sprop->baseoffset,
        tprop->usertype->size);
    return 0;
}

int coyaml_array_copy(coyaml_context_t *ctx,
    struct coyaml_array_s *sprop, void *source,
    struct coyaml_array_s *tprop, void *target)
{
    coyaml_arrayel_head_t *m = REF(target, tprop, coyaml_arrayel_head_t *);
    for(;m && m->next; m = m->next);
    if(m) {
        m->next = REF(source, sprop, coyaml_arrayel_head_t *);
    } else {
        REF(target, tprop, coyaml_arrayel_head_t *) \
            = REF(source, sprop, coyaml_arrayel_head_t *);
    }
    LEN(target, tprop, coyaml_arrayel_head_t *) \
        += LEN(source, sprop, coyaml_arrayel_head_t *);
    
    return 0;
}

int coyaml_mapping_copy(coyaml_context_t *ctx,
    struct coyaml_mapping_s *sprop, void *source,
    struct coyaml_mapping_s *tprop, void *target)
{
    coyaml_mappingel_head_t *m = REF(target, tprop, coyaml_mappingel_head_t *);
    for(;m && m->next; m = m->next);
    if(m) {
        m->next = REF(source, sprop, coyaml_mappingel_head_t *);
    } else {
        REF(target, tprop, coyaml_mappingel_head_t *) \
            = REF(source, sprop, coyaml_mappingel_head_t *);
    }
    LEN(target, tprop, coyaml_mappingel_head_t *) \
        += LEN(source, sprop, coyaml_mappingel_head_t *);
    return 0;
}

int coyaml_int_copy(coyaml_context_t *ctx,
    struct coyaml_int_s *sprop, void *source,
    struct coyaml_int_s *tprop, void *target)
{
    REF(target, tprop, long) = REF(source, sprop, long);
    return 0;
}

int coyaml_uint_copy(coyaml_context_t *ctx,
    struct coyaml_uint_s *sprop, void *source,
    struct coyaml_uint_s *tprop, void *target)
{
    REF(target, tprop, unsigned long) = REF(source, sprop, unsigned long);
    return 0;
}

int coyaml_bool_copy(coyaml_context_t *ctx,
    struct coyaml_bool_s *sprop, void *source,
    struct coyaml_bool_s *tprop, void *target)
{
    REF(target, tprop, bool) = REF(source, sprop, bool);
    return 0;
}

int coyaml_float_copy(coyaml_context_t *ctx,
    struct coyaml_float_s *sprop, void *source,
    struct coyaml_float_s *tprop, void *target)
{
    REF(target, tprop, float) = REF(source, sprop, float);
    return 0;
}

int coyaml_dir_copy(coyaml_context_t *ctx,
    struct coyaml_dir_s *sprop, void *source,
    struct coyaml_dir_s *tprop, void *target)
{
    REF(target, tprop, char *) = REF(source, sprop, char *);
    LEN(target, tprop, char *) = LEN(source, sprop, char *);
    return 0;
}

int coyaml_file_copy(coyaml_context_t *ctx,
    struct coyaml_file_s *sprop, void *source,
    struct coyaml_file_s *tprop, void *target)
{
    REF(target, tprop, char *) = REF(source, sprop, char *);
    LEN(target, tprop, char *) = LEN(source, sprop, char *);
    return 0;
}

int coyaml_string_copy(coyaml_context_t *ctx,
    struct coyaml_string_s *sprop, void *source,
    struct coyaml_string_s *tprop, void *target)
{
    REF(target, tprop, char *) = REF(source, sprop, char *);
    LEN(target, tprop, char *) = LEN(source, sprop, char *);
    return 0;
}
