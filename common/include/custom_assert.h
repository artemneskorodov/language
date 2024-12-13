#ifndef CUSTOM_ASSERT_H
#define CUSTOM_ASSERT_H

#ifndef NDEBUG
#define _C_ASSERT(expression, operand) {                    \
    if(expression) {(void)0;}                               \
    else {                                                  \
        print_assert_error(#expression, __LINE__, __FILE__);\
        operand;                                            \
        }                                                   \
    }
#else
#define C_ASSERT(expression, return_value) ((void)0);
#endif

void print_assert_error(const char *string,
                        int         line_number,
                        const char *filename);

#endif
