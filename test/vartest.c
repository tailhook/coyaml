#include <stdio.h>
#include <getopt.h>

#include "vars.h"

config_main_t config;

int main(int argc, char **argv) {
    config_load(&config, argc, argv);
    for(int i = optind; i < argc; ++i) {
        printf("option: %s\n", argv[i]);
    }
    config_free(&config);
}
