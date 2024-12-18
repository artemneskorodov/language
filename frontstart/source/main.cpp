#include <stdio.h>
#include <stdlib.h>

#include "frontstart.h"
#include "language.h"
#include "lang_dump.h"

static int main_exit_failure(language_t *ctx);

int main(int argc, const char *argv[]) {
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }

    language_t ctx = {};
    if(frontstart_ctor(&ctx, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    if(read_tree(&ctx) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    if(dump_tree(&ctx, "dump after reading") != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    if(frontstart_write(&ctx) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }

    frontstart_dtor(&ctx);
    return EXIT_SUCCESS;
}

int main_exit_failure(language_t *ctx) {
    frontstart_dtor(ctx);
    return EXIT_FAILURE;
}
