#include <stdio.h>

#include "tinyconfig.h"

config_main_t config;

int main(int argc, char **argv) {
    config_load(&config, argc, argv);
    config_free(&config);
}
