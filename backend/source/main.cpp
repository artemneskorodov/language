#include <stdio.h>
#include <stdlib.h>

//===========================================================================//

#include "backend.h"
#include "language.h"
#include "lang_dump.h"
#include "colors.h"

//===========================================================================//

static int main_exit_failure(language_t *language);
static language_error_t run_compilation(language_t *language);

//===========================================================================//

int main(int argc, const char *argv[]) {
    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \n"
                 "|                                     |\n"
                 "|     Compiling tree to assembler     |\n"
                 "|_____________________________________|\n");
    if(verify_keywords() != LANGUAGE_SUCCESS) {
        return EXIT_FAILURE;
    }
    //-----------------------------------------------------------------------//
    language_t language = {};
    if(backend_ctor(&language, argc, argv) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully initialized context\n");
    //-----------------------------------------------------------------------//
    if(read_tree(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    if(add_stdlib_id(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully read syntax\n");
    //-----------------------------------------------------------------------//
    if(dump_tree(&language, "main dump after reading.") != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully dumped tree\n");
    //-----------------------------------------------------------------------//
    if(run_compilation(&language) != LANGUAGE_SUCCESS) {
        return main_exit_failure(&language);
    }
    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully wrote compiled code\n");
    //-----------------------------------------------------------------------//
    backend_dtor(&language);
    return EXIT_SUCCESS;
}

//===========================================================================//

int main_exit_failure(language_t *language) {
    backend_dtor(language);
    return EXIT_FAILURE;
}

//===========================================================================//

language_error_t run_compilation(language_t *language) {
    switch(language->machine_flag) {
        case MACHINE_SPU: {
            return compile_spu(language);
        }
        case MACHINE_ELF_X86: {
            return compile_elf(language);
        }
        case MACHINE_ASM_X86: {
            return compile_nasm(language);
        }
        default: {
            print_error("Unexpected machine flag.");
            return LANGUAGE_UNEXPECTED_MACHINE;
        }
    }
}

//===========================================================================//
