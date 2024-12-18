#include <stdio.h>
#include <stdlib.h>

//====================================================================================//

#include "language.h"
#include "frontend.h"
#include "lang_dump.h"
#include "syntax_parser.h"
#include "colors.h"

//====================================================================================//

static int main_exit_failure(language_t *language);

//====================================================================================//

int main(int argc, const char *argv[]) {
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \n"
                 "|                                     |\n"
                 "|     Parsing source code to tree     |\n"
                 "|_____________________________________|\n");
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }
    //--------------------------------------------------------------------------------//
    language_t language = {};
    if(frontend_ctor(&language, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully initialized context\n");
    //--------------------------------------------------------------------------------//
    if(parse_tokens(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully parsed tokens\n");
    //--------------------------------------------------------------------------------//
    if(parse_syntax(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully parsed syntax\n");
    //--------------------------------------------------------------------------------//
    dump_tree(&language, "frontend result");
    //--------------------------------------------------------------------------------//
    if(write_tree(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully wrote tree to file\n");
    //--------------------------------------------------------------------------------//
    frontend_dtor(&language);
    return EXIT_SUCCESS;
}

//====================================================================================//

int main_exit_failure(language_t *language) {
    frontend_dtor(language);
    return EXIT_FAILURE;
}

//====================================================================================//
