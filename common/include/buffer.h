#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#include "language.h"

language_error_t buffer_write_byte(language_t *ctx, uint8_t byte);
language_error_t buffer_write(language_t *ctx, const uint8_t *data, size_t size);
language_error_t buffer_reset(language_t *ctx);
language_error_t buffer_check_size(language_t *ctx, size_t needed_sizes);
language_error_t buffer_write_qword(language_t *ctx, uint64_t data);
language_error_t buffer_write_string(language_t *ctx, const char *data, size_t size);

#endif
