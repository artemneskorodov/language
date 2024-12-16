#include <math.h>

#include "utils.h"

size_t file_size(FILE *file) {
    long current_position = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t size = (unsigned long)ftell(file);
    fseek(file, current_position, SEEK_SET);
    return size;
}

bool is_equal(double first, double second) {
    const double epsilon = 10e-6;
    if(fabs(first - second) < epsilon) {
        return true;
    }
    return false;
}

size_t get_random_index(size_t size) {
    return (size_t)rand() % size;
}
