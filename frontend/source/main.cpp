#include <stdio.h>
#include <stdlib.h>

#include "language.h"
#include "frontend.h"
#include "lang_dump.h"
#include "syntax_parser.h"

static int main_exit_failure(language_t *language);

int main(int argc, const char *argv[]) {
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }

    language_t language = {};
    if(frontend_ctor(&language, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    fprintf(stderr, "~1\n");
    if(parse_tokens(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    fprintf(stderr, "~2\n");
    fprintf(stderr, "===============\n");
    for(size_t i = 0; i < language.nodes.size; i++) {
        fprintf(stderr, "%d ", language.nodes.nodes[i].type);
        switch(language.nodes.nodes[i].type) {
            case NODE_TYPE_IDENTIFIER: {
                fprintf(stderr, "%lu\n", language.nodes.nodes[i].value.identifier);
                break;
            }
            case NODE_TYPE_NUMBER: {
                fprintf(stderr, "%lg\n", language.nodes.nodes[i].value.number);
                break;
            }
            case NODE_TYPE_OPERATION: {
                fprintf(stderr, "%d\n", language.nodes.nodes[i].value.opcode);
                break;
            }
        }
    }
    fprintf(stderr, "===============\n");
    if(parse_syntax(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    fprintf(stderr, "~3\n");
    dump_tree(&language, "huy");
    if(write_tree(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }

    frontend_dtor(&language);
    fprintf(stderr, "END success\n");
    return EXIT_SUCCESS;
}

int main_exit_failure(language_t *language) {
    frontend_dtor(language);
    return EXIT_FAILURE;
}
