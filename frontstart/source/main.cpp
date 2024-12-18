#include <stdio.h>
#include <stdlib.h>

#include "frontstart.h"
#include "language.h"
#include "lang_dump.h"
#include "colors.h"

static int main_exit_failure(language_t *ctx);

int main(int argc, const char *argv[]) {
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \n"
                 "|                                     |\n"
                 "|       Parsing tree to source        |\n"
                 "|_____________________________________|\n");
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }

    language_t ctx = {};
    if(frontstart_ctor(&ctx, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully initialized context\n");
    if(read_tree(&ctx) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully read tree\n");
    if(dump_tree(&ctx, "dump after reading") != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully dumped tree\n");
    if(frontstart_write(&ctx) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully wrote to source\n");

    frontstart_dtor(&ctx);
    return EXIT_SUCCESS;
}

int main_exit_failure(language_t *ctx) {
    frontstart_dtor(ctx);
    return EXIT_FAILURE;
}
