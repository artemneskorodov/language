#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//===========================================================================//

#include "language.h"
#include "colors.h"
#include "utils.h"
#include "name_table.h"
#include "custom_assert.h"
//===========================================================================//

struct file_elem_t {
    char                symbol;
    void               *output;
    language_error_t  (*reader)(language_t *, void *, char, file_elem_t *);
};

//===========================================================================//

struct flag_prototype_t {
    const char         *long_name;
    const char         *short_name;
    size_t              params;
    language_error_t  (*handler)(language_t *, int, size_t, const char **);
};

//===========================================================================//

static language_error_t handler_output   (language_t       *ctx,
                                          int               argc,
                                          size_t            position,
                                          const char       *argv[]);

static language_error_t handler_input    (language_t       *ctx,
                                          int               argc,
                                          size_t            position,
                                          const char       *argv[]);

static language_error_t skip_spaces      (language_t       *ctx);

static language_error_t write_subtree    (language_t       *ctx,
                                          language_node_t  *node,
                                          FILE             *output);

static language_error_t read_name_table  (language_t       *ctx);

static language_error_t read_subtree     (language_t       *ctx,
                                          language_node_t **output);

static language_error_t get_nt_name      (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t nt_error         (language_t        *ctx);

static language_error_t get_size         (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t get_id_type      (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t check_char       (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t get_node_type    (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t get_node_value   (language_t        *ctx,
                                          void              *value,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t get_node_left    (language_t        *ctx,
                                          void *             output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t get_node_right   (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t create_node      (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

static language_error_t get_bool         (language_t        *ctx,
                                          void              *output,
                                          char               symbol,
                                          file_elem_t       *rules);

//===========================================================================//

static const flag_prototype_t SupportedFlags[] = {
    {"-o", "--output", 1, handler_output},
    {"-i", "--input" , 1, handler_input }
};

//===========================================================================//

language_error_t nodes_storage_ctor(language_t *ctx, size_t capacity) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->nodes.nodes = (language_node_t *)calloc(capacity,
                                                 sizeof(ctx->nodes.nodes[0]));
    if(ctx->nodes.nodes == NULL) {
        print_error("Error while allocating nodes memory.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    //-----------------------------------------------------------------------//
    ctx->nodes.size     = 0;
    ctx->nodes.capacity = capacity;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t nodes_storage_add(language_t       *ctx,
                                   node_type_t       type,
                                   value_t           value,
                                   const char       *name,
                                   size_t            length,
                                   language_node_t **output) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    if(ctx->nodes.size >= ctx->nodes.capacity) {
        print_error("Nodes storage size does not know how to reallocate memory. "
                    "Plz implement it or, change capacity when initializing.\n");
        return LANGUAGE_MEMORY_ERROR;
    }
    //-----------------------------------------------------------------------//
    language_node_t *node = ctx->nodes.nodes + ctx->nodes.size;
    ctx->nodes.size++;
    node->type               = type;
    node->value              = value;
    node->source_info.name   = name;
    node->source_info.length = length;
    node->source_info.line   = ctx->frontend_info.current_line;
    //-----------------------------------------------------------------------//
    if(output != NULL) {
        *output = node;
    }
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t nodes_storage_dtor(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    free(ctx->nodes.nodes);
    ctx->nodes.nodes    = NULL;
    ctx->nodes.capacity = 0;
    ctx->nodes.size     = 0;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t read_tree(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    FILE *input = fopen(ctx->input_file, "rb");
    //-----------------------------------------------------------------------//
    if(input == NULL) {
        print_error("Error while opening file '%s'.\n", ctx->input_file);
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    ctx->input_size = file_size(input);
    ctx->input = (char *)calloc(ctx->input_size, sizeof(ctx->input[0]));
    if(fread(ctx->input,
             sizeof(char),
             ctx->input_size,
             input) != ctx->input_size) {
        print_error("Error while reading code tree file.\n");
        fclose(input);
        return LANGUAGE_READING_TREE_ERROR;
    }
    fclose(input);
    //-----------------------------------------------------------------------//
    ctx->input_position = ctx->input;
    _RETURN_IF_ERROR(read_name_table(ctx));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    char *nodes_number_end = NULL;
    size_t nodes_number = strtoull(ctx->input_position, &nodes_number_end, 10);
    if(nodes_number_end == NULL) {
        print_error("Unexpected code file structure. "
                    "It is expected to see nodes number before nodes.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    ctx->input_position = nodes_number_end;
    _RETURN_IF_ERROR(nodes_storage_ctor(ctx, nodes_number));
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(read_subtree(ctx, &ctx->root));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t read_name_table(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    char *length_end = NULL;
    size_t name_table_size = strtoull(ctx->input_position, &length_end, 10);
    if(length_end == NULL) {
        print_error("Tree file expected to start with name table length.\n");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(name_table_ctor(ctx, name_table_size));
    ctx->input_position = length_end;
    for(size_t elem = 0; elem < name_table_size; elem++) {
        //-------------------------------------------------------------------//
        size_t            name_size    = 0;
        const char       *name_start   = NULL;
        identifier_type_t type         = (identifier_type_t)0;
        bool              is_global    = false;
        size_t            param_number = 0;

        file_elem_t nt_elems[] = {
            {'{', NULL         , check_char },
            {EOF, &name_size   , get_size   },
            {EOF, &name_start  , get_nt_name},
            {EOF, &type        , get_id_type},
            {EOF, &is_global   , get_bool   },
            {EOF, &param_number, get_size   },
            {'}', NULL         , check_char }};
        //-------------------------------------------------------------------//
        for(size_t i = 0; i < sizeof(nt_elems) / sizeof(nt_elems[0]); i++) {
            _RETURN_IF_ERROR(nt_elems[i].reader(ctx,
                                                nt_elems[i].output,
                                                nt_elems[i].symbol,
                                                nt_elems));
        }
        //-------------------------------------------------------------------//
        size_t name_index = 0;
        _RETURN_IF_ERROR(name_table_add(ctx,
                                        name_start,
                                        name_size,
                                        &name_index,
                                        type));
        identifier_t *ident = ctx->name_table.identifiers + name_index;
        ident->parameters_number = param_number;
        ident->is_global         = is_global;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_size(language_t *ctx, void *length, char, file_elem_t *) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(length != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    char *name_length_end = NULL;
    *(size_t *)length = strtoull(ctx->input_position, &name_length_end, 10);
    if(name_length_end == NULL) {
        return nt_error(ctx);
    }
    ctx->input_position = name_length_end;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_id_type(language_t *ctx, void *type, char, file_elem_t *) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(type != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    char *type_end = NULL;
    *(identifier_type_t *)type = (identifier_type_t)strtol(ctx->input_position,
                                                           &type_end, 10);
    if(type_end == NULL) {
        return nt_error(ctx);
    }
    ctx->input_position = type_end;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_nt_name(language_t *ctx, void *name, char, file_elem_t *) {
    _C_ASSERT(ctx  != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(name != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    *(const char **)name = ctx->input_position;
    while(isalpha(*ctx->input_position) || *ctx->input_position == '_') {
        ctx->input_position++;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t check_char(language_t *ctx, void *, char symbol, file_elem_t *) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    if(*ctx->input_position != symbol) {
        return nt_error(ctx);
    }
    ctx->input_position++;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_bool(language_t *ctx, void *output, char, file_elem_t *) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    if(*ctx->input_position == '0') {
        *(bool *)output = false;
    }
    else if(*ctx->input_position == '1') {
        *(bool *)output = true;
    }
    else {
        return nt_error(ctx);
    }
    ctx->input_position++;
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t nt_error(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    print_error("Error on " SZ_SP "Name table element is "
                "expected to have structure: "
                "{LENGTH \"NAME\" TYPE PARAMS}\n",
                (size_t)ctx->input_position -
                (size_t)ctx->input);
    //-----------------------------------------------------------------------//
    return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
}

//===========================================================================//

language_error_t read_subtree(language_t *ctx, language_node_t **output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    node_type_t      type  = (node_type_t)0;
    value_t          value = {};
    language_node_t *node  = NULL;

    file_elem_t node_elems[] = {
        {'{', NULL  , check_char    },
        {EOF, &type , get_node_type },
        {EOF, &value, get_node_value},
        {EOF, &node , create_node   },
        {EOF, NULL  , get_node_left },
        {EOF, NULL  , get_node_right},
        {'}', NULL  , check_char    }};
    //-----------------------------------------------------------------------//
    for(size_t i = 0; i < sizeof(node_elems) / sizeof(node_elems[0]); i++) {
        _RETURN_IF_ERROR(node_elems[i].reader(ctx,
                                              node_elems[i].output,
                                              node_elems[i].symbol,
                                              node_elems));
    }
    //-----------------------------------------------------------------------//
    *output = node;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_node_type(language_t *ctx,
                               void *output,
                               char,
                               file_elem_t *) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    //-----------------------------------------------------------------------//
    _RETURN_IF_ERROR(skip_spaces(ctx));
    char *type_end = NULL;
    node_type_t type = (node_type_t)strtol(ctx->input_position,
                                           &type_end, 10);
    if(type_end == NULL) {
        print_error("Nodes are written as {TYPE VALUE LEFT RIGHT}");
        return LANGUAGE_UNKNOWN_CODE_TREE_TYPE;
    }
    ctx->input_position = type_end;
    //-----------------------------------------------------------------------//
    *(node_type_t *)output = type;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_node_value(language_t  *ctx,
                                void        *output,
                                char,
                                file_elem_t *rules) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    _C_ASSERT(rules  != NULL, return LANGUAGE_RULES_NULL );
    //-----------------------------------------------------------------------//
    node_type_t type = *(node_type_t *)rules[1].output;
    char *value_end = NULL;
    value_t *value = (value_t *)output;
    switch(type) {
        case NODE_TYPE_IDENTIFIER: {
            value->identifier = strtoull(ctx->input_position,
                                         &value_end, 10);
            break;
        }
        case NODE_TYPE_NUMBER: {
            value->number = strtod(ctx->input_position,
                                   &value_end);
            break;
        }
        case NODE_TYPE_OPERATION: {
            value->opcode = (operation_t)strtol(ctx->input_position,
                                                &value_end, 10);
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    //-----------------------------------------------------------------------//
    ctx->input_position = value_end;
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t create_node(language_t *ctx, void *output, char, file_elem_t *rules) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL   );
    _C_ASSERT(output != NULL, return LANGUAGE_NULL_OUTPUT);
    _C_ASSERT(rules  != NULL, return LANGUAGE_RULES_NULL );
    //-----------------------------------------------------------------------//
    node_type_t  type  = *(node_type_t *)rules[1].output;
    value_t     *value = (value_t *)rules[2].output;
    _RETURN_IF_ERROR(nodes_storage_add(ctx,
                                       type,
                                       *value,
                                       NULL, 0,
                                       (language_node_t **)output));
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_node_left(language_t *ctx, void *, char, file_elem_t *rules) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL  );
    _C_ASSERT(rules != NULL, return LANGUAGE_RULES_NULL);
    //-----------------------------------------------------------------------//
    language_node_t *node = *(language_node_t **)rules[3].output;
    _RETURN_IF_ERROR(skip_spaces(ctx));
    if(*ctx->input_position != '_') {
        _RETURN_IF_ERROR(read_subtree(ctx, &node->left));
    }
    else {
        ctx->input_position++;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t get_node_right(language_t *ctx, void *, char, file_elem_t *rules) {
    _C_ASSERT(ctx   != NULL, return LANGUAGE_CTX_NULL  );
    _C_ASSERT(rules != NULL, return LANGUAGE_RULES_NULL);
    //-----------------------------------------------------------------------//
    language_node_t *node = *(language_node_t **)rules[3].output;
    _RETURN_IF_ERROR(skip_spaces(ctx));
    if(*ctx->input_position != '_') {
        _RETURN_IF_ERROR(read_subtree(ctx, &node->right));
    }
    else {
        ctx->input_position++;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t write_tree(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    FILE *output = fopen(ctx->output_file, "wb");
    if(output == NULL) {
        print_error("Error while opening file to write tree.\n");
        return LANGUAGE_OPENING_FILE_ERROR;
    }
    //-----------------------------------------------------------------------//
    fprintf(output, SZ_SP "\r\n", ctx->name_table.size);
    for(size_t elem = 0; elem < ctx->name_table.size; elem++) {
        identifier_t *ident = ctx->name_table.identifiers + elem;
        fprintf(output,
                "{" SZ_SP " %.*s %d %d %lu}\n",
                ident->length,
                (int)ident->length,
                ident->name,
                ident->type,
                ident->is_global,
                ident->parameters_number);
    }
    //-----------------------------------------------------------------------//
    fprintf(output, "\r\n" SZ_SP "\r\n", ctx->nodes.size);
    language_error_t error_code = write_subtree(ctx, ctx->root, output);
    //-----------------------------------------------------------------------//
    fclose(output);
    return error_code;
}

//===========================================================================//

language_error_t write_subtree(language_t *ctx, language_node_t *node, FILE *output) {
    _C_ASSERT(ctx    != NULL, return LANGUAGE_CTX_NULL          );
    _C_ASSERT(node   != NULL, return LANGUAGE_NODE_NULL         );
    _C_ASSERT(output != NULL, return LANGUAGE_OPENING_FILE_ERROR);
    //-----------------------------------------------------------------------//s
    fprintf(output, "{ %d ", node->type);
    switch(node->type) {
        case NODE_TYPE_IDENTIFIER: {
            fprintf(output, SZ_SP " ", node->value.identifier);
            break;
        }
        case NODE_TYPE_NUMBER: {
            fprintf(output, "%lg ", node->value.number);
            break;
        }
        case NODE_TYPE_OPERATION: {
            fprintf(output, "%d ", node->value.opcode);
            break;
        }
        default: {
            print_error("Unknown node type.\n");
            return LANGUAGE_UNKNOWN_NODE_TYPE;
        }
    }
    //-----------------------------------------------------------------------//
    if(node->left != NULL) {
        _RETURN_IF_ERROR(write_subtree(ctx, node->left, output));
    }
    else {
        fprintf(output, "_ ");
    }
    if(node->right != NULL) {
        _RETURN_IF_ERROR(write_subtree(ctx, node->right, output));
    }
    else {
        fprintf(output, "_ ");
    }
    //-----------------------------------------------------------------------//
    fprintf(output, "} ");
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t verify_keywords(void) {
    for(size_t i = 1; i < sizeof(KeyWords) / sizeof(KeyWords[0]); i++) {
        if((size_t)KeyWords[i].code != i) {
            print_error("Broken keywords table.\n");
            return LANGUAGE_BROKEN_KEYWORDS_TABLE;
        }
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t parse_flags(language_t *ctx, int argc, const char *argv[]) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    if(argc < 1) {
        print_error("Undefined parsing flags error. No falgs found.\n");
        return LANGUAGE_PARSING_FLAGS_ERROR;
    }
    //-----------------------------------------------------------------------//
    for(size_t elem = 1; elem < (size_t)argc; ) {
        size_t index = PoisonIndex;
        size_t flags_num = sizeof(SupportedFlags) / sizeof(SupportedFlags[0]);
        for(size_t i = 0; i < flags_num; i++) {
            if(strcmp(SupportedFlags[i].short_name, argv[elem]) == 0 ||
               strcmp(SupportedFlags[i].long_name, argv[elem]) == 0) {
                index = i;
                break;
            }
        }
        if(index == PoisonIndex) {
            print_error("Unknown flag '%s'.\n", argv[elem]);
            return LANGUAGE_UNKNOWN_FLAG;
        }
        if(elem + SupportedFlags[index].params >= (size_t)argc) {
            print_error("Flag '%s' is expected to have " SZ_SP
                        " parameters after it.\n",
                        argv[elem],
                        SupportedFlags[index].params);
            return LANGUAGE_PARSING_FLAGS_ERROR;
        }
        _RETURN_IF_ERROR(SupportedFlags[index].handler(ctx,
                                                       argc,
                                                       elem,
                                                       argv));
        elem += SupportedFlags[index].params + 1;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t handler_input(language_t *ctx,
                               int       /*argc*/,
                               size_t      position,
                               const char *argv[]) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->input_file = argv[position + 1];
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t handler_output(language_t *ctx,
                                int       /*argc*/,
                                size_t      position,
                                const char *argv[]) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    ctx->output_file = argv[position + 1];
    return LANGUAGE_SUCCESS;
}

//===========================================================================//

language_error_t skip_spaces(language_t *ctx) {
    _C_ASSERT(ctx != NULL, return LANGUAGE_CTX_NULL);
    //-----------------------------------------------------------------------//
    while(isspace(*ctx->input_position)) {
        ctx->input_position++;
    }
    //-----------------------------------------------------------------------//
    return LANGUAGE_SUCCESS;
}

//===========================================================================//
