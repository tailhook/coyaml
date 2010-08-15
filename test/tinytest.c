#include <stdio.h>

#include "tinytestcfg.h"

cfg_main_t config;

int main(int argc, char **argv) {
    coyaml_cli_prepare(argc, argv, &cfg_cmdline);
    if(cfg_readfile(cfg_cmdline.filename, &config, cfg_cmdline.debug) < 0) {
        perror(cfg_cmdline.filename);
    }
    coyaml_cli_parse(argc, argv, &cfg_cmdline, &config); // CLI overrides config
}
