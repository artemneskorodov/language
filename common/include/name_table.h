#ifndef NAME_TABLE_H
#define NAME_TABLE_H

//===========================================================================//

#include <stdio.h>

//===========================================================================//

#include "language.h"

//===========================================================================//

language_error_t name_table_ctor        (language_t         *language,
                                         size_t             capacity);

language_error_t name_table_add         (language_t        *language,
                                         const char        *name,
                                         size_t             length,
                                         size_t            *output,
                                         identifier_type_t  type);

language_error_t name_table_dtor        (language_t        *language);

language_error_t set_memory_addr        (language_t        *language,
                                         language_node_t   *node,
                                         size_t             addr);

language_error_t used_names_ctor        (language_t        *language,
                                         size_t             capacity);

language_error_t used_names_add         (language_t        *language,
                                         const char        *name,
                                         size_t             length,
                                         size_t            *output);

language_error_t used_names_dtor        (language_t        *language);

language_error_t variables_stack_ctor   (language_t        *language,
                                         size_t             capacity);

language_error_t variables_stack_push   (language_t        *language,
                                         size_t             nt_index);

language_error_t variables_stack_remove (language_t        *language,
                                         size_t             number);

language_error_t variables_stack_dtor   (language_t        *language);

//===========================================================================//

#endif
