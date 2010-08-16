#include <stdio.h>

#include "tinytestcfg.h"

cfg_main_t config;

int main(int argc, char **argv) {
    cfg_load(&config, argc, argv);
    cfg_free(&config);
}
