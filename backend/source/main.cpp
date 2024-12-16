#include <stdio.h>
#include <stdlib.h>

#include "backend.h"
#include "language.h"
#include "lang_dump.h"

static int main_exit_failure(language_t *language);

int main(int argc, const char *argv[]) {
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }

    language_t language = {};
    if(backend_ctor(&language, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    if(read_tree(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    if(dump_tree(&language, "main dump after reading.") != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    if(compile_code(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }

    return EXIT_SUCCESS;
}


int main_exit_failure(language_t *language) {
    backend_dtor(language);
    return EXIT_FAILURE;
}
