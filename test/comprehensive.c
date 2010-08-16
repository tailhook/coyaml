#include <stdio.h>

#include "comprehensivecfg.h"

cfg_main_t config;

int main(int argc, char **argv) {
    cfg_load(&config, argc, argv);
    cfg_free(&config);
}
