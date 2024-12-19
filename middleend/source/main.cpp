#include <stdlib.h>

#include "language.h"
#include "middleend.h"
#include "lang_dump.h"
#include "colors.h"

static int main_exit_failure(language_t *ctx);

//===========================================================================//

int main(int argc, const char *argv[]) {
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \n"
                 "|                                     |\n"
                 "|           Optimizing tree           |\n"
                 "|_____________________________________|\n");
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }
    //-----------------------------------------------------------------------//
    language_t ctx = {};
    if(middleend_ctor(&ctx, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully initialized context\n");
    dump_tree(&ctx, "before");
    //-----------------------------------------------------------------------//
    if(optimize_tree(&ctx) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully optimized tree\n");
    dump_tree(&ctx, "after");
    //-----------------------------------------------------------------------//
    if(write_tree(&ctx) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&ctx);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully wrote tree\n");
    //-----------------------------------------------------------------------//
    middleend_dtor(&ctx);
    return EXIT_SUCCESS;
}

//===========================================================================//

int main_exit_failure(language_t *ctx) {
    middleend_dtor(ctx);
    return EXIT_FAILURE;
}
