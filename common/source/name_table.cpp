#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "language.h"
#include "name_table.h"
#include "colors.h"
#include "custom_assert.h"

language_error_t name_table_ctor(language_t *language, size_t capacity) {
    language->name_table.identifiers = (identifier_t *)calloc(capacity, sizeof(language->name_table.identifiers[0]));
    if(language->name_table.identifiers == NULL) {
        print_error("Error while allocating memory to identifiers.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    language->name_table.size = 0;
    language->name_table.capacity = capacity;
    return LANGUAGE_SUCCESS;
}

language_error_t name_table_add(language_t *language, const char *name, size_t length, size_t *output, identifier_type_t type) {
    //TODO add checking size
    identifier_t *identifier = language->name_table.identifiers + language->name_table.size;

    identifier->name = name;
    identifier->length = length;
    identifier->type = type;

    if(output != NULL) {
        *output = language->name_table.size;
    }

    language->name_table.size++;
    return LANGUAGE_SUCCESS;
}

language_error_t name_table_dtor(language_t *language) {
    free(language->name_table.identifiers);
    return LANGUAGE_SUCCESS;
}

language_error_t set_memory_addr(language_t *language, language_node_t *node, size_t addr) {
    _C_ASSERT(node->type == NODE_TYPE_IDENTIFIER, return LANGUAGE_INVALID_NODE_TYPE);
    size_t index = node->value.identifier;
    if(index >= language->name_table.size) {
        print_error("Node index is bigger that name table size.\n");
    }
    language->name_table.identifiers[index].memory_addr = addr;
    return LANGUAGE_SUCCESS;
}

language_error_t variables_stack_ctor(language_t *language, size_t capacity) {
    language->name_table.stack = (size_t *)calloc(capacity, sizeof(language->name_table.stack[0]));
    if(language->name_table.stack == 0) {
        print_error("Error while allocating memory for variables stack.\n");
        return LANGUAGE_MEMORY_ERROR;
    }

    language->name_table.stack_size = 0;
    return LANGUAGE_SUCCESS;
}

language_error_t variables_stack_push(language_t *language, size_t index) {
    language->name_table.stack[language->name_table.stack_size++] = index;
    return LANGUAGE_SUCCESS;
}

language_error_t variables_stack_remove(language_t *language, size_t number) {
    memset(language->name_table.stack + language->name_table.stack_size - number, 0, number * sizeof(language->name_table.stack[0]));
    language->name_table.stack_size -= number;
    return LANGUAGE_SUCCESS;
}

language_error_t variables_stack_dtor(language_t *language) {
    free(language->name_table.stack);
    return LANGUAGE_SUCCESS;
}

language_error_t used_names_ctor(language_t *language, size_t capacity) {
    language->name_table.used_names = (name_t *)calloc(capacity, sizeof(language->name_table.used_names[0]));
    if(language->name_table.used_names == NULL) {
        print_error("Error while allocating used names memory.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    language->name_table.used_names_size = 0;
    return LANGUAGE_SUCCESS;
}

language_error_t used_names_add(language_t *language, const char *name, size_t length, size_t *index) {
    language->name_table.used_names[language->name_table.used_names_size].length = length;
    language->name_table.used_names[language->name_table.used_names_size].name = name;
    *index = language->name_table.used_names_size;
    language->name_table.used_names_size++;
    return LANGUAGE_SUCCESS;
}

language_error_t used_names_dtor(language_t *language) {
    free(language->name_table.used_names);
    return LANGUAGE_SUCCESS;
}
