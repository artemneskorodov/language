#include "stdio.h"
#include "stdlib.h"

static const size_t command_length = 256;

int main(int argc, const char *argv[]) {
    if(argc != 2) {
        printf("NAME!!!");
        return EXIT_FAILURE;
    }

    char command[command_length] = {};
    snprintf(command, command_length, "./frontend.out -i %s.kvm -o %s.tree", argv[1], argv[1]);
    system(command);
    snprintf(command, command_length, "./middleend.out -i %s.tree -o %s.tree", argv[1], argv[1]);
    system(command);
    snprintf(command, command_length, "./backend.out -i %s.tree -o %s.asm", argv[1], argv[1]);
    system(command);
    snprintf(command, command_length, "./frontstart.out -i %s.tree -o %s_back.kvm", argv[1], argv[1]);
    system(command);
    return EXIT_SUCCESS;
}
