#include <unistd.h>
#include <errno.h>
#include "coyaml_src.h"

void coyaml_cli_prepare_or_exit(coyaml_context_t *ctx, int argc, char **argv) {
    if(coyaml_cli_prepare(ctx, argc, argv) < 0) {
        if(errno > ECOYAML_MAX || errno < ECOYAML_MIN) {
            perror(ctx->program_name);
        }
        coyaml_config_free(ctx->target);
        coyaml_context_free(ctx);
        exit((errno == ECOYAML_CLI_HELP) ? 0 : 1);
    }
}

void coyaml_readfile_or_exit(coyaml_context_t *ctx) {
    if(coyaml_readfile(ctx) < 0) {
        if(errno > ECOYAML_MAX || errno < ECOYAML_MIN) {
            perror(ctx->program_name);
        }
        coyaml_config_free(ctx->target);
        coyaml_context_free(ctx);
        exit(1);
    }
}
void coyaml_cli_parse_or_exit(coyaml_context_t *ctx, int argc, char **argv) {
    if(coyaml_cli_parse(ctx, argc, argv) < 0) {
        if(errno > ECOYAML_MAX || errno < ECOYAML_MIN) {
            perror(ctx->program_name);
        }
        coyaml_config_free(ctx->target);
        coyaml_context_free(ctx);
        exit((errno == ECOYAML_CLI_EXIT) ? 0 : 1);
    }
}
void coyaml_env_parse_or_exit(coyaml_context_t *ctx) {
    if(coyaml_env_parse(ctx) < 0) {
        if(errno > ECOYAML_MAX || errno < ECOYAML_MIN) {
            perror(ctx->program_name);
        }
        coyaml_config_free(ctx->target);
        coyaml_context_free(ctx);
        exit(1);
    }
}
