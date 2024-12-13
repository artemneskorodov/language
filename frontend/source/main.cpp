#include <stdio.h>
#include <stdlib.h>

#include "language.h"
#include "frontend.h"
#include "lang_dump.h"

static int main_exit_failure(language_t *language);

int main(int argc, const char *argv[]) {
    language_t language = {};
    if(frontend_ctor(&language, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    fprintf(stderr, "~1");
    if(parse_tokens(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    for(size_t i = 0; i < language.nodes.size; i++) {
        fprintf(stderr, "%d %d\n", language.nodes.nodes[i].type, language.nodes.nodes[i].value.opcode);
    }
    fprintf(stderr, "~2");
    if(parse_syntax(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    fprintf(stderr, "~3");
    dump_tree(&language, "huy");
    if(write_tree(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }

    frontend_dtor(&language);
    return EXIT_SUCCESS;
}

int main_exit_failure(language_t *language) {
    frontend_dtor(language);
    return EXIT_FAILURE;
}
