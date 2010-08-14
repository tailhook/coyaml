#include <stdio.h>

#include "tinytestcfg.h"

cfg_main_t config;

int main(int argc, char **argv) {
    char *filename = "/etc/testhttp.yaml";
    bool debug = FALSE;
    //config_prepare_options(argc, argv, &config_filename, &config_debug);
    if(cfg_readfile(filename, &config, debug) < 0) {
        perror(filename);
    }
    //config_read_options(argc, argv, &config); // CLI overrides config
}
